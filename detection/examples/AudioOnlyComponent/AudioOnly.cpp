/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2023 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2023 The MITRE Corporation                                       *
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

#include <iostream>
#include "AudioOnly.h"


bool AudioOnly::Init() {

    // Determine where the executable is running
    std::string run_dir = GetRunDirectory();
    if (run_dir == "") {
        run_dir = ".";
    }

    std::cout << "Running in directory " << run_dir << std::endl;

    return true;
}

//-----------------------------------------------------------------------------
bool AudioOnly::Close() {

    return true;
}

//-----------------------------------------------------------------------------
// Audio case
std::vector<MPFAudioTrack> AudioOnly::GetDetections(const MPFAudioJob &job) {

    // The MPFAudioJob structure contains all of the details needed to
    // process an audio file.
    std::cout << "[" << job.job_name << "] Processing \"" << job.data_uri << "\" from start time " << job.start_time << " msec to stop time " << job.stop_time << " msec." << std::endl;

    // NOTE: A stop_time parameter of -1 means process the whole file.

    // Detection logic goes here

    MPFAudioTrack audio_track(job.start_time, job.start_time+1);
    audio_track.confidence = 0.80f;
    // The MPFAudioTrack structure contains a Properties member that
    // can be used to return component-specific information about the
    // track. Here we add "METADATA", which might be used, for
    // example, to return the phrase recognized in the audio track.
    audio_track.detection_properties["METADATA"] = "extra audio track info";


    std::vector<MPFAudioTrack> tracks { audio_track };
    std::cout << "[" << job.job_name << "] Processing complete. Generated " << tracks.size() << " dummy audio tracks." << std::endl;
    return tracks;
}

//-----------------------------------------------------------------------------

MPF_COMPONENT_CREATOR(AudioOnly);
MPF_COMPONENT_DELETER();


