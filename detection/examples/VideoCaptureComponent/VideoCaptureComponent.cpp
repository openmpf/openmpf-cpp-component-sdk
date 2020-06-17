/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2020 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2020 The MITRE Corporation                                       *
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
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <detectionComponentUtils.h>
#include <MPFDetectionException.h>
#include <MPFVideoCapture.h>
#include "VideoCaptureComponent.h"

using namespace MPF;
using namespace COMPONENT;

bool VideoCaptureComponent::Init() {

    // Determine where the executable is running
    std::string run_dir = GetRunDirectory();
    if (run_dir == "") {
        run_dir = ".";
    }

    std::cout << "Running in directory " << run_dir << std::endl;

    return true;
}

bool VideoCaptureComponent::Close() {

    return true;
}

std::vector<MPFVideoTrack> VideoCaptureComponent::GetDetections(const MPFVideoJob &job)
{

    // The MPFVideoJob structure contains all of the details needed to
    // process a segment of a video.
    std::cout << "[" << job.job_name << "] Processing \"" << job.data_uri << "\"." << std::endl;
    std::cout << "start frame: " << job.start_frame << std::endl;
    std::cout << "stop frame: " << job.stop_frame << std::endl;

    // Check whether the use of imshow() to illustrate the results of
    // frame transformations has been requested. This should only be
    // set when running in standalone mode; this functionality will
    // fail when the component is run through the web UI.

    bool imshow_on = DetectionComponentUtils::GetProperty<bool>(
            job.job_properties, "IMSHOW_ON", false);
    if (imshow_on) {
        std::cout << "Frame transformation illustration windows turned on" << std::endl;
    }

    // The frame interval is in the job_properties structure. It can
    // be retrieved using the DetectionComponentUtils::GetProperty
    // template function. That function takes the job_properties map,
    // the name of the property to be retrieved, and a default
    // value. When no frame interval is supplied through the job
    // properties, the component must use a value of 1 as the
    // default.
    int frame_interval = DetectionComponentUtils::GetProperty<int>(
            job.job_properties, "FRAME_INTERVAL", 1);

    // The MPFVideoCapture object is a wrapper around the OpenCV
    // cv::VideoCapture object. It automatically applies
    // transformations to the captured frame, as specified in the
    // job_properties map and the media_properties map in the
    // MPFVideoJob structure. Here, we instantiate an MPFVideoCapture
    // object for this job.
    MPFVideoCapture cap(job);

    // The MPFVideoCapture can provide information about the segment.
    int total_frames = cap.GetFrameCount();
    std::cout << "Total video frames: " << total_frames << std::endl;

    // Make sure the start and stop frames defined for this segment is
    // not beyond the end of the video.
    if (job.start_frame >= total_frames) {
        std::cout << "Requested start_frame is greater than the number of frames in the video" << std::endl;
        throw MPFDetectionException(MPF_INVALID_START_FRAME,
                "Requested start_frame is greater than the number of frames in the video");
    }

    if (job.stop_frame >= total_frames) {
        std::cout << "Requested stop_frame is greater than the number of frames in the video" << std::endl;
        throw MPFDetectionException(MPF_INVALID_STOP_FRAME,
                                    "Requested stop_frame is greater than the number of frames in the video");
    }

    double fps = cap.GetFrameRate();
    std::cout << "Video frame rate: " << fps << std::endl;

    // If cropping was requested for this job, then the
    // MPFVideoCapture can be used to retrieve the original frame size
    // and the cropped frame size.
    cv::Size original_size = cap.GetOriginalFrameSize();
    cv::Size cropped_size = original_size;
    std::cout << "Original frame width: " << original_size.width << std::endl;
    std::cout << "Original frame height: " << original_size.height << std::endl << std::endl;
    bool isCropped = DetectionComponentUtils::GetProperty<bool>(job.job_properties, "SEARCH_REGION_ENABLE_DETECTION", false);
    if (isCropped) {
        cropped_size = cap.GetFrameSize();
        std::cout << "Cropped frame width: " << cropped_size.width << std::endl;
        std::cout << "Cropped frame height: " << cropped_size.height << std::endl;
    }

    // The MPFVideoCapture can be used to set the frame position.
    // Here, we first do some sanity checking to make sure that the
    // start and stop frame positions requested in the job are within
    // the bounds of the video being captured. We then set the
    // position of the next frame to be captured to the start frame.
    int frame_index = 0;
    //try to set start frame if start_frame != 0
    if (job.start_frame > 0 && job.stop_frame < total_frames) {
        cap.SetFramePosition(job.start_frame);
        frame_index = job.start_frame;
    }

    // The MPFVideoCapture can be used to retrieve one frame at a time
    // from the video.
    cv::Mat frame;
    cap.Read(frame);

    if (imshow_on) {
        std::cout << "For illustration purposes only, we display the captured frame in a window." << std::endl << std::endl;
        std::cout << "======================================================================================" << std::endl;
        std::cout << "To continue, click the window titled \"Captured Frame\" and hit any key." << std::endl;
        std::cout << "======================================================================================" << std::endl << std::endl << std::endl;
        cv::namedWindow("Captured Frame", cv::WINDOW_AUTOSIZE);
        cv::imshow("Captured Frame", frame);
        cv::waitKey(0);
    }

    // Here is an example of a loop to process the video segment one
    // frame at a time. the frame interval must be taken into account,
    // so we forward the position of the next frame to be read.
    frame_index = frame_index + frame_interval;
    bool wasRead = false;
    while (!frame.empty() && (frame_index <= job.stop_frame)) {
        std::cout << "Capturing frame " << frame_index << std::endl;
        cap.SetFramePosition(frame_index);
        wasRead = cap.Read(frame);
        if (wasRead && !frame.empty()) {
            // Detection and tracking logic goes here. The component operates
            // on the cv:Mat object "frame" returned by the call to cap.Read().
        }
        frame_index += frame_interval;
    }

    // After detection processing is complete, the component creates a
    // vector of tracks containing image locations.

    MPFVideoTrack video_track(job.start_frame, job.stop_frame);
    video_track.confidence = 0.80f;

    // The MPFVideoTrack structure contains a Properties member that
    // can be used to return component-specific information about the
    // track. Here we add "METADATA", which might be used, for
    // example, to return the type of the object that is tracked.
    video_track.detection_properties["METADATA"] = "extra video track info";

    MPFImageLocation image_location(0, 0, 100, 100);
    image_location.confidence = 0.80f;

    // The MPFImageLocation structure also contains a Properties
    // member that can be used to return component-specific
    // information about the image in a particular frame. Here we add
    // "METADATA", which might be used, for example, to return the
    // pose of the object detected in the frame.
    image_location.detection_properties["METADATA"] = "extra image location info";

    video_track.frame_locations[job.start_frame] = image_location;

    // The component must call ReverseTransform() on each video track,
    // so that they can be related back to the original media.
    cap.ReverseTransform(video_track);

    std::vector<MPFVideoTrack> tracks { video_track };
    std::cout << "[" << job.job_name << "] Processing complete. Generated " << tracks.size() << " dummy tracks." << std::endl;
    return tracks;
}


MPF_COMPONENT_CREATOR(VideoCaptureComponent);
MPF_COMPONENT_DELETER();


