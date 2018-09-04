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

#include <opencv2/opencv.hpp>
#include <opencv2/videoio.hpp>

#include "frame_transformers/FrameTransformerFactory.h"
#include "MPFDetectionException.h"

#include "MPFImageReader.h"


namespace MPF { namespace COMPONENT {

    MPFImageReader::MPFImageReader(const MPFImageJob &job) {
        cv::VideoCapture video_cap(job.data_uri);
        if (!video_cap.isOpened()) {
            throw MPFDetectionException(MPFDetectionError::MPF_COULD_NOT_OPEN_DATAFILE,
                                        "Failed to open \"" + job.data_uri + "\".");
        }
        bool was_read = video_cap.read(image_);
        if (!was_read || image_.empty()) {
            throw MPFDetectionException(MPFDetectionError::MPF_COULD_NOT_READ_DATAFILE,
                                        "Failed to read image from \"" + job.data_uri + "\".");
        }
        frameTransformer_ = GetFrameTransformer(job, image_);
        frameTransformer_->TransformFrame(image_, 0);
    }


    cv::Mat MPFImageReader::GetImage() const {
        return image_;
    }


    void MPFImageReader::ReverseTransform(MPFImageLocation &imageLocation) const {
        frameTransformer_->ReverseTransform(imageLocation, 0);
    }


    IFrameTransformer::Ptr MPFImageReader::GetFrameTransformer(const MPFImageJob &job, const cv::Mat &image) {
        return FrameTransformerFactory::GetTransformer(job, cv::Size(image.cols, image.rows));
    }
}}

