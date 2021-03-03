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


#ifndef OPENMPF_CPP_COMPONENT_SDK_MPF_GENERIC_DETECTION_COMPONENT_ADAPTER_H
#define OPENMPF_CPP_COMPONENT_SDK_MPF_GENERIC_DETECTION_COMPONENT_ADAPTER_H


#include <vector>

#include "MPFDetectionComponent.h"

namespace MPF { namespace COMPONENT {

    class MPFGenericDetectionComponentAdapter : public MPFDetectionComponent {
    public:

        std::vector<MPFAudioTrack> GetDetections(const MPFAudioJob &audio_job) override {
            // process generic job
            std::vector<MPFGenericTrack> generic_tracks = GetDetections(ConvertJob(audio_job));

            // convert generic tracks to expected type
            std::vector<MPFAudioTrack> audio_tracks;
            for (auto &generic_track : generic_tracks) {
                MPFAudioTrack audio_track;
                if (audio_job.has_feed_forward_track) {
                    audio_track.start_time = audio_job.feed_forward_track.start_time;
                    audio_track.stop_time = audio_job.feed_forward_track.stop_time;
                }
                audio_track.confidence = generic_track.confidence;
                audio_track.detection_properties = std::move(generic_track.detection_properties);
                audio_tracks.push_back(std::move(audio_track));
            }

            return audio_tracks;
        };

        std::vector<MPFImageLocation> GetDetections(const MPFImageJob &image_job) override {
            // process generic job
            std::vector<MPFGenericTrack> generic_tracks = GetDetections(ConvertJob(image_job));

            // convert generic tracks to expected type
            std::vector<MPFImageLocation> locations;
            for (auto &generic_track : generic_tracks) {
                MPFImageLocation location;
                if (image_job.has_feed_forward_location) {
                    location.x_left_upper = image_job.feed_forward_location.x_left_upper;
                    location.y_left_upper = image_job.feed_forward_location.y_left_upper;
                    location.width = image_job.feed_forward_location.width;
                    location.height = image_job.feed_forward_location.height;
                }
                location.confidence = generic_track.confidence;
                location.detection_properties = std::move(generic_track.detection_properties);
                locations.push_back(std::move(location));
            }

            return locations;
        }

        std::vector<MPFVideoTrack> GetDetections(const MPFVideoJob &video_job) override {
            // process generic job
            std::vector<MPFGenericTrack> generic_tracks = GetDetections(ConvertJob(video_job));

            std::vector<MPFVideoTrack> video_tracks;
            // convert generic tracks to expected type
            for (auto &generic_track : generic_tracks) {
                MPFVideoTrack video_track;
                if (video_job.has_feed_forward_track) {
                    video_track.start_frame = video_job.feed_forward_track.start_frame;
                    video_track.stop_frame = video_job.feed_forward_track.stop_frame;
                }
                video_track.confidence = generic_track.confidence;
                video_track.detection_properties = std::move(generic_track.detection_properties);

                // a video track needs at least one frame location
                video_track.frame_locations[0] = MPFImageLocation(0, 0, 0, 0, generic_track.confidence, video_track.detection_properties);

                video_tracks.push_back(std::move(video_track));
            }

            return video_tracks;
        }

        virtual std::vector<MPFGenericTrack> GetDetections(const MPFGenericJob &job) = 0;


        bool Supports(MPFDetectionDataType data_type) override {
            return true;
        };


    protected:
        MPFGenericDetectionComponentAdapter() = default;

    private:
        static MPFGenericJob ConvertJob(const MPFAudioJob &job) {
            if (job.has_feed_forward_track) {
                return {job.job_name,
                        job.data_uri,
                        MPFGenericTrack(job.feed_forward_track.confidence,
                                        job.feed_forward_track.detection_properties),
                        job.job_properties,
                        job.media_properties};
            }
            else {
                return {job.job_name, job.data_uri, job.job_properties, job.media_properties};
            }
        }


        static MPFGenericJob ConvertJob(const MPFImageJob &job) {
            if (job.has_feed_forward_location) {
                return {job.job_name,
                        job.data_uri,
                        MPFGenericTrack(job.feed_forward_location.confidence,
                                        job.feed_forward_location.detection_properties),
                        job.job_properties,
                        job.media_properties};
            }
            else {
                return {job.job_name, job.data_uri, job.job_properties, job.media_properties};
            }
        }


        static MPFGenericJob ConvertJob(const MPFVideoJob &job) {
            if (job.has_feed_forward_track) {
                return {job.job_name,
                        job.data_uri,
                        MPFGenericTrack(job.feed_forward_track.confidence,
                                        job.feed_forward_track.detection_properties),
                        job.job_properties,
                        job.media_properties};
            }
            else {
                return {job.job_name, job.data_uri, job.job_properties, job.media_properties};
            }
        }
    };

}}

#endif //OPENMPF_CPP_COMPONENT_SDK_MPF_GENERIC_DETECTION_COMPONENT_ADAPTER_H
