/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2024 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2024 The MITRE Corporation                                       *
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
#include "FeedForwardFrameFilter.h"


namespace MPF { namespace COMPONENT {

    FeedForwardFrameFilter::FeedForwardFrameFilter(const MPFVideoTrack &feedForwardTrack)
            : FrameListFilter(GetFramesInTrack(feedForwardTrack)) {
    }


    std::vector<int> FeedForwardFrameFilter::GetFramesInTrack(const MPFVideoTrack &track) {
        std::vector<int> framesInTrack;
        framesInTrack.reserve(track.frame_locations.size());

        for (const auto &frameLocPair : track.frame_locations) {
            framesInTrack.push_back(frameLocPair.first);
        }

        framesInTrack.shrink_to_fit();
        return framesInTrack;
    }


}}

