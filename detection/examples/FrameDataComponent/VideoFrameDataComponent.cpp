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

#include <iostream>

#include <VideoSegmentToFramesConverter.h>
#include <detectionComponentUtils.h>

#include "VideoFrameDataComponent.h"

using namespace MPF;
using namespace COMPONENT;

using std::map;
using std::pair;
using std::vector;

bool VideoFrameDataComponent::Init() {

    // Determine where the executable is running
    std::string run_dir = GetRunDirectory();
    if (run_dir == "") {
        run_dir = ".";
    }

    std::cout << "Running in directory " << run_dir << std::endl;

    return true;
}

bool VideoFrameDataComponent::Close() {
    return true;
}

MPFDetectionError VideoFrameDataComponent::GetDetectionsFromVideoFrameData(
        const MPFVideoFrameData &frame_data,
        const Properties &job_properties,
        const std::string &job_name,
        std::vector<MPFVideoTrack>& tracks)
{
    std::cout << "[" << job_name << "] Processing video from frame " << frame_data.start_frame
              << " to frame " << frame_data.stop_frame << "." << std::endl;

    // The MPFVideoFrameData structure contains a vector of byte
    // arrays.  The width, height, number of channels, and bytes per
    // channel can be used to calculate the number of bytes in each frame.
    std::cout << "[" << job_name << "] Frame data info:" << std::endl;
    std::cout << "                        frame width = " << frame_data.width << std::endl;
    std::cout << "                        frame height = " << frame_data.height << std::endl;
    std::cout << "                        number of channels = " << frame_data.num_channels << std::endl;
    std::cout << "                        bytes per channel = " << frame_data.bytes_per_channel << std::endl;
    std::cout << "                        frames in segment = " << frame_data.frames_in_segment << std::endl;

    // Detection and tracking logic goes here. The address of each
    // frame of data can be obtained through logic similar to the
    // following:
    for (int i = 0; i < frame_data.frames_in_segment; i++) {
        uint8_t *frame_addr = frame_data.data[i];
        // Detection processing operates on frame_addr.
    }

    // The following loop creates the output vector of tracks. In this
    // example, there is only one track in the output vector.

    MPFVideoTrack track;

    // Suppose we have a track that started in the third frame of the
    // segment.  Using indexing starting with 0, then the first
    // detection in the track was found in data array #2. This would
    // be the track start frame.
    int first_detection_frame_index = 2;
    track.start_frame = frame_data.start_frame + first_detection_frame_index;
    track.confidence = 0.8;
    track.detection_properties["METADATA"] = "interesting info about this track";

    int track_stop_frame = track.start_frame;
    for (int i = 0; i < 4; i++) {
        int frame_index = track.start_frame + i;
        if (frame_index > frame_data.stop_frame) break;
        track_stop_frame = frame_index;
        MPFImageLocation image(10+i, 10+i, 100, 200, 0.8);
        image.detection_properties["METADATA"] = "interesting info about this image";
        track.frame_locations[frame_index] = image;
    }
    track.stop_frame = track_stop_frame;
    tracks.push_back(track);

    return (MPF_DETECTION_SUCCESS);
}


MPFDetectionError VideoFrameDataComponent::GetDetections(const MPFVideoJob &job,
                                                         std::vector <MPFVideoTrack> &tracks) {

    MPFDetectionError rc = MPF_DETECTION_SUCCESS;
    MPFVideoFrameData frame_data;
    std::pair<MPFDetectionError, std::string> ret_info;

    // This example uses the MPFVideoCapture object, which is a
    // wrapper around the OpenCV cv::VideoCapture object.  It is
    // capable of applying frame transformations (e.g., cropping,
    // rotation, or flipping) to the data according to the properties
    // set in the MPFVideoJob structure.  We use it here to extract
    // the byte data from the video.  See also the MPFVideoCapture and
    // MPFImageReader examples.
    try {
        MPFVideoCapture cap(job);
        if (!cap.IsOpened()){
            std::cout << "Video Capture creation failed" << std::endl;
            return (MPF_COULD_NOT_OPEN_DATAFILE);
        }

        // This function allocates memory to store the byte data.
        // That allocation must be freed by the caller.
        ret_info = convertSegmentToFrameData(job, cap, frame_data);

        if (ret_info.first != MPF_DETECTION_SUCCESS) {
            // An unrecoverable error was returned from the video
            // segment converter.
            if (!frame_data.data.empty()) {
                delete [] frame_data.data[0];
                frame_data.data.clear();
            }
            std::cout << "[" << job.job_name << "] convertSegmentToFrameData failed" << std::endl;
            std::cout << "Error message: " << ret_info.second << std::endl;
            return ret_info.first;
        }
        // If MPF_DETECTION_SUCCESS was returned, but the error
        // message string is not empty, this indicates a warning
        // message. This can happen, for example, if an empty frame is
        // encountered in the video before the end frame.  The
        // component may choose to treat this as an error, but the
        // convention with other MPF components is to continue
        // processing, since there may be some data that was extracted
        // successfully.
        if (!ret_info.second.empty()) {
            std::cout << "video segment converter returned a warning message: "
                      << ret_info.second <<std::endl;

        }
        if (frame_data.data.empty()) {
            // In this case, no frame data was extracte from the
            // video.  The convention used with other MPF components
            // has been to simply return success in this circumstance,
            // but a component may choose to handle it differently.
            std::cout << "[" << job.job_name << "] Processing complete. found 0 detections" << std::endl;
            return MPF_DETECTION_SUCCESS;
        }

        rc = GetDetectionsFromVideoFrameData(frame_data,
                                             job.job_properties,
                                             job.job_name,
                                             tracks);
        delete [] frame_data.data[0];
        frame_data.data.clear();

        if (rc != MPF_DETECTION_SUCCESS) {
            std::cout << "[" << job.job_name << "] GetDetectionsFromVideoFrameData failed" << std::endl;
            return rc;
        }

        // The MPFVideoCapture object may have applied certain
        // transformations on the input video.  The following logic
        // reverses any such transformations, so that the final output
        // that the user sees from the job relates more readily to the
        // input media.
        for (auto &ThisTrack : tracks) {
            cap.ReverseTransform(ThisTrack);
        }
    }
    catch (...) {
        if (!frame_data.data.empty()) {
            delete [] frame_data.data[0];
            frame_data.data.clear();
        }
        ret_info = DetectionComponentUtils::HandleDetectionException(MPFDetectionDataType::VIDEO);
    }

    return rc;
}

MPF_COMPONENT_CREATOR(VideoFrameDataComponent);
MPF_COMPONENT_DELETER();
