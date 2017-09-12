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


#include <algorithm>
#include <iostream>
#include "FeedForwardFrameSkipper.h"


namespace MPF { namespace COMPONENT {

    FeedForwardFrameSkipper::FeedForwardFrameSkipper(const MPFVideoTrack &feedForwardTrack)
            : framesInTrack_(GetFramesInTrack(feedForwardTrack)) {
    }


    std::vector<int> FeedForwardFrameSkipper::GetFramesInTrack(const MPFVideoTrack &track) {
        std::vector<int> framesInTrack;
        framesInTrack.reserve(track.frame_locations.size());

        for (const auto &frameLocPair : track.frame_locations) {
            framesInTrack.push_back(frameLocPair.first);
        }

        framesInTrack.shrink_to_fit();
        return framesInTrack;
    }


    int FeedForwardFrameSkipper::SegmentToOriginalFramePosition(int segmentPosition) const {
        return framesInTrack_.at(static_cast<size_t>(segmentPosition));
    }


    int FeedForwardFrameSkipper::OriginalToSegmentFramePosition(int originalPosition) const {
        // Use binary search to get index of original position
        auto iter = std::lower_bound(framesInTrack_.begin(), framesInTrack_.end(), originalPosition);
        if (iter == framesInTrack_.end()) {
            return GetSegmentFrameCount();
        }
        return static_cast<int>(iter - framesInTrack_.begin());
    }


    int FeedForwardFrameSkipper::GetSegmentFrameCount() const {
        return static_cast<int>(framesInTrack_.size());
    }


    double FeedForwardFrameSkipper::GetSegmentDuration(double originalFrameRate) const {
        int range = framesInTrack_.back() - framesInTrack_.front() + 1;
        return range / originalFrameRate;
    }


    int FeedForwardFrameSkipper::GetAvailableInitializationFrameCount() const {
        return 0;
    }

}}

