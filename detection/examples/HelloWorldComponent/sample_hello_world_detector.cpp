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

#include <string>
#include <vector>
#include <iostream>
#include "HelloWorld.h"

using namespace MPF::COMPONENT;


void print_usage(char* program) {
    std::cout << "Usage: " << program << " -i IMAGE_DATA_URI" << std::endl;
    std::cout << "Usage: " << program << " -a AUDIO_DATA_URI START_TIME STOP_TIME" << std::endl;
    std::cout << "Usage: " << program << " -v VIDEO_DATA_URI START_FRAME STOP_FRAME FRAME_INTERVAL" << std::endl;
    std::cout << "Usage: " << program << " -g GENERIC_DATA_URI" << std::endl;
}

/**
 * NOTE: This main is only intended to serve as a test harness for compiling a
 * stand-alone binary to debug the component logic independently of MPF.
 * MPF requires that the component logic be compiled into a shared object
 * library that is then dynamically loaded into a common detection component
 * executable.
 */
int main(int argc, char* argv[]) {

    if (argc < 3) {
        print_usage(argv[0]);
        return 0;
    }

    MPF::COMPONENT::Properties algorithm_properties;
    MPF::COMPONENT::Properties media_properties;
    MPFDetectionError rc = MPF_DETECTION_SUCCESS;

    std::string option(argv[1]);

    MPFDetectionDataType media_type;
    if (3 == argc && option == "-i") {
        media_type = IMAGE;
    } else if (3 == argc && option == "-g") {
        media_type = UNKNOWN;
    } else if (5 == argc && option == "-a") {
        media_type = AUDIO;
    } else if (6 == argc && option == "-v") {
        media_type = VIDEO;
    } else {
        print_usage(argv[0]);
        return 0;
    }

    std::string uri(argv[2]);

    // instantiate the test component
    HelloWorld hw;

    if (hw.Init()) {

        switch (media_type) {
            case IMAGE:
            {
                MPF::COMPONENT::MPFImageJob job("TestImageJob", uri,
                                                algorithm_properties,
                                                media_properties);
                std::vector<MPFImageLocation> locations;
                rc = hw.GetDetections(job, locations);
                if (rc == MPF_DETECTION_SUCCESS) {
                    std::cout << "Number of image locations = "
                              << locations.size() << std::endl;

                    for (int i = 0; i < locations.size(); i++) {
                        std::cout << "Image location number " << i << "\n"
                                  << "   x left upper = " << locations[i].x_left_upper << "\n"
                                  << "   y left upper = " << locations[i].y_left_upper << "\n"
                                  << "   width = " << locations[i].width << "\n"
                                  << "   height = " << locations[i].height << "\n"
                                  << "   confidence = " << locations[i].confidence << std::endl;
                        for (auto prop : locations[i].detection_properties) {
                            std::cout << "   " << prop.first << " = \"" << prop.second << "\"" << std::endl;
                        }
                    }
                }
                break;
            }

            case AUDIO:
            {
                int start_time = atoi(argv[2]);
                int stop_time = atoi(argv[3]);
                MPFAudioJob job("TestAudioJob", uri,
                                start_time, stop_time,
                                algorithm_properties,
                                media_properties);
                std::vector<MPFAudioTrack> tracks;

                rc = hw.GetDetections(job, tracks);
                if (rc == MPF_DETECTION_SUCCESS) {
                    std::cout << "Number of audio tracks = " << tracks.size() << std::endl;

                    for (int i = 0; i < tracks.size(); i++) {
                        std::cout << "Audio track number " << i << "\n"
                                  << "   start time = " << tracks[i].start_time << "\n"
                                  << "   stop time = " << tracks[i].stop_time << "\n"
                                  << "   confidence = " << tracks[i].confidence << std::endl;
                        for (auto prop : tracks[i].detection_properties) {
                            std::cout << "   " << prop.first << " = \"" << prop.second << "\"" << std::endl;
                        }
                    }
                }
                break;
            }

            case VIDEO:
            {
                int start_frame = atoi(argv[2]);
                int stop_frame = atoi(argv[3]);
                algorithm_properties["FRAME_INTERVAL"] = argv[4];

                MPFVideoJob job("TestVideoJob", uri,
                                start_frame, stop_frame,
                                algorithm_properties,
                                media_properties);

                std::vector<MPFVideoTrack> tracks;

                rc = hw.GetDetections(job, tracks);
                if (rc == MPF_DETECTION_SUCCESS) {
                    std::cout << "Number of video tracks = " << tracks.size() << std::endl;

                    for (int i = 0; i < tracks.size(); i++) {
                        std::cout << "Video track number " << i << "\n"
                                  << "   start frame = " << tracks[i].start_frame << "\n"
                                  << "   stop frame = " << tracks[i].stop_frame << "\n"
                                  << "   number of locations = " << tracks[i].frame_locations.size() << "\n"
                                  << "   confidence = " << tracks[i].confidence << std::endl;
                        for (auto prop : tracks[i].detection_properties) {
                            std::cout << "   " << prop.first << " = \"" << prop.second << "\"" << std::endl;
                        }

                        for (auto it : tracks[i].frame_locations) {
                            std::cout << "   Image location frame = " << it.first << "\n"
                                      << "      x left upper = " << it.second.x_left_upper << "\n"
                                      << "      y left upper = " << it.second.y_left_upper << "\n"
                                      << "      width = " << it.second.width << "\n"
                                      << "      height = " << it.second.height << "\n"
                                      << "      confidence = " << it.second.confidence << std::endl;
                            for (auto prop : it.second.detection_properties) {
                                std::cout << "      " << prop.first << " = \"" << prop.second << "\"" << std::endl;
                            }
                        }
                    }
                }
                break;
            }

            case UNKNOWN:
            {
                MPFGenericJob job("TestGenericJob", uri,
                                  algorithm_properties,
                                  media_properties);
                std::vector<MPFGenericTrack> tracks;

                rc = hw.GetDetections(job, tracks);
                if (rc == MPF_DETECTION_SUCCESS) {
                    std::cout << "Number of generic tracks = " << tracks.size() << std::endl;

                    for (int i = 0; i < tracks.size(); i++) {
                        std::cout << "Generic track number " << i << "\n"
                                  << "   confidence = " << tracks[i].confidence << std::endl;
                        for (auto prop : tracks[i].detection_properties) {
                            std::cout << "   " << prop.first << " = \"" << prop.second << "\"" << std::endl;
                        }
                    }
                }
                break;
            }

            default:
            {
                std::cerr << "Error: Invalid detection type." << std::endl;
                break;
            }
        }

    } else {
        std::cerr << "Error: Could not initialize detection component." << std::endl;
    }

    hw.Close();
    return 0;
}


