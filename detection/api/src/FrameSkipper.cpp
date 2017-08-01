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


#include "FrameSkipper.h"
#include "detectionComponentUtils.h"
#include <stdexcept>

namespace MPF { namespace COMPONENT {

    FrameSkipper::FrameSkipper(const MPFVideoJob &job, int originalFrameCount)
            : FrameSkipper(job.start_frame, GetStopFrame(job, originalFrameCount), GetFrameInterval(job)) {

    }

    FrameSkipper::FrameSkipper(int startFrame, int stopFrame, int frameInterval)
            : startFrame_(startFrame)
            , stopFrame_(stopFrame)
            , frameInterval_(frameInterval)
    {

    }


    bool FrameSkipper::TryGetNextFrame(int prevFrame, int &nextFrame) const {
        if (prevFrame > stopFrame_) {
            nextFrame = -1;
            return false;
        }
        int segPos = OriginalToSegmentFramePosition(prevFrame);
        nextFrame = SegmentToOriginalFramePosition(segPos + 1);
        return true;
    }


    int FrameSkipper::SegmentToOriginalFramePosition(int segmentPosition) const {
        return frameInterval_ * segmentPosition + startFrame_;
    }

    int FrameSkipper::OriginalToSegmentFramePosition(int originalPosition) const {
        return (originalPosition - startFrame_) / frameInterval_;
    }


    int FrameSkipper::GetFrameCount() const {
        int range = stopFrame_ - startFrame_ + 1;
        int fullSegments = range / frameInterval_;
        bool hasRemainder = range % frameInterval_ != 0;
        if (hasRemainder) {
            return fullSegments + 1;
        }
        return fullSegments;
    }


    double FrameSkipper::GetSegmentDuration(double originalFrameRate) const {
        int range = stopFrame_ - startFrame_ + 1;
        return range / originalFrameRate;
    }

    double FrameSkipper::GetSegmentFrameRate(double originalFrameRate) const {
        return GetFrameCount() / GetSegmentDuration(originalFrameRate);
    }

    double FrameSkipper::GetCurrentTimeInMillis(int originalPosition, double originalFrameRate) const {
        int segmentPos = OriginalToSegmentFramePosition(originalPosition);
        double framesPerSecond = GetSegmentFrameRate(originalFrameRate);
        double timeInSeconds = segmentPos / framesPerSecond;
        return timeInSeconds * 1000;
    }



    bool FrameSkipper::TryGetFramePositionForMillis(double originalFrameRate,
                                                    double milliseconds,
                                                    int &originalFramePos) const {
        originalFramePos = -1;
        if (milliseconds < 0) {
            return false;
        }

        double segmentFps = GetSegmentFrameRate(originalFrameRate);
        int segmentPosition = (int) (segmentFps * milliseconds / 1000);
        if (segmentPosition < GetFrameCount()) {
            originalFramePos = SegmentToOriginalFramePosition(segmentPosition);
            return true;
        }

        return false;
    }


    double FrameSkipper::GetPositionRatio(int originalPosition) const {
        double segmentPosition = OriginalToSegmentFramePosition(originalPosition);
        return segmentPosition / GetFrameCount();
    }


    int FrameSkipper::GetFramePositionForRatio(double ratio) const {
        int segmentPosition = (int) (GetFrameCount() * ratio);
        return SegmentToOriginalFramePosition(segmentPosition);
    }


    int FrameSkipper::GetFrameInterval(const MPFJob &job) {
        int interval = DetectionComponentUtils::GetProperty(job.job_properties, "FRAME_INTERVAL", 1);
        return interval > 0
               ? interval
               : 1;
    }

    int FrameSkipper::GetStopFrame(const MPFVideoJob &job, int originalFrameCount) {
        if (job.stop_frame < originalFrameCount) {
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

    void FrameSkipper::GetAvailableInitializationFrames(int numberOfRequestedFrames,
                                                        int &firstInitializationFrame,
                                                        int &numberOfInitializationFramesAvailable) const {

        numberOfInitializationFramesAvailable = std::min(startFrame_, numberOfRequestedFrames);
        firstInitializationFrame = startFrame_ - numberOfInitializationFramesAvailable;

    }


}}