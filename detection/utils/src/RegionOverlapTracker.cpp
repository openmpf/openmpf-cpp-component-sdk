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

#include "RegionOverlapTracker.h"

#include <stdexcept>
#include <opencv2/core.hpp>

namespace MPF { namespace COMPONENT {

    RegionOverlapTracker::RegionOverlapTracker(double min_overlap)
            : min_overlap_(min_overlap) {
    }


    bool RegionOverlapTracker::IsSameTrack(const MPFImageLocation &new_loc, int frame_number,
                                           const MPFVideoTrack &existing_track) {
        return OverlapsEnough(new_loc, existing_track)
               && OverlappingDetectionsAreSameTrack(new_loc, frame_number, existing_track);
    }


    bool RegionOverlapTracker::OverlapsEnough(const MPFImageLocation &new_loc, const MPFVideoTrack &existing_track) {
        cv::Rect detection_rect(new_loc.x_left_upper, new_loc.y_left_upper, new_loc.width, new_loc.height);

        auto last_detection_it = existing_track.frame_locations.rbegin();
        if (last_detection_it == existing_track.frame_locations.rend()) {
            throw std::length_error("Track must not be empty.");
        }
        auto &last_detection = last_detection_it->second;
        cv::Rect track_rect(last_detection.x_left_upper, last_detection.y_left_upper,
                            last_detection.width, last_detection.height);

        if (track_rect.empty()) {
            return track_rect == detection_rect;
        }

        cv::Rect intersection = track_rect & detection_rect;
        double ratio = intersection.area() / static_cast<double>(track_rect.area());
        return ratio >= min_overlap_;
    }

}}