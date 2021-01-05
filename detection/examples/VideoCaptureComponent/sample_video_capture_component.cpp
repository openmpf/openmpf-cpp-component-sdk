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

#include <string>
#include <vector>
#include <iostream>
#include "VideoCaptureComponent.h"

/**
 * NOTE: This main is only intended to serve as a test harness for compiling a
 * stand-alone binary to debug the component logic independently of MPF.
 * MPF requires that the component logic be compiled into a shared object
 * library that is then dynamically loaded into a common detection component
 * executable.
 */

using namespace MPF;
using namespace COMPONENT;

using std::to_string;

int main(int argc, char* argv[]) {

    if ((5 != argc)) {
        std::cout << "Usage: " << argv[0] << " VIDEO_FILE_URI START_FRAME STOP_FRAME <ROTATE | CROP | FLIP>" << std::endl;
        return 0;
    }

    std::string uri(argv[1]);
    int start_frame = atoi(argv[2]);
    int stop_frame = atoi(argv[3]);
    std::string transformation(argv[4]);

    if ((start_frame < 0) || (stop_frame <= start_frame)) {
        std::cerr << "Start frame must be greater than or equal 0, and stop_frame must be greater than start_frame" << std::endl;
        return MPF_INVALID_STOP_FRAME;
    }

    // Requests for frame transformations are passed to the component
    // through the job_properties field of the MPFVideoJob. This is a
    // map where both the key and the value are strings.  The
    // MPFVideoCapture looks for the strings that indicate which
    // transformations to make. Here, we only choose one type of
    // transformation, but the MPFVideoCapture will compose the
    // transformations if more than one is requested. For example, if
    // you specify cropping and rotation, the image will be rotated
    // first and then cropped.

    Properties algorithm_properties;
    if (transformation == "ROTATE") {
        // Each video frame of the video will be rotated 270 degrees
        algorithm_properties["ROTATION"] = to_string(270);
        std::cout << "Rotating frames by " << algorithm_properties["ROTATION"] << " degrees" << std::endl;
    }
    else if (transformation == "CROP") {
        // Each video frame will be cropped to a 200x200 pixel section in the upper left corner
        algorithm_properties["SEARCH_REGION_TOP_LEFT_X_DETECTION"] = to_string(0);
        algorithm_properties["SEARCH_REGION_TOP_LEFT_Y_DETECTION"] = to_string(0);
        algorithm_properties["SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION"] = to_string(200);
        algorithm_properties["SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION"] = to_string(200);
        algorithm_properties["SEARCH_REGION_ENABLE_DETECTION"] = "true";
        std::cout << "Cropping frames" << std::endl;
    }
    else if (transformation == "FLIP") {
        // Each video frame will be flipped horizontally, i.e., left to right
        algorithm_properties["HORIZONTAL_FLIP"] = "true";
        std::cout << "Flipping frames" << std::endl;
    }
    algorithm_properties["FRAME_INTERVAL"] = to_string(1);

    // This example component uses the OpenCV imshow() function to
    // illustrate the results of the frame transformations. It is
    // controlled by the following application-specific property. This
    // is a boolean-valued property, and when it is set to true, the
    // image windows will pop up on the console. However, this will
    // fail when run as a component through the MPF web UI, since
    // the component will not have access to the console. Therefore,
    // when run through the UI, this property must be left undefined.

    algorithm_properties["IMSHOW_ON"] = "true";

    Properties media_properties;
    std::string job_name("Test Video Capture");
    MPFVideoJob job(job_name, uri, start_frame, stop_frame, algorithm_properties, media_properties);

    // Instantiate the component
    VideoCaptureComponent vc;

    try {
        std::vector<MPFVideoTrack> tracks = vc.GetDetections(job);
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
    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}


