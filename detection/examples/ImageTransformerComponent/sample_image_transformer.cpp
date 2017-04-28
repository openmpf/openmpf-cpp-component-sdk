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
#include "ImageTransformerComponent.h"

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

    if ((3 != argc)) {
        std::cout << "Usage: " << argv[0] << " IMAGE_FILE_URI <ROTATE | CROP | FLIP>" << std::endl;
        return 0;
    }

    std::string uri(argv[1]);
    std::string transformation(argv[2]);

    // Requests for image transformations are passed to the component
    // through the job_properties field of the MPFImageJob. This is a
    // map where both the key and the value are strings.  The
    // MPFImageReader looks for the strings that indicate which
    // transformations to make. Here, we only choose one type of
    // transformation, but the MPFImageReader will compose the
    // transformations if more than one is requested. For example, if
    // you specify cropping and rotation, the image will be rotated
    // first and then cropped.

    Properties algorithm_properties;
    if (transformation == "ROTATE") {
        // The input image will be rotated 90 degrees
        algorithm_properties["ROTATION"] = to_string(90);
        std::cout << "Rotating the image by " << algorithm_properties["ROTATION"] << " degrees" << std::endl;
    }
    else if (transformation == "CROP") {
        // The input image will be cropped to a 100x100 pixel section in the upper left corner
        algorithm_properties["SEARCH_REGION_TOP_LEFT_X_DETECTION"] = to_string(0);
        algorithm_properties["SEARCH_REGION_TOP_LEFT_Y_DETECTION"] = to_string(0);
        algorithm_properties["SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION"] = to_string(100);
        algorithm_properties["SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION"] = to_string(100);
        algorithm_properties["SEARCH_REGION_ENABLE_DETECTION"] = "true";
        std::cout << "Cropping the image" << std::endl;
    }
    else if (transformation == "FLIP") {
        // The input image will be flipped horizontally, i.e., left to right
        algorithm_properties["HORIZONTAL_FLIP"] = "true";
        std::cout << "Flipping the image" << std::endl;
    }

    Properties media_properties;
    std::string job_name("Test Image Rotation");
    MPFImageJob job(job_name, uri, algorithm_properties, media_properties);

    // Instantiate the component
    ImageTransformerComponent im;
    // Declare the vector of image locations to be filled in by the
    // component.
    std::vector<MPFImageLocation> locations;
    // Pass the job to the image detection component
    MPFDetectionError rc = MPF_DETECTION_SUCCESS;
    rc = im.GetDetections(job, locations);
    if (rc == MPF_DETECTION_SUCCESS) {
        std::cout << "Number of image locations = "
                  << locations.size() << std::endl;

        for (int i = 0; i < locations.size(); i++) {
            std::cout << "Image location number " << i << "\n"
                      << "   x left upper = " << locations[i].x_left_upper << "\n"
                      << "   y left upper = " << locations[i].y_left_upper << "\n"
                      << "   width = " << locations[i].width << "\n"
                      << "   height = " << locations[i].height << "\n"
                      << "   confidence = " << locations[i].confidence << "\n"
                      << "   metadata = \"" << locations[i].detection_properties.at("METADATA") << "\"" << std::endl;
        }
    }
    else {
        std::cout << "GetDetections failed" << std::endl;
    }

    return 0;
}


