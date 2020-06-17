/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2020 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2020 The MITRE Corporation                                       *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 *    http://www.apache.org/licenses/LICENSE-2.0                              *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 ******************************************************************************/


#include <iostream>
#include <string>
#include <wordexp.h>

#include <boost/algorithm/string/trim.hpp>

#include <opencv2/imgproc.hpp>

#include "Utils.h"

using std::string;
using std::vector;

using MPF::COMPONENT::MPFAudioJob;
using MPF::COMPONENT::MPFDetectionDataType;
using MPF::COMPONENT::MPFDetectionError;
using MPF::COMPONENT::MPFImageJob;
using MPF::COMPONENT::MPFImageLocation;
using MPF::COMPONENT::MPFVideoJob;
using MPF::COMPONENT::MPFVideoTrack;


namespace MPF { namespace COMPONENT { namespace Utils {

    namespace {
        [[noreturn]] void LogAndReThrowExceptionInner(const string &job_name, MPFDetectionDataType dataType,
                                                      const log4cxx::LoggerPtr &logger) {
            try {
                DetectionComponentUtils::ReThrowAsMpfDetectionException(dataType);
            }
            catch (const std::exception &ex) {
                LOG4CXX_ERROR(logger, "[" << job_name << "] " << ex.what());
                throw;
            }
        }
    }


    [[noreturn]] void LogAndReThrowException(const MPFVideoJob &job, const log4cxx::LoggerPtr &logger) {
        LogAndReThrowExceptionInner(job.job_name, MPFDetectionDataType::VIDEO, logger);
    }

    [[noreturn]] void LogAndReThrowException(const MPFImageJob &job, const log4cxx::LoggerPtr &logger) {
        LogAndReThrowExceptionInner(job.job_name, MPFDetectionDataType::IMAGE, logger);
    }

    [[noreturn]] void LogAndReThrowException(const MPFAudioJob &job, const log4cxx::LoggerPtr &logger) {
        LogAndReThrowExceptionInner(job.job_name, MPFDetectionDataType::AUDIO, logger);
    }

    [[noreturn]] void LogAndReThrowException(const MPFGenericJob &job, const log4cxx::LoggerPtr &logger) {
        LogAndReThrowExceptionInner(job.job_name, MPFDetectionDataType::UNKNOWN, logger);
    }

    // This function performs shell-like file name expansion. It
    // returns an error string. If that string is not empty, then the
    // filename expansion failed and the exp_filename output should
    // not be used.
    std::string expandFileName(const std::string &filename, std::string &exp_filename) {
        wordexp_t my_exp;
        std::string err_string;
        int rc = wordexp(filename.c_str(), &my_exp, WRDE_UNDEF);
        if (rc) {
            switch (rc) {
                case WRDE_BADCHAR:
                    err_string = "Illegal occurrence of an unescaped character from the set: \\n, |, &, ;, <, >, (, ), {, }.";
                    break;
                case WRDE_BADVAL:
                    err_string = "An undefined shell variable was referenced.";
                    break;
                case WRDE_SYNTAX:
                    err_string = "Shell syntax error, such as unbalanced parentheses or unmatched quotes.";
                    break;
                default:
                    err_string = "Unknown error.";
            }
            return err_string;
        }
        if (*my_exp.we_wordv != nullptr) {
            exp_filename = *my_exp.we_wordv;
        }
        wordfree(&my_exp);

        return "";
    }


    void trim(std::string &str) {
        boost::trim(str);
    }


    cv::Mat ConvertToGray(const cv::Mat &image) {
        cv::Mat gray;
        if (image.channels() == 3) {
            cv::cvtColor(image, gray, CV_BGR2GRAY);
        }
        else if (image.channels() == 1) {
            gray = image.clone();
        }
        else {
            std::cout << "--(!)Error reading frames" << std::endl;
        }

        return gray;
    }


    cv::Rect ImageLocationToCvRect(const MPFImageLocation &location) {
        return cv::Rect(location.x_left_upper, location.y_left_upper, location.width, location.height);
    }


    MPFImageLocation CvRectToImageLocation(const cv::Rect &rect, float confidence) {
        return MPFImageLocation(rect.x, rect.y, rect.width, rect.height, confidence);
    }


    bool IsExistingRectIntersection(const cv::Rect new_rect,
                                    const vector<cv::Rect> &existing_rects,
                                    int &intersection_index) {
        intersection_index = -1;

        if (!existing_rects.empty()) {
            for (vector<cv::Rect>::const_iterator rect = existing_rects.begin(); rect != existing_rects.end(); ++rect) {
                ++intersection_index;

                cv::Rect existing_rect(*rect);

                // opencv allows for this comparison
                if (new_rect == existing_rect) {
                    // assuming the index is equal - TODO: not the best way to check - could cause an infinite loop
                    continue;
                }

                cv::Rect intersection = existing_rect & new_rect;  // (rectangle intersection)

                if (intersection.area() > 0) {
                    return true;
                }
            }
            // reset intersection_index to -1 before return - intersection_index could be used as the check
            // rather than the return bool
            intersection_index = -1;
            return false;
        }

        return false;
    }


    void DrawText(cv::Mat &image, int frame_index) {
        // assuming there won't be more than 10 chars in the frame_index
        string text = std::to_string(frame_index);
        int length = text.size();

        int font_face = cv::FONT_HERSHEY_PLAIN;
        double font_scale = 0.7;
        int thickness = 1;

        cv::Point text_origin((image.cols - 20 - 5 * length), (image.rows - 10));

        putText(image, text, text_origin, font_face, font_scale,
                cv::Scalar(0, 255, 0), thickness, 8);
    }


    void DrawTracks(cv::Mat &image,
                    const vector<MPFVideoTrack> &tracks_to_draw,
                    const vector<MPFImageLocation> &current_locations,
                    int tracks_completed_count,
                    vector<int> track_indexes) {
        int y_pos = image.rows - 10;
        if (tracks_to_draw.empty()) {
            return;
        }
        else {
            y_pos = y_pos - (tracks_to_draw.size() - 1) * 10;
        }

        for (vector<cv::Point2f>::size_type i = 0; i < tracks_to_draw.size(); i++) {
            int track_index = tracks_completed_count + i;

            MPFVideoTrack track = tracks_to_draw[i];

            MPFImageLocation latest_location;
            if (current_locations.empty()) {
                // assume this is displaying during tracking
                latest_location = track.frame_locations.rbegin()->second; // last element in map
            }
            else {
                // display post tracking or output video generation
                latest_location = current_locations[i];
                if (!track_indexes.empty()) {
                    track_index = track_indexes[i];
                }
            }

            string text = std::to_string(track_index) + ": ";
            text = text + std::to_string(latest_location.x_left_upper) + ",";
            text = text + std::to_string(latest_location.y_left_upper) + ",";
            text = text + std::to_string(latest_location.width) + ",";
            text = text + std::to_string(latest_location.height) + ",";
            text = text + std::to_string(latest_location.confidence);

            cv::Point text_origin(10, y_pos);

            int font_face = cv::FONT_HERSHEY_PLAIN;
            double font_scale = 0.7;
            int thickness = 1;

            putText(image, text, text_origin, font_face, font_scale,
                    cv::Scalar(0, 255, 0), thickness, 8);

            // try to place the track index at the bottom center of the bounding box!!
            // TODO: could be out of image bounds...
            int x = latest_location.x_left_upper + latest_location.width / 2 - 6;
            int y = latest_location.y_left_upper + latest_location.height + 20;
            text_origin = cv::Point(x, y);

            font_scale = 1.2;
            thickness = 2;
            putText(image, std::to_string(track_index), text_origin, font_face, font_scale,
                    cv::Scalar(255, 0, 255), thickness, 8);

            y_pos += 10;
        }
    }
}}}

