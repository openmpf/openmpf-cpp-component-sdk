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


#ifndef OPENMPF_CPP_COMPONENT_SDK_VIDEOGENERATION_H
#define OPENMPF_CPP_COMPONENT_SDK_VIDEOGENERATION_H


#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "MPFDetectionComponent.h"

namespace MPF { namespace COMPONENT {

    class VideoGeneration {
    public:
        VideoGeneration(bool display_window = false);


        std::vector<MPFImageLocation>
        GetImageLocationsAtFrameIndex(int frame_index, const std::vector<MPFVideoTrack> &tracks,
                                      std::vector<int> &track_indexes);


        int WriteTrackOutputVideo(const std::string video_in_uri, const std::vector<MPFVideoTrack> &tracks,
                                  const std::string video_out_filepath);

        cv::Mat RotateFace(const cv::Mat &src);

        cv::Rect GetRandomRect(const cv::Mat &image,
                               const cv::Rect &face_rect,
                               const std::vector<cv::Rect> &existing_rects = {});

        cv::Rect StepRectThroughFrame(const cv::Mat &image,
                                      const cv::Rect &rect,
                                      int x_step,
                                      int y_step,
                                      const std::vector<cv::Rect> &existing_rects = {});

        int CreateTestVideoAndTrackOutput(const std::vector<cv::Mat> &faces,
                                          int video_length,
                                          bool use_scaling_and_rotation,
                                          const std::string video_out_filepath,
                                          std::vector<MPFVideoTrack> &tracks);

    private:
        bool imshow_on;

        static int GetCompletedTracksCount(int current_frame_index, const std::vector<MPFVideoTrack> &tracks);
    };

}}


#endif //OPENMPF_CPP_COMPONENT_SDK_VIDEOGENERATION_H
