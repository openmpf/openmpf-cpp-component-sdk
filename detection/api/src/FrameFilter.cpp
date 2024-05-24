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


#include "IntervalFrameFilter.h"
#include "FrameFilter.h"


namespace MPF { namespace COMPONENT {

    bool FrameFilter::IsPastEndOfSegment(int originalPosition) const {
        int lastSegmentPos = GetSegmentFrameCount() - 1;
        int lastOriginalPos = SegmentToOriginalFramePosition(lastSegmentPos);
        return originalPosition > lastOriginalPos;
    }


    double FrameFilter::GetSegmentFrameRate(double originalFrameRate) const {
        return GetSegmentFrameCount() / GetSegmentDuration(originalFrameRate);
    }


    double FrameFilter::GetCurrentSegmentTimeInMillis(int originalPosition, double originalFrameRate) const {
        int segmentPos = OriginalToSegmentFramePosition(originalPosition);
        double framesPerSecond = GetSegmentFrameRate(originalFrameRate);
        double timeInSeconds = segmentPos / framesPerSecond;
        return timeInSeconds * 1000;
    }


    int FrameFilter::MillisToSegmentFramePosition(double originalFrameRate, double segmentMilliseconds) const {
        double segmentFps = GetSegmentFrameRate(originalFrameRate);
        return static_cast<int>(segmentFps * segmentMilliseconds / 1000);
    }

    double FrameFilter::GetSegmentFramePositionRatio(int originalPosition) const {
        double segmentPosition = OriginalToSegmentFramePosition(originalPosition);
        return segmentPosition / GetSegmentFrameCount();
    }

    int FrameFilter::RatioToOriginalFramePosition(double ratio) const {
        auto segmentPosition = static_cast<int>(GetSegmentFrameCount() * ratio);
        return SegmentToOriginalFramePosition(segmentPosition);
    }

    FrameFilter::CPtr FrameFilter::GetNoOpFilter(int frameCount) {
        return CPtr(new IntervalFrameFilter(0, frameCount - 1, 1));
    }

}}
