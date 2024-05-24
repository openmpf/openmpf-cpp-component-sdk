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
#include "detectionComponentUtils.h"
#include <stdexcept>

namespace MPF { namespace COMPONENT {

    IntervalFrameFilter::IntervalFrameFilter(const MPFVideoJob &job, int originalFrameCount)
            : IntervalFrameFilter(job.start_frame, GetStopFrame(job, originalFrameCount), GetFrameInterval(job)) {

    }

    IntervalFrameFilter::IntervalFrameFilter(int startFrame, int stopFrame, int frameInterval)
            : startFrame_(startFrame)
            , stopFrame_(stopFrame)
            , frameInterval_(frameInterval) {

    }


    int IntervalFrameFilter::SegmentToOriginalFramePosition(int segmentPosition) const {
        return frameInterval_ * segmentPosition + startFrame_;
    }


    int IntervalFrameFilter::OriginalToSegmentFramePosition(int originalPosition) const {
        return (originalPosition - startFrame_) / frameInterval_;
    }


    int IntervalFrameFilter::GetSegmentFrameCount() const {
        int range = stopFrame_ - startFrame_ + 1;
        int fullSegments = range / frameInterval_;
        bool hasRemainder = range % frameInterval_ != 0;
        if (hasRemainder) {
            return fullSegments + 1;
        }
        return fullSegments;
    }


    double IntervalFrameFilter::GetSegmentDuration(double originalFrameRate) const {
        int range = stopFrame_ - startFrame_ + 1;
        return range / originalFrameRate;
    }


    int IntervalFrameFilter::GetAvailableInitializationFrameCount() const {
        return startFrame_ / frameInterval_;
    }


    int IntervalFrameFilter::GetFrameInterval(const MPFJob &job) {
        int interval = DetectionComponentUtils::GetProperty(job.job_properties, "FRAME_INTERVAL", 1);
        return interval > 0
               ? interval
               : 1;
    }


    int IntervalFrameFilter::GetStopFrame(const MPFVideoJob &job, int originalFrameCount) {
        if (job.stop_frame >= 0 && job.stop_frame < originalFrameCount) {
            return job.stop_frame;
        }

        int stopFrame  = originalFrameCount - 1;
        if (stopFrame < job.start_frame) {
            std::stringstream ss;
            ss << "Unable to handle segment: " << job.start_frame << " - " << job.stop_frame
               << " because original media only has " << originalFrameCount << " frames";
            throw std::range_error(ss.str());
        }
        return stopFrame;
    }


}}
