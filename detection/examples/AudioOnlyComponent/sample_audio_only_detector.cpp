/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2022 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2022 The MITRE Corporation                                       *
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

#include <string>
#include <vector>
#include <iostream>
#include <sstream>
#include <algorithm>
#include "AudioOnly.h"

/**
 * NOTE: This main is only intended to serve as a test harness for compiling a
 * stand-alone binary to debug the component logic independently of MPF.
 * MPF requires that the component logic be compiled into a shared object
 * library that is then dynamically loaded into a common detection component
 * executable.
 */
int main(int argc, char* argv[]) {

    if (4 != argc) {
        std::cout << "Usage: " << argv[0] << " AUDIO_DATA_URI START_TIME STOP_TIME" << std::endl;
        return 0;
    }

    std::string uri(argv[1]);

    MPF::COMPONENT::Properties algorithm_properties;
    MPF::COMPONENT::Properties media_properties;

    // instantiate the test component
    AudioOnly hw;

    if (hw.Init()) {
        int start_time = atoi(argv[2]);
        int stop_time = atoi(argv[3]);

        MPF::COMPONENT::MPFAudioJob job("TestAudioJob", uri, start_time, stop_time,
                                        algorithm_properties, media_properties);
        try {
            std::vector<MPFAudioTrack> tracks = hw.GetDetections(job);
            std::cout << "Number of audio tracks = " << tracks.size() << std::endl;

            for (int i = 0; i < tracks.size(); i++) {
                std::cout << "Audio track number " << i << "\n"
                          << "   start time = " << tracks[i].start_time << "\n"
                          << "   stop time = " << tracks[i].stop_time << "\n"
                          << "   confidence = " << tracks[i].confidence << "\n"
                          << "   metadata = \"" << tracks[i].detection_properties.at("METADATA") << "\"" << std::endl;
            }
        }
        catch (const std::exception &e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    } else {
        std::cerr << "Error: Could not initialize detection component." << std::endl;
    }

    hw.Close();
    return 0;
}


