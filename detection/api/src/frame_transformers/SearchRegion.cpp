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

#include "frame_transformers/SearchRegion.h"

#include <utility>


namespace MPF { namespace COMPONENT {

    SearchRegion::SearchRegion()
        : SearchRegion(RegionEdge::Default(), RegionEdge::Default(), RegionEdge::Default(), RegionEdge::Default())
    { }

    SearchRegion::SearchRegion(resolve_region_edge_t left,
                               resolve_region_edge_t top,
                               resolve_region_edge_t right,
                               resolve_region_edge_t bottom)
            : left_(std::move(left))
            , top_(std::move(top))
            , right_(std::move(right))
            , bottom_(std::move(bottom))
    { }


    cv::Rect SearchRegion::GetRect(const cv::Size &frameSize) const {
        int leftValue = left_(0, frameSize.width);
        int topValue = top_(0, frameSize.height);
        int rightValue = right_(frameSize.width, frameSize.width);
        int bottomValue = bottom_(frameSize.height, frameSize.height);
        return cv::Rect(cv::Point(leftValue, topValue), cv::Point(rightValue, bottomValue));
    }



    namespace RegionEdge {

        resolve_region_edge_t Default() {
            return [](int defaultSize, int maxSize) {
                return defaultSize;
            };
        }

        resolve_region_edge_t Percentage(double percentage) {
            if (percentage < 0) {
                return Default();
            }

            return [percentage](int defaultSize, int maxSize) {
                if (percentage >= 100) {
                    return maxSize;
                }
                return static_cast<int>(percentage * maxSize / 100);
            };
        }

        resolve_region_edge_t Absolute(int value) {
            if (value < 0) {
                return Default();
            }

            return [value](int defaultSize, int maxSize) {
                return std::min(value, maxSize);
            };
        }
    }
}}