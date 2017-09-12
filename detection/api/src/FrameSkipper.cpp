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


#include "IntervalFrameSkipper.h"
#include "FrameSkipper.h"


namespace MPF { namespace COMPONENT {

    bool FrameSkipper::IsPastEndOfSegment(int originalPosition) const {
        int lastSegmentPos = GetSegmentFrameCount() - 1;
        int lastOriginalPos = SegmentToOriginalFramePosition(lastSegmentPos);
        return originalPosition > lastOriginalPos;
    }


    double FrameSkipper::GetSegmentFrameRate(double originalFrameRate) const {
        return GetSegmentFrameCount() / GetSegmentDuration(originalFrameRate);
    }


    double FrameSkipper::GetCurrentSegmentTimeInMillis(int originalPosition, double originalFrameRate) const {
        int segmentPos = OriginalToSegmentFramePosition(originalPosition);
        double framesPerSecond = GetSegmentFrameRate(originalFrameRate);
        double timeInSeconds = segmentPos / framesPerSecond;
        return timeInSeconds * 1000;
    }


    int FrameSkipper::MillisToSegmentFramePosition(double originalFrameRate, double segmentMilliseconds) const {
        double segmentFps = GetSegmentFrameRate(originalFrameRate);
        return static_cast<int>(segmentFps * segmentMilliseconds / 1000);
    }

    double FrameSkipper::GetSegmentFramePositionRatio(int originalPosition) const {
        double segmentPosition = OriginalToSegmentFramePosition(originalPosition);
        return segmentPosition / GetSegmentFrameCount();
    }

    int FrameSkipper::RatioToOriginalFramePosition(double ratio) const {
        auto segmentPosition = static_cast<int>(GetSegmentFrameCount() * ratio);
        return SegmentToOriginalFramePosition(segmentPosition);
    }

    FrameSkipper::CPtr FrameSkipper::GetNoOpSkipper(int frameCount) {
        return CPtr(new IntervalFrameSkipper(0, frameCount - 1, 1));
    }

}}
