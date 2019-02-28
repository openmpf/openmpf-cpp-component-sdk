/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2019 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2019 The MITRE Corporation                                       *
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
#include <vector>
#include <opencv2/core.hpp>

#include "MPFComponentInterface.h"
#include "MPFDetectionObjects.h"

namespace MPF { namespace COMPONENT {

    struct MPFJob {
        const std::string job_name;
        const std::string data_uri;
        const Properties job_properties;
        const Properties media_properties;

    protected:
        MPFJob(const std::string &job_name,
               const std::string &data_uri,
               const Properties &job_properties,
               const Properties &media_properties)
                : job_name(job_name)
                , data_uri(data_uri)
                , job_properties(job_properties)
                , media_properties(media_properties) {
        }
    };


    struct MPFVideoJob : MPFJob {
        const int start_frame;
        const int stop_frame;
        bool has_feed_forward_track = false;
        MPFVideoTrack feed_forward_track;

        MPFVideoJob(const std::string &job_name,
                    const std::string &data_uri,
                    int start_frame,
                    int stop_frame,
                    const Properties &job_properties,
                    const Properties &media_properties)
                : MPFJob(job_name, data_uri, job_properties, media_properties)
                , start_frame(start_frame)
                , stop_frame(stop_frame)
                , has_feed_forward_track(false) {
        }

        MPFVideoJob(const std::string &job_name,
                    const std::string &data_uri,
                    int start_frame,
                    int stop_frame,
                    const MPFVideoTrack &track,
                    const Properties &job_properties,
                    const Properties &media_properties)
                : MPFJob(job_name, data_uri, job_properties, media_properties)
                , start_frame(start_frame)
                , stop_frame(stop_frame)
                , has_feed_forward_track(true)
                , feed_forward_track(track) {
        }
    };


    struct MPFImageJob : MPFJob {
        bool has_feed_forward_location = false;
        MPFImageLocation feed_forward_location;
        MPFImageJob(const std::string &job_name,
                    const std::string &data_uri,
                    const Properties &job_properties,
                    const Properties &media_properties)
                : MPFJob(job_name, data_uri, job_properties, media_properties)
                , has_feed_forward_location(false) {
        }

        MPFImageJob(const std::string &job_name,
                    const std::string &data_uri,
                    const MPFImageLocation &location,
                    const Properties &job_properties,
                    const Properties &media_properties)
                : MPFJob(job_name, data_uri, job_properties, media_properties)
                , has_feed_forward_location(true)
                , feed_forward_location(location) {
        }
    };


    struct MPFAudioJob : MPFJob {
        const int start_time;
        const int stop_time;
        bool has_feed_forward_track = false;
        MPFAudioTrack feed_forward_track;

        MPFAudioJob(const std::string &job_name,
                    const std::string &data_uri,
                    int start_time,
                    int stop_time,
                    const Properties &job_properties,
                    const Properties &media_properties)
                : MPFJob(job_name, data_uri, job_properties, media_properties)
                , start_time(start_time)
                , stop_time(stop_time)
                , has_feed_forward_track(false) {
        }

        MPFAudioJob(const std::string &job_name,
                    const std::string &data_uri,
                    int start_time,
                    int stop_time,
                    const MPFAudioTrack &track,
                    const Properties &job_properties,
                    const Properties &media_properties)
                : MPFJob(job_name, data_uri, job_properties, media_properties)
                , start_time(start_time)
                , stop_time(stop_time)
                , has_feed_forward_track(true)
                , feed_forward_track(track) {
        }
    };


    struct MPFGenericJob : MPFJob {
        bool has_feed_forward_track = false;
        MPFGenericTrack feed_forward_track;

        MPFGenericJob(const std::string &job_name,
                      const std::string &data_uri,
                      const Properties &job_properties,
                      const Properties &media_properties)
                : MPFJob(job_name, data_uri, job_properties, media_properties)
                , has_feed_forward_track(false) {
        }

        MPFGenericJob(const std::string &job_name,
                      const std::string &data_uri,
                      const MPFGenericTrack &track,
                      const Properties &job_properties,
                      const Properties &media_properties)
                : MPFJob(job_name, data_uri, job_properties, media_properties)
                , has_feed_forward_track(true)
                , feed_forward_track(track) {
        }
    };


// Class used for batch processing detection jobs, any type of media.
    class MPFDetectionComponent : public MPFComponent {

    public:

        virtual ~MPFDetectionComponent() { }


        virtual MPFDetectionError GetDetections(const MPFVideoJob &job, std::vector<MPFVideoTrack> &tracks)  = 0;

        virtual MPFDetectionError GetDetections(const MPFImageJob &job, std::vector<MPFImageLocation> &locations) = 0;

        virtual MPFDetectionError GetDetections(const MPFAudioJob &job, std::vector<MPFAudioTrack> &tracks) = 0;

        virtual MPFDetectionError GetDetections(const MPFGenericJob &job, std::vector<MPFGenericTrack> &tracks) = 0;

        virtual bool Supports(MPFDetectionDataType data_type) = 0;

        virtual std::string GetDetectionType() = 0;

        MPFComponentType GetComponentType() { return MPF_DETECTION_COMPONENT; };

    protected:

        MPFDetectionComponent() = default;
    };
}}


#endif //OPENMPF_CPP_COMPONENT_SDK_DETECTION_BASE_H
