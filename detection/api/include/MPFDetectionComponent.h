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



#ifndef OPENMPF_CPP_COMPONENT_SDK_DETECTION_BASE_H
#define OPENMPF_CPP_COMPONENT_SDK_DETECTION_BASE_H

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "MPFComponentInterface.h"
#include "MPFDetectionObjects.h"

namespace MPF { namespace COMPONENT {

    struct MPFJob {
        const std::string job_name;
        const std::string data_uri;
        const Properties job_properties;
        const Properties media_properties;

    protected:
        MPFJob(std::string job_name,
               std::string data_uri,
               Properties job_properties,
               Properties media_properties)
            : job_name(std::move(job_name))
            , data_uri(std::move(data_uri))
            , job_properties(std::move(job_properties))
            , media_properties(std::move(media_properties)) {
        }
    };


    struct MPFVideoJob : MPFJob {
        const int start_frame;
        const int stop_frame;
        const bool has_feed_forward_track;
        const MPFVideoTrack feed_forward_track;

        MPFVideoJob(std::string job_name,
                    std::string data_uri,
                    int start_frame,
                    int stop_frame,
                    Properties job_properties,
                    Properties media_properties)
                : MPFJob(std::move(job_name),
                         std::move(data_uri),
                         std::move(job_properties),
                         std::move(media_properties))
                , start_frame(start_frame)
                , stop_frame(stop_frame)
                , has_feed_forward_track(false) {
        }

        MPFVideoJob(std::string job_name,
                    std::string data_uri,
                    int start_frame,
                    int stop_frame,
                    MPFVideoTrack track,
                    Properties job_properties,
                    Properties media_properties)
                : MPFJob(std::move(job_name),
                         std::move(data_uri),
                         std::move(job_properties),
                         std::move(media_properties))
                , start_frame(start_frame)
                , stop_frame(stop_frame)
                , has_feed_forward_track(true)
                , feed_forward_track(std::move(track)) {
        }
    };


    struct MPFImageJob : MPFJob {
        const bool has_feed_forward_location;
        const MPFImageLocation feed_forward_location;

        MPFImageJob(std::string job_name,
                    std::string data_uri,
                    Properties job_properties,
                    Properties media_properties)
                : MPFJob(std::move(job_name),
                         std::move(data_uri),
                         std::move(job_properties),
                         std::move(media_properties))
                , has_feed_forward_location(false) {
        }

        MPFImageJob(std::string job_name,
                    std::string data_uri,
                    MPFImageLocation location,
                    Properties job_properties,
                    Properties media_properties)
                : MPFJob(std::move(job_name),
                         std::move(data_uri),
                         std::move(job_properties),
                         std::move(media_properties))
                , has_feed_forward_location(true)
                , feed_forward_location(std::move(location)) {
        }
    };


    struct MPFAudioJob : MPFJob {
        const int start_time;
        const int stop_time;
        const bool has_feed_forward_track;
        const MPFAudioTrack feed_forward_track;

        MPFAudioJob(std::string job_name,
                    std::string data_uri,
                    int start_time,
                    int stop_time,
                    Properties job_properties,
                    Properties media_properties)
                : MPFJob(std::move(job_name),
                         std::move(data_uri),
                         std::move(job_properties),
                         std::move(media_properties))
                , start_time(start_time)
                , stop_time(stop_time)
                , has_feed_forward_track(false) {
        }

        MPFAudioJob(std::string job_name,
                    std::string data_uri,
                    int start_time,
                    int stop_time,
                    MPFAudioTrack track,
                    Properties job_properties,
                    Properties media_properties)
                : MPFJob(std::move(job_name),
                         std::move(data_uri),
                         std::move(job_properties),
                         std::move(media_properties))
                , start_time(start_time)
                , stop_time(stop_time)
                , has_feed_forward_track(true)
                , feed_forward_track(std::move(track)) {
        }
    };


    struct MPFGenericJob : MPFJob {
        const bool has_feed_forward_track;
        const MPFGenericTrack feed_forward_track;

        MPFGenericJob(std::string job_name,
                      std::string data_uri,
                      Properties job_properties,
                      Properties media_properties)
                : MPFJob(std::move(job_name),
                         std::move(data_uri),
                         std::move(job_properties),
                         std::move(media_properties))
                , has_feed_forward_track(false) {
        }

        MPFGenericJob(std::string job_name,
                      std::string data_uri,
                      MPFGenericTrack track,
                      Properties job_properties,
                      Properties media_properties)
                : MPFJob(std::move(job_name),
                         std::move(data_uri),
                         std::move(job_properties),
                         std::move(media_properties))
                , has_feed_forward_track(true)
                , feed_forward_track(std::move(track)) {
        }
    };


// Class used for batch processing detection jobs, any type of media.
    class MPFDetectionComponent : public MPFComponent {

    public:

        virtual std::vector<MPFVideoTrack> GetDetections(const MPFVideoJob &job) = 0;

        virtual std::vector<MPFImageLocation> GetDetections(const MPFImageJob &job) = 0;

        virtual std::vector<MPFAudioTrack> GetDetections(const MPFAudioJob &job) = 0;

        virtual std::vector<MPFGenericTrack> GetDetections(const MPFGenericJob &job) = 0;

        virtual bool Supports(MPFDetectionDataType data_type) = 0;

        virtual std::string GetDetectionType() = 0;

        MPFComponentType GetComponentType() override { return MPF_DETECTION_COMPONENT; };

    protected:

        MPFDetectionComponent() = default;
    };
}}


#endif //OPENMPF_CPP_COMPONENT_SDK_DETECTION_BASE_H
