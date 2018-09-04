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

#include <map>
#include <iostream>
#include <opencv2/highgui/highgui.hpp>
#include <MPFImageReader.h>
#include "ImageTransformerComponent.h"

using namespace MPF;
using namespace COMPONENT;

bool ImageTransformerComponent::Init() {

    // Determine where the executable is running
    std::string run_dir = GetRunDirectory();
    if (run_dir == "") {
        run_dir = ".";
    }

    std::cout << "Running in directory " << run_dir << std::endl;

    return true;
}

bool ImageTransformerComponent::Close() {

    return true;
}

MPFDetectionError ImageTransformerComponent::GetDetections(const MPFImageJob &job,
                                                           std::vector <MPFImageLocation> &locations)
{

    // The MPFImageJob structure contains all of the details needed to
    // process an image file.
    std::cout << "[" << job.job_name << "] Processing \"" << job.data_uri << "\"." << std::endl;

    // The MPFImage Reader parses the properties in the job to look
    // for transformation requests.  If found, they will be applied to
    // the image when GetImage() is called. If more than one
    // transformation has been requested, they will be composed in the
    // following order: rotation, followed by flipping, followed by
    // cropping. This is done transparently, so the component logic
    // must remember to reverse any transformations after the
    // detection logic is finished.

    // For illustration purposes only, we display the input image here
    // before applying the transformation.
    cv::Mat original = MPFImageReader({"Test", job.data_uri, {}, {}}).GetImage();
    std::cout << "original image rows = " << original.rows << std::endl;
    std::cout << "original image cols = " << original.cols << std::endl;
    std::cout << std::endl << std::endl;
    cv::namedWindow("Original", CV_WINDOW_AUTOSIZE);
    cv::imshow("Original", original);
    cv::waitKey(500);

    if (original.empty()) {
        std::cout << "[" << job.job_name << "] Could not open original image and will not return detections" << std::endl;
        return MPF_IMAGE_READ_ERROR;
    }

    MPFImageReader image_reader(job);
    cv::Mat image_data(image_reader.GetImage());

    if (image_data.empty()) {
        std::cout << "[" << job.job_name << "] Could not open transformed image and will not return detections" << std::endl;
        return MPF_IMAGE_READ_ERROR;
    }

    std::cout << "transformed image rows = " << image_data.rows << std::endl;
    std::cout << "transformed image cols = " << image_data.cols << std::endl;
    std::cout << std::endl << std::endl;
    std::cout << "======================================================================================" << std::endl;
    std::cout << "To continue, click on the window titled \"Transformed\" and hit any key." << std::endl;
    std::cout << "======================================================================================" << std::endl << std::endl << std::endl;

    // Again, for illustration purposes only, we display the image after
    // the transformation has been applied.
    cv::namedWindow("Transformed", CV_WINDOW_AUTOSIZE);
    cv::imshow("Transformed", image_data);
    cv::waitKey(0);

    // Detection logic goes here. the component operates on the cv:Mat
    // object "image_data" returned by the call to GetImage.

    // After detection processing is complete, the component creates a
    // vector of image locations.

    MPFImageLocation image_location(2, 4, 10, 10);
    image_location.confidence = 0.80f;

    // The MPFImageLocation structure contains a Properties member that
    // can be used to return component-specific information about the
    // image. Here we add "METADATA", which might be used, for
    // example, to return the type of object detected in the image.
    image_location.detection_properties["METADATA"] = "extra image location info";

    // The component must call ReverseTransform() on each
    // image location in the vector, so that they can be related back
    // to the original media.

    image_reader.ReverseTransform(image_location);
    locations.push_back(image_location);

    std::cout << "[" << job.job_name << "] Processing complete. Generated " << locations.size() << " dummy image locations." << std::endl;

    return MPF_DETECTION_SUCCESS;
}

//-----------------------------------------------------------------------------
bool ImageTransformerComponent::Supports(MPFDetectionDataType data_type) {

    if (data_type == MPFDetectionDataType::IMAGE) {
        return true;
    }
    return false;
}

MPF_COMPONENT_CREATOR(ImageTransformerComponent);
MPF_COMPONENT_DELETER();


