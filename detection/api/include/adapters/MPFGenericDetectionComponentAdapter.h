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


#ifndef OPENMPF_CPP_COMPONENT_SDK_MPF_GENERIC_DETECTION_COMPONENT_ADAPTER_H
#define OPENMPF_CPP_COMPONENT_SDK_MPF_GENERIC_DETECTION_COMPONENT_ADAPTER_H


#include <vector>

#include "MPFDetectionComponent.h"

namespace MPF { namespace COMPONENT {

    class MPFGenericDetectionComponentAdapter : public MPFDetectionComponent {
    public:
        virtual ~MPFGenericDetectionComponentAdapter() = default;

        MPFDetectionError GetDetections(const MPFAudioJob &audio_job, std::vector<MPFAudioTrack> &audio_tracks) override {
            // create generic job
            MPFGenericJob generic_job(audio_job.job_name, audio_job.data_uri, audio_job.job_properties, audio_job.media_properties);
            if (audio_job.has_feed_forward_track) {
                MPFGenericTrack generic_feed_forward_track(audio_job.feed_forward_track.confidence, audio_job.feed_forward_track.detection_properties);
                generic_job.feed_forward_track = generic_feed_forward_track;
            }

            // process generic job
            std::vector<MPFGenericTrack> generic_tracks;
            MPFDetectionError rc = GetDetections(generic_job, generic_tracks);

            if (rc != MPFDetectionError::MPF_DETECTION_SUCCESS) {
                return rc;
            }

            // convert generic tracks to expected type
            for (auto generic_track : generic_tracks) {
                MPFAudioTrack audio_track;
                if (audio_job.has_feed_forward_track) {
                    audio_track.start_time = audio_job.feed_forward_track.start_time;
                    audio_track.stop_time = audio_job.feed_forward_track.stop_time;
                }
                audio_track.confidence = generic_track.confidence;
                audio_track.detection_properties = generic_track.detection_properties;
                audio_tracks.push_back(audio_track);
            }

            return MPFDetectionError::MPF_DETECTION_SUCCESS;
        };

        MPFDetectionError GetDetections(const MPFImageJob &job, std::vector<MPFImageLocation> &locations) override {
            return MPFDetectionError::MPF_UNSUPPORTED_DATA_TYPE;
        }

        MPFDetectionError GetDetections(const MPFVideoJob &job, std::vector<MPFVideoTrack> &tracks) override {
            return MPFDetectionError::MPF_UNSUPPORTED_DATA_TYPE;
        }

        virtual MPFDetectionError GetDetections(const MPFGenericJob &job, std::vector<MPFGenericTrack> &tracks) = 0;


        bool Supports(MPFDetectionDataType data_type) override {
            return true;
        };


    protected:
        MPFGenericDetectionComponentAdapter() = default;
    };

}}

#endif //OPENMPF_CPP_COMPONENT_SDK_MPF_GENERIC_DETECTION_COMPONENT_ADAPTER_H