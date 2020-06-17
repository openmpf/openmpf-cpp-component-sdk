/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2020 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2020 The MITRE Corporation                                       *
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


#ifndef OPENMPF_CPP_COMPONENT_SDK_INTERVAL_FRAMEFILTER_H
#define OPENMPF_CPP_COMPONENT_SDK_INTERVAL_FRAMEFILTER_H


#include "FrameFilter.h"
#include "MPFDetectionComponent.h"

namespace MPF { namespace COMPONENT {

    /**
     * If a video file is long enough, the Workflow Manager will create multiple jobs, each with different start
     * and stop frames. Additionally many components support a FRAME_INTERVAL property.
     * These values tell components to only process certain frames in the video. Instead of having components
     * figure out which frames to process and which frames to skip, this class performs the calculations necessary
     * to filter out frames that shouldn't be processed. From the component's point of view, it is processing
     * the entire video, but it is really only processing a particular segment of the video.
     */
    class IntervalFrameFilter : public FrameFilter {

    public:
        IntervalFrameFilter(int startFrame, int stopFrame, int frameInterval);

        IntervalFrameFilter(const MPFVideoJob &job, int originalFrameCount);


        int SegmentToOriginalFramePosition(int segmentPosition) const override;

        int OriginalToSegmentFramePosition(int originalPosition) const override;

        int GetSegmentFrameCount() const override;

        double GetSegmentDuration(double originalFrameRate) const override;

        int GetAvailableInitializationFrameCount() const override;



    private:
        const int startFrame_;
        const int stopFrame_;
        const int frameInterval_;

        static int GetFrameInterval(const MPFJob &job);
        static int GetStopFrame(const MPFVideoJob &job, int originalFrameCount);
    };

}}


#endif //OPENMPF_CPP_COMPONENT_SDK_INTERVAL_FRAMEFILTER_H
