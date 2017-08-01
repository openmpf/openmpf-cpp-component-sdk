/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2016 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2016 The MITRE Corporation                                       *
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


#ifndef OPENMPF_CPP_COMPONENT_SDK_FRAMESKIPPER_H
#define OPENMPF_CPP_COMPONENT_SDK_FRAMESKIPPER_H


#include "MPFDetectionComponent.h"

namespace MPF { namespace COMPONENT {

    /**
     * Handles calculations needed to support frame skipping.
     * Frame skipping is based on start frame, stop frame, and frame interval.
     */
    class FrameSkipper {

    public:
        FrameSkipper(int startFrame, int stopFrame, int frameInterval);

        FrameSkipper(const MPFVideoJob &job, int originalFrameCount);

        int SegmentToOriginalFramePosition(int segmentPosition) const;

        int OriginalToSegmentFramePosition(int originalPosition) const;

        bool TryGetNextFrame(int prevFrame, int &nextFrame) const;

        int GetFrameCount() const;

        double GetSegmentDuration(double originalFrameRate) const;

        double GetSegmentFrameRate(double originalFrameRate) const;

        double GetCurrentTimeInMillis(int originalPosition, double originalFrameRate) const;

        bool TryGetFramePositionForMillis(double originalFrameRate, double milliseconds, int& originalFramePos) const;

        double GetPositionRatio(int originalPosition) const;

        int GetFramePositionForRatio(double ratio) const;

        void GetAvailableInitializationFrames(int numberOfRequestedFrames, int &firstInitializationFrame,
                                              int &numberOfInitializationFramesAvailable) const;



    private:
        const int startFrame_;
        const int stopFrame_;
        const int frameInterval_;

        static int GetFrameInterval(const MPFJob &job);
        static int GetStopFrame(const MPFVideoJob &job, int originalFrameCount);

    };

}}


#endif //OPENMPF_CPP_COMPONENT_SDK_FRAMESKIPPER_H
