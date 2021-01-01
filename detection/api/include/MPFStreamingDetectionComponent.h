/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2021 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2021 The MITRE Corporation                                       *
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


#ifndef OPENMPF_CPP_COMPONENT_SDK_MPFSTREAMINGDETECTIONCOMPONENT_H
#define OPENMPF_CPP_COMPONENT_SDK_MPFSTREAMINGDETECTIONCOMPONENT_H

#include <map>
#include <string>
#include <vector>
#include <opencv2/core.hpp>

#include "MPFDetectionObjects.h"


namespace MPF { namespace COMPONENT {

    struct MPFStreamingVideoJob {
        const std::string job_name;
        const std::string run_directory;
        const Properties job_properties;
        const Properties media_properties;

        MPFStreamingVideoJob(
                const std::string &job_name,
                const std::string &run_directory,
                const Properties &job_properties,
                const Properties &media_properties)
                : job_name(job_name)
                , run_directory(run_directory)
                , job_properties(job_properties)
                , media_properties(media_properties) {
        }
    };


    struct VideoSegmentInfo {
        int segment_number;
        int start_frame;
        int end_frame;

        int frame_width;
        int frame_height;

        VideoSegmentInfo(int segment_number, int start_frame, int end_frame, int frame_width, int frame_height)
                : segment_number(segment_number)
                , start_frame(start_frame)
                , end_frame(end_frame)
                , frame_width(frame_width)
                , frame_height(frame_height) {
        }
    };


    class MPFStreamingDetectionComponent {

    public:
        virtual ~MPFStreamingDetectionComponent() = default;

        virtual std::string GetDetectionType() = 0;

        // Optional
        virtual void BeginSegment(const VideoSegmentInfo &segment_info) { };

        virtual bool ProcessFrame(const cv::Mat &frame, int frame_number) = 0;

        virtual std::vector<MPFVideoTrack> EndSegment() = 0;

    protected:
        explicit MPFStreamingDetectionComponent(const MPFStreamingVideoJob &job) { };
    };
}}


#define EXPORT_MPF_STREAMING_COMPONENT(name) \
    extern "C" MPF::COMPONENT::MPFStreamingDetectionComponent* streaming_component_creator(const MPF::COMPONENT::MPFStreamingVideoJob *job) { \
          return new (name)(*job);      \
    } \
    \
    extern "C" void streaming_component_deleter(MPF::COMPONENT::MPFStreamingDetectionComponent *component) { \
            delete component; \
    }

#endif //OPENMPF_CPP_COMPONENT_SDK_MPFSTREAMINGDETECTIONCOMPONENT_H
