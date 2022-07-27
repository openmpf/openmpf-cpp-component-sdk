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

#ifndef OPENMPF_CPP_COMPONENT_SDK_UTILS_H
#define OPENMPF_CPP_COMPONENT_SDK_UTILS_H

#include <vector>

#include <log4cxx/logger.h>

#include <opencv2/core.hpp>

#include "MPFDetectionComponent.h"
#include "detectionComponentUtils.h"


namespace MPF { namespace COMPONENT { namespace Utils {

    [[noreturn]] void LogAndReThrowException(const MPFVideoJob &job, const log4cxx::LoggerPtr &logger);

    [[noreturn]] void LogAndReThrowException(const MPFImageJob &job, const log4cxx::LoggerPtr &logger);

    [[noreturn]] void LogAndReThrowException(const MPFAudioJob &job, const log4cxx::LoggerPtr &logger);

    [[noreturn]] void LogAndReThrowException(const MPFGenericJob &job, const log4cxx::LoggerPtr &logger);

    // This function performs shell-like file name expansion. It
    // returns an error string. If that string is not empty, then the
    // filename expansion failed and the exp_filename output should
    // not be used.
    std::string expandFileName(const std::string &filename, std::string &exp_filename);

    void trim(std::string &str);

    cv::Mat ConvertToGray(const cv::Mat &image);


    cv::Rect ImageLocationToCvRect(const MPFImageLocation &image_location);

    MPFImageLocation CvRectToImageLocation(const cv::Rect &rect, float confidence = -1.0f);

    bool IsExistingRectIntersection(const cv::Rect new_rect,
                                    const std::vector<cv::Rect> &existing_rects,
                                    int &intersection_index);

    void DrawText(cv::Mat &image, int frame_index);

    void DrawTracks(cv::Mat &image,
                    const std::vector<MPFVideoTrack> &tracks_to_draw,
                    const std::vector<MPFImageLocation> &locations,
                    int tracks_completed_count,
                    std::vector<int> track_indexes = {});

}}}


#endif //OPENMPF_CPP_COMPONENT_SDK_UTILS_H
