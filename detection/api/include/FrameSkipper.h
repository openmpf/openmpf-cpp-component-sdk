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
     * If a video file is long enough, the Workflow Manager will create multiple jobs, each with different start
     * and stop frames. Additionally many components support a FRAME_INTERVAL property.
     * These values tell components to only process certain frames in the video. Instead of having components
     * figure out which frames process and which frames to skip, this class performs the calculations necessary
     * to filter out frames that shouldn't be processed. From the component's point of view, it is processing
     * the entire video, but it is really only processing a particular segment of the video.
     */
    class FrameSkipper {

    public:
        FrameSkipper(int startFrame, int stopFrame, int frameInterval);

        FrameSkipper(const MPFVideoJob &job, int originalFrameCount);


        /**
         * Map a frame position within the segment to a frame position in the original video.
         * @param segmentPosition A frame position within the segment
         * @return The matching frame position in the original video.
         */
        int SegmentToOriginalFramePosition(int segmentPosition) const;


        /**
         * Map a frame position in the original video, to a position in the segment
         * @param originalPosition  A frame position in the original video
         * @return The matching frame position in the segment
         */
        int OriginalToSegmentFramePosition(int originalPosition) const;


        bool IsPastEndOfSegment(int originalPosition) const;


        /**
         * @return The number of frames in the segment
         */
        int GetSegmentFrameCount() const;


        /**
         * Gets the amount of time from the segment start to the segment end. The duration of the segment
         * is not affected by the frame interval, instead the frame rate is adjusted so that the time
         * from the start frame to the stop frame is the same as the original video.
         * @param originalFrameRate Frame rate of original video
         * @return The duration of the segment in seconds
         */
        double GetSegmentDuration(double originalFrameRate) const;


        /**
         * Gets the frame rate of the segment. The frame rate is calculated so that regardless of the frame interval,
         * the duration between the start frame and stop frame is the same as the original video.
         * @param originalFrameRate Frame rate of original video
         * @return Frames per second of the video segment
         */
        double GetSegmentFrameRate(double originalFrameRate) const;


        /**
         * Gets the time in milliseconds between the segment start frame and the current position.
         * @param originalPosition Frame position in original video
         * @param originalFrameRate Frame rate of original video
         * @return Time in milliseconds since the segment started
         */
        double GetCurrentSegmentTimeInMillis(int originalPosition, double originalFrameRate) const;


        /**
         * Gets the segment position that is the specified number of milliseconds since the start of the segment
         * @param originalFrameRate Frame position in original video
         * @param segmentMilliseconds Time since start of segment in milliseconds
         * @return Segment frame position for the specified number of milliseconds
         */
        int MillisToSegmentFramePosition(double originalFrameRate, double segmentMilliseconds) const;


        /**
         * Returns a number between 0 (start of video) and 1 (end of video) that indicates the current position
         * in the video
         * @param originalPosition Frame position in original video
         * @return Number between 0 and 1 indicating current position in video
         */
        double GetSegmentFramePositionRatio(int originalPosition) const;


        /**
         * Returns the position in the original video for the given ratio
         * @param ratio Number between 0 and 1 that indicates position in video
         * @return Position in the original video
         */
        int RatioToOriginalFramePosition(double ratio) const;


        /**
         * Returns the number of frames before the beginning of the segment.
         * Frames that are skipped due to the frame interval are not counted.
         * @return Number of available initialization frames
         */
        int GetAvailableInitializationFrameCount() const;



    private:
        const int startFrame_;
        const int stopFrame_;
        const int frameInterval_;

        static int GetFrameInterval(const MPFJob &job);
        static int GetStopFrame(const MPFVideoJob &job, int originalFrameCount);

    };

}}


#endif //OPENMPF_CPP_COMPONENT_SDK_FRAMESKIPPER_H
