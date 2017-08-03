/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2017 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2017 The MITRE Corporation                                       *
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


#ifndef OPENMPF_CPP_COMPONENT_SDK_IMAGEGENERATION_H
#define OPENMPF_CPP_COMPONENT_SDK_IMAGEGENERATION_H


#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "MPFDetectionComponent.h"


namespace MPF { namespace COMPONENT {

    class ImageGeneration {
    public:
        ImageGeneration(bool imshow_on = false);


        int WriteDetectionOutputImage(const std::string image_in_uri,
                                      const std::vector<MPFImageLocation> &detections,
                                      const std::string image_out_filepath);

        int WriteDetectionOutputImage(const std::string image_in_uri,
                                      const std::vector<MPFVideoTrack> &detections,
                                      const std::string image_out_filepath);


        int CreateTestImageAndDetectionOutput(const std::vector<cv::Mat> &faces,
                                              bool use_scaling_and_rotation,
                                              const std::string image_out_filepath,
                                              std::vector<MPFImageLocation> &detections);

        cv::Mat RotateFace(const cv::Mat &src);

        cv::Rect GetRandomRect(const cv::Mat &image,
                               const cv::Rect &face_rect,
                               const std::vector<cv::Rect> &existing_rects = {});

    private:
        bool imshow_on;

    };

}}


#endif //OPENMPF_CPP_COMPONENT_SDK_IMAGEGENERATION_H
