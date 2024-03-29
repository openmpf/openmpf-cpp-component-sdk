/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2023 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2023 The MITRE Corporation                                       *
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

#include <string>
#include <utility>

#include "frame_transformers/FrameCropper.h"


namespace MPF { namespace COMPONENT {

    FrameCropper::FrameCropper(IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform)) {
    }


    void FrameCropper::DoFrameTransform(cv::Mat &frame, int frameIndex) const {
        frame = frame(GetRegionOfInterest(frameIndex));
    }


    void FrameCropper::DoReverseTransform(MPFImageLocation &imageLocation, int frameIndex) const {
        const cv::Rect roi = GetRegionOfInterest(frameIndex);
        imageLocation.x_left_upper += roi.x;
        imageLocation.y_left_upper += roi.y;
    }


    cv::Size FrameCropper::GetFrameSize(int frameIndex) const {
        return GetRegionOfInterest(frameIndex).size();
    }

    cv::Rect FrameCropper::GetIntersectingRegion(
            const cv::Rect &regionOfInterest, int frameIndex) const {
        cv::Rect frameRect(cv::Point(0, 0), GetInnerFrameSize(frameIndex));
        return regionOfInterest & frameRect;
    }


    SearchRegionFrameCropper::SearchRegionFrameCropper(const cv::Rect &regionOfInterest,
                                                       IFrameTransformer::Ptr innerTransform)
        : FrameCropper(std::move(innerTransform))
        , searchRegion_(GetIntersectingRegion(regionOfInterest, 0)) {
    }


    cv::Rect SearchRegionFrameCropper::GetRegionOfInterest(int frameIndex) const {
        return searchRegion_;
    }




    FeedForwardFrameCropper::FeedForwardFrameCropper(const std::map<int, MPFImageLocation> &track,
                                                     IFrameTransformer::Ptr innerTransform)
        : FrameCropper(std::move(innerTransform)) {

        fedForwardDetections_.reserve(track.size());

        for (const auto &frameLocationPair : track) {
            const auto &frameLocation = frameLocationPair.second;
            cv::Rect roi(frameLocation.x_left_upper, frameLocation.y_left_upper,
                         frameLocation.width, frameLocation.height);
            fedForwardDetections_.emplace_back(GetIntersectingRegion(roi, frameLocationPair.first));
        }
        fedForwardDetections_.shrink_to_fit();
    }


    cv::Rect FeedForwardFrameCropper::GetRegionOfInterest(int frameIndex) const {
        try {
            return fedForwardDetections_.at(frameIndex);
        }
        catch (const std::out_of_range &error) {
            std::stringstream ss;
            ss << "Attempted to get feed forward region of interest for frame: " << frameIndex
               << ", but there are only " << fedForwardDetections_.size() << " entries in the feed forward track.";
            throw std::out_of_range(ss.str());
        }
    }
}}
