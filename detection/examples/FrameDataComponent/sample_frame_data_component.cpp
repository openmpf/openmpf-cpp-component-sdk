/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2016 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2016 The MITRE Corporation                                       *
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
#include "VideoFrameDataComponent.h"

/**
 * NOTE: This main is only intended to serve as a test harness for compiling a
 * stand-alone binary to debug the component logic independently of MPF.
 * MPF requires that the component logic be compiled into a shared object
 * library that is then dynamically loaded into a common detection component
 * executable.
 */

using namespace MPF;
using namespace COMPONENT;

int main(int argc, char* argv[]) {

    MPFDetectionError rc = MPF_DETECTION_SUCCESS;

    if ((5 != argc)) {
        std::cout << "Usage: " << argv[0] << " VIDEO_DATA_URI START_FRAME STOP_FRAME FRAME_INTERVAL" << std::endl;
        return 0;
    }

    // First do some sanity checking on the input arguments.

    std::string uri(argv[1]);
    int start_frame = atoi(argv[2]);
    int stop_frame = atoi(argv[3]);
    int frame_interval = atoi(argv[4]);
    std::cout << "Processing video from " << uri << std::endl;
    std::cout << "Start frame = " << start_frame << std::endl;
    std::cout << "Stop frame = " << stop_frame << std::endl;
    std::cout << "Frame interval = " << frame_interval << std::endl;

    if ((start_frame < 0) || (stop_frame <= start_frame)) {
        std::cerr << "Start frame must be greater than or equal 0, and stop_frame must be greater than start_frame" << std::endl;
        rc = MPF_INVALID_STOP_FRAME;
    }
    if (frame_interval < 1) {
        std::cerr << "Frame interval must be greater than or equal to 1" << std::endl;
        rc = MPF_INVALID_FRAME_INTERVAL;
    }
    // This example wants to build a track with 4 detections, and the
    // track starts in the third frame of the converted video segment.
    // We check here to make sure that the start frame, stop frame,
    // and frame interval will result in a converted video segment
    // with enough frames to contain such a track.
    int frames_in_segment = stop_frame - start_frame + 1;

    // Adjust the frame count to account for the frame interval
    float ratio = static_cast<float>(frames_in_segment)/static_cast<float>(frame_interval);
    frames_in_segment = static_cast<int>(std::ceil(ratio));
    int track_start_offset = 3;
    int track_num_detections = 4;
    int num_frames_needed = track_start_offset + track_num_detections;
    if (frames_in_segment < track_num_detections) {
        std::cerr << "Video segment is not big enough to contain the example track" << std::endl;
        rc = MPF_DETECTION_FAILED;
    }

    if (rc != MPF_DETECTION_SUCCESS) return (int)rc;

    Properties algorithm_properties;
    Properties media_properties;


    // instantiate the test component
    VideoFrameDataComponent comp;

    if (comp.Init()) {

        algorithm_properties["FRAME_INTERVAL"] = argv[4];

        MPFVideoJob job("TestVideoJob", uri,
                        start_frame, stop_frame,
                        algorithm_properties,
                        media_properties);

        std::vector<MPFVideoTrack> tracks;

        rc = comp.GetDetections(job, tracks);
        if (rc == MPF_DETECTION_SUCCESS) {
            std::cout << "Number of video tracks = " << tracks.size() << std::endl;

            for (int i = 0; i < tracks.size(); i++) {
                std::cout << "Video track number " << i << "\n"
                          << "   start frame = " << tracks[i].start_frame << "\n"
                          << "   stop frame = " << tracks[i].stop_frame << "\n"
                          << "   number of locations = " << tracks[i].frame_locations.size() << "\n"
                          << "   confidence = " << tracks[i].confidence << "\n"
                          << "   metadata = \"" << tracks[i].detection_properties.at("METADATA") << "\"" << std::endl;

                for (auto it : tracks[i].frame_locations) {
                    std::cout << "   Image location frame = " << it.first << "\n"
                              << "      x left upper = " << it.second.x_left_upper << "\n"
                              << "      y left upper = " << it.second.y_left_upper << "\n"
                              << "      width = " << it.second.width << "\n"
                              << "      height = " << it.second.height << "\n"
                              << "      confidence = " << it.second.confidence << "\n"
                              << "      metadata = \"" << it.second.detection_properties.at("METADATA") << "\"" << std::endl;
                }
            }
        }
    } else {
        std::cerr << "Error: Could not initialize detection component." << std::endl;
    }

    comp.Close();
    return 0;
}


