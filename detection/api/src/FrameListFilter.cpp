/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2019 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2019 The MITRE Corporation                                       *
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
#include <sstream>

#include "FrameListFilter.h"

namespace MPF { namespace COMPONENT {

    FrameListFilter::FrameListFilter(std::vector<int> &&framesToShow)
        : framesToShow_(std::move(framesToShow)) {
    }


    int FrameListFilter::SegmentToOriginalFramePosition(int segmentPosition) const {
        try {
            return framesToShow_.at(static_cast<size_t>(segmentPosition));
        }
        catch (const std::out_of_range &error) {
            std::stringstream ss;
            ss << "Attempted to get the original position for segment position: "
               << segmentPosition << ", but the maximum segment position is " << GetSegmentFrameCount() - 1;
            throw std::out_of_range(ss.str());
        }
    }


    int FrameListFilter::OriginalToSegmentFramePosition(int originalPosition) const {
        // Use binary search to get index of original position
        auto iter = std::lower_bound(framesToShow_.begin(), framesToShow_.end(), originalPosition);
        if (iter == framesToShow_.end()) {
            return GetSegmentFrameCount();
        }
        return static_cast<int>(iter - framesToShow_.begin());
    }


    int FrameListFilter::GetSegmentFrameCount() const {
        return static_cast<int>(framesToShow_.size());
    }


    double FrameListFilter::GetSegmentDuration(double originalFrameRate) const {
        int range = framesToShow_.back() - framesToShow_.front() + 1;
        return range / originalFrameRate;
    }


    int FrameListFilter::GetAvailableInitializationFrameCount() const {
        return 0;
    }
}}