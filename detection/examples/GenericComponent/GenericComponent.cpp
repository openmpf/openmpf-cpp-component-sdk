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

#include <map>
#include <iostream>
#include <detectionComponentUtils.h>
#include "GenericComponent.h"

//-----------------------------------------------------------------------------
/* virtual */ bool GenericComponent::Init() {

    // Determine where the executable is running
    std::string run_dir = GetRunDirectory();
    if (run_dir == "") {
        run_dir = ".";
    }

    std::cout << "Running in directory " << run_dir << std::endl;

    return true;
}

//-----------------------------------------------------------------------------
/* virtual */ bool GenericComponent::Close() {

    return true;
}

//-----------------------------------------------------------------------------
// Generic case
std::vector<MPFGenericTrack> GenericComponent::GetDetections(const MPFGenericJob &job) {

    // The MPFGenericJob structure contains all of the details needed to
    // process a generic file.
    std::cout << "[" << job.job_name << "] Processing \"" << job.data_uri << "\"." << std::endl;

    // Detection logic goes here

    MPFGenericTrack track(0.80f);

    // The MPFGenericTrack structure contains a Properties member that
    // can be used to return component-specific information about the
    // file. Here we add "METADATA", which might be used, for
    // example, to return the type of object detected in the image.
    track.detection_properties["METADATA"] = "extra generic track info";

    // Do something with the feed forward track if it exists
    if (job.has_feed_forward_track) {
        int feed_forward_count = DetectionComponentUtils::GetProperty(job.feed_forward_track.detection_properties, "FEED_FORWARD_COUNT", 0);
        track.detection_properties["FEED_FORWARD_COUNT"] = std::to_string(feed_forward_count+1);
    }

    std::vector<MPFGenericTrack> tracks { track };
    std::cout << "[" << job.job_name << "] Processing complete. Generated " << tracks.size() << " dummy tracks." << std::endl;
    return tracks;
}

//-----------------------------------------------------------------------------
bool GenericComponent::Supports(MPFDetectionDataType data_type) {
    return true; // supports everything, even UNKNOWN types
}

//-----------------------------------------------------------------------------
std::string GenericComponent::GetDetectionType() {
    return "GENERIC";
}

MPF_COMPONENT_CREATOR(GenericComponent);
MPF_COMPONENT_DELETER();


