/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2018 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2018 The MITRE Corporation                                       *
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

#include "BaseTracker.h"

#include <algorithm>
#include <unordered_map>


namespace MPF { namespace COMPONENT {

    void BaseTracker::ProcessFrameDetections(MPFImageLocation &&image_location, int frame_number) {
        detections_.emplace_back(frame_number, std::move(image_location));
    }


    std::vector<MPFVideoTrack> BaseTracker::GetTracks() {
        using detection_element_t = decltype(detections_)::value_type;
        std::stable_sort(detections_.begin(), detections_.end(),
                         [](const detection_element_t& left, const detection_element_t &right){
                             return left.first < right.first;
                         });

        std::unordered_multimap<int, MPFVideoTrack> tracks;
        for (auto& detection_pair : detections_) {
            auto& frame_number = detection_pair.first;
            auto& image_location = detection_pair.second;

            auto is_same_track_pred = [&](const std::pair<int, MPFVideoTrack> &pair) {
                return IsSameTrack(image_location, frame_number, pair.second);
            };
            {
                auto current_frame_range = tracks.equal_range(frame_number);
                auto find_result = std::find_if(current_frame_range.first, current_frame_range.second, is_same_track_pred);
                if (find_result != current_frame_range.second) {
                    AddToTrack(std::move(image_location), frame_number, find_result->second);
                    continue;
                }
            }
            {
                auto prev_frame_range = tracks.equal_range(frame_number - 1);
                auto find_result = std::find_if(prev_frame_range.first, prev_frame_range.second, is_same_track_pred);
                if (find_result != prev_frame_range.second) {
                    AddToTrack(std::move(image_location), frame_number, find_result->second);
                    tracks.emplace(frame_number, std::move(find_result->second));
                    tracks.erase(find_result);
                    continue;
                }
            }

            tracks.emplace(frame_number, CreateTrack(std::move(image_location), frame_number));
        }
        detections_ = { };

        std::vector<MPFVideoTrack> results;
        results.reserve(tracks.size());
        for (auto& pair : tracks) {
            results.push_back(std::move(pair.second));
        }
        return results;
    }
}}
