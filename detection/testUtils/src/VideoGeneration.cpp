/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2022 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2022 The MITRE Corporation                                       *
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

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "Utils.h"

#include "VideoGeneration.h"


using std::string;
using std::vector;

using cv::Mat;
using cv::Point;
using cv::Rect;
using cv::Scalar;
using cv::Size;
using cv::VideoCapture;
using cv::VideoWriter;


namespace MPF { namespace COMPONENT {

    VideoGeneration::VideoGeneration(bool display_window)
            : imshow_on(display_window) {

    }


    vector<MPFImageLocation>
    VideoGeneration::GetImageLocationsAtFrameIndex(int frame_index, const vector<MPFVideoTrack> &tracks,
                                                   vector<int> &track_indexes) {
        vector<MPFImageLocation> locations;
        int track_index = 0;
        for (vector<MPFVideoTrack>::const_iterator track = tracks.begin();
             track != tracks.end(); track++) {
            if (track->frame_locations.find(frame_index) != track->frame_locations.end()) {
                locations.emplace_back(track->frame_locations.at(frame_index));
                track_indexes.push_back(track_index);
            }
            track_index++;
        }
        return locations;
    }


    Mat VideoGeneration::Rotate(const cv::Mat &src) {
        Mat rot_mat(2, 3, CV_32FC1);
        Mat warp_rotate_dst;

        Point center = Point(src.cols / 2, src.rows / 2);

        double angle = rand() % 11 - 5;;

        double scale = static_cast<double>(rand() % 10) / 100.0 + 0.9;

        rot_mat = cv::getRotationMatrix2D(center, angle, scale);

        cv::warpAffine(src, warp_rotate_dst, rot_mat, src.size());

        if (imshow_on) {
            cv::imshow("warp_rotate_window", warp_rotate_dst);
            cv::waitKey(5);
        }

        return warp_rotate_dst;
    }

    Rect VideoGeneration::GetRandomRect(const Mat &image, const Rect &rect, const vector<Rect> &existing_rects) {
        Mat mask;
        mask = Mat::zeros(image.size(), image.type());

        int x_pos = rand() % (image.cols - rect.width);
        int y_pos = rand() % (image.rows - rect.height);

        Rect new_rect(x_pos, y_pos, rect.width, rect.height);

        cv::rectangle(mask, new_rect, Scalar(255, 255, 255), cv::FILLED);

        if (imshow_on) {
            cv::imshow("random mask", mask);
            cv::waitKey(5);
        }

        return new_rect;
    }


    Rect VideoGeneration::StepRectThroughFrame(const Mat &image, const Rect &rect,
                                               int x_step, int y_step, const vector<Rect> &existing_rects) {
        Mat mask;
        mask = Mat::zeros(image.size(), image.type());

        int x_pos = rect.x + x_step;
        int y_pos = rect.y + y_step;

        Rect new_rect(x_pos, y_pos, rect.width, rect.height);

        if (new_rect.x + new_rect.width >= image.cols ||
            new_rect.y + new_rect.height >= image.rows) {
            return Rect(0, 0, 0, 0);
        }
        else {
            cv::rectangle(mask, new_rect, Scalar(255, 255, 255), cv::FILLED);
            if (imshow_on) {
                cv::imshow("random mask", mask);
                cv::waitKey(5);
            }
        }

        return new_rect;
    }


    int VideoGeneration::GetCompletedTracksCount(int current_frame_index, const vector<MPFVideoTrack> &tracks) {
        int tracks_completed_count = 0;
        for (vector<MPFVideoTrack>::const_iterator track = tracks.begin();
             track != tracks.end(); track++) {
            if (current_frame_index > track->stop_frame) {
                tracks_completed_count++;
            }
        }
        return tracks_completed_count;
    }

    int VideoGeneration::WriteTrackOutputVideo(const string video_in_uri, const vector<MPFVideoTrack> &tracks,
                                               const string video_out_filepath) {
        VideoCapture input_video(video_in_uri);
        if (!input_video.isOpened()) {
            std::cout << "Could not open the input video: " << video_in_uri << std::endl;
            return -1;
        }

        Size size = Size(static_cast<int>(input_video.get(cv::CAP_PROP_FRAME_WIDTH)),
                         static_cast<int>(input_video.get(cv::CAP_PROP_FRAME_HEIGHT)));

        VideoWriter output_video;

        double fps = input_video.get(cv::CAP_PROP_FPS);
        output_video.open(video_out_filepath, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, size, true);

        if (!output_video.isOpened()) {
            std::cout << "Could not open the output video for write: " << video_out_filepath << std::endl;
            return -1;
        }

        Mat src;

        int frame_index = 0;
        for (;;) {
            input_video >> src;
            if (src.empty()) {
                break;
            }

            vector<int> track_indexes;
            vector<MPFImageLocation> rects = GetImageLocationsAtFrameIndex(frame_index, tracks, track_indexes);

            Utils::DrawText(src, frame_index);

            if (!track_indexes.empty()) {
                vector<MPFVideoTrack> tracks_to_draw;
                for (unsigned int i = 0; i < track_indexes.size(); i++) {
                    MPFVideoTrack track = MPFVideoTrack(tracks[track_indexes[i]]);
                    tracks_to_draw.push_back(track);
                }

                int tracks_completed_count = GetCompletedTracksCount(frame_index, tracks);
                Utils::DrawTracks(src, tracks_to_draw, rects, tracks_completed_count, track_indexes);
            }

            for (vector<MPFImageLocation>::iterator it = rects.begin(); it != rects.end(); it++) {
                Rect rect(it->x_left_upper, it->y_left_upper, it->width, it->height);
                cv::rectangle(src, rect, Scalar(0, 255, 0));
            }

            if (imshow_on) {
                cv::imshow("Output Video Creation", src);
                cv::waitKey(5);
            }

            output_video << src;

            frame_index++;
        }

        return 0;
    }

    int VideoGeneration::CreateTestVideoAndTrackOutput(const vector<Mat> &objects,
                                                       int video_length,
                                                       bool use_scaling_and_rotation,
                                                       const string video_out_filepath,
                                                       vector<MPFVideoTrack> &tracks) {
        if (objects.empty()) {
            return -1;
        }

        Size video_size = Size(1000, 750);
        VideoWriter output_video;
        double fps = 30;

        output_video.open(video_out_filepath, cv::VideoWriter::fourcc('M', 'J', 'P', 'G'), fps, video_size, true);
        if (!output_video.isOpened()) {
            std::cout << "Could not open the output video for write: " << video_out_filepath << std::endl;
            return -1;
        }

        Mat src;
        Mat blank_frame = Mat(video_size, CV_8UC3, Scalar(127, 127, 127));

        vector<Rect> random_rects;
        vector<Rect> stepped_rects;
        vector<MPFVideoTrack> current_tracks;

        for (unsigned int i = 0; i < objects.size(); i++) {
            Mat object = Mat(objects[i]);
            Rect rect = Rect(0, 0, object.cols, object.rows);

            Rect random_rect;

            int intersection_index = -1;
            do {
                random_rect = GetRandomRect(blank_frame, rect);
            } while (Utils::IsExistingRectIntersection(random_rect, stepped_rects, intersection_index));

            random_rects.push_back(random_rect);
            stepped_rects.push_back(random_rect);

            MPFVideoTrack track;
            track.start_frame = 0;

            MPFImageLocation detection(random_rect.x, random_rect.y, random_rect.width, random_rect.height);
            track.frame_locations[0] = detection;
            current_tracks.push_back(track);
        }

        int frame_index = 0;
        for (; frame_index < video_length;) {
            src = blank_frame.clone();

            for (unsigned int i = 0; i < random_rects.size(); i++) {
                if (frame_index > 0) {
                    int step_x = rand() % 3 + 0;
                    int step_y = rand() % 2 + 0;
                    stepped_rects[i] = StepRectThroughFrame(src, stepped_rects[i], step_x, step_y, stepped_rects);
                }

                if (stepped_rects[i].area() == 0) {
                    current_tracks[i].stop_frame = frame_index;
                    tracks.push_back(current_tracks[i]);
                    current_tracks[i].start_frame = frame_index;
                    current_tracks[i].stop_frame = -1;
                    current_tracks[i].frame_locations.clear();

                    Mat object = Mat(objects[i]);
                    Rect rect = Rect(0, 0, object.cols, object.rows);
                    Rect random_rect;

                    int intersection_index = -1;
                    do {
                        random_rect = GetRandomRect(blank_frame, rect);
                    } while (Utils::IsExistingRectIntersection(random_rect, stepped_rects, intersection_index));

                    stepped_rects[i] = random_rect;
                }

                MPFImageLocation detection(stepped_rects[i].x,
                                                stepped_rects[i].y,
                                                stepped_rects[i].width,
                                                stepped_rects[i].height);
                current_tracks[i].frame_locations[frame_index] = detection;

                Mat subview = src(stepped_rects[i]);
                Mat object = Mat(objects[i]);
                if (use_scaling_and_rotation) {
                    object = Rotate(object);
                }
                object.copyTo(subview);
            }

            Utils::DrawText(src, frame_index);

            if (imshow_on) {
                cv::imshow("src", src);
                cv::waitKey(5);
            }

            output_video.write(src);

            frame_index++;
        }

        output_video.release();
        for (vector<MPFVideoTrack>::iterator iter = current_tracks.begin(); iter != current_tracks.end(); iter++) {
            iter->stop_frame = frame_index - 1;
            tracks.push_back(*iter);
        }

        return 0;
    }

}}