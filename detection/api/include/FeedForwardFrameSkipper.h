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


#ifndef OPENMPF_CPP_COMPONENT_SDK_FEEDFORWARDFRAMESKIPPER_H
#define OPENMPF_CPP_COMPONENT_SDK_FEEDFORWARDFRAMESKIPPER_H


#include <vector>

#include "MPFDetectionComponent.h"
#include "FrameSkipper.h"

namespace MPF { namespace COMPONENT {

    class FeedForwardFrameSkipper : public FrameSkipper {
    public:
        explicit FeedForwardFrameSkipper(const MPFVideoTrack &feed_forward_track);

        int SegmentToOriginalFramePosition(int segmentPosition) const override;

        int OriginalToSegmentFramePosition(int originalPosition) const override;

        int GetSegmentFrameCount() const override;

        double GetSegmentDuration(double originalFrameRate) const override;

        int GetAvailableInitializationFrameCount() const override;

    private:
        const std::vector<int> framesInTrack_;

        static std::vector<int> GetFramesInTrack(const MPFVideoTrack &track);
    };

}}


#endif //OPENMPF_CPP_COMPONENT_SDK_FEEDFORWARDFRAMESKIPPER_H
