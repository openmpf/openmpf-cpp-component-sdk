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

#include "MPFAsyncVideoCapture.h"

#include <utility>

#include "detectionComponentUtils.h"


using DetectionComponentUtils::GetProperty;


namespace MPF { namespace COMPONENT {

    MPFFrame::MPFFrame(int index, cv::Mat data)
            : index(index)
            , data(std::move(data)) {

    }

    bool MPFFrame::isValid() const {
        return index >= 0 && !data.empty();
    }

    MPFFrame::operator bool() const {
        return isValid();
    }


    namespace {

        void frameReader(MPFVideoCapture videoCapture, BlockingQueue<MPFFrame> &queue) {
            try {
                while (true) {
                    MPFFrame frame(videoCapture.GetCurrentFramePosition());
                    bool wasRead = videoCapture.Read(frame.data);
                    if (wasRead && frame) {
                        queue.push(std::move(frame));
                    }
                    else {
                        // Add invalid frame to indicate that the end of the video has been reached.
                        frame.data.release();
                        queue.push(std::move(frame));
                        queue.complete_adding();
                        return;
                    }
                }
            }
            catch (const QueueHaltedException&) {
                // Other side requested early exit.
            }
            catch (...) {
                // Make sure other side doesn't get stuck if an exception is thrown.
                try {
                    // If you try to read past the end of a video with cv::VideoCapture,
                    // it stops incrementing the frame count and keeps reporting
                    // (last_frame_index + 1) or equivalently the total number of frames.
                    queue.emplace(videoCapture.GetFrameCount());
                    queue.complete_adding();
                }
                catch (const QueueHaltedException&) {
                    // This is very unlikely, but we don't want the QueueHaltedException to
                    // hide the true exception;
                }
                throw;
            }
        }
    }



    MPFAsyncVideoCapture::MPFAsyncVideoCapture(const MPFVideoJob &videoJob,
                                               bool enableFrameTransformers,
                                               bool enableFrameFiltering)
            : MPFAsyncVideoCapture(
                    MPFVideoCapture(videoJob, enableFrameTransformers, enableFrameFiltering),
                    GetProperty(videoJob.job_properties, "FRAME_QUEUE_CAPACITY", 4))
    {
    }


    MPFAsyncVideoCapture::MPFAsyncVideoCapture(const std::string &videoPath, int frameQueueSize)
            : MPFAsyncVideoCapture(MPFVideoCapture(videoPath), frameQueueSize) {
    }


    MPFAsyncVideoCapture::MPFAsyncVideoCapture(MPFVideoCapture &&videoCapture, int frameQueueSize)
            : frameQueue_(frameQueueSize)
            , frameCount_ (videoCapture.GetFrameCount())
            , frameRate_(videoCapture.GetFrameRate())
            , frameSize_(videoCapture.GetFrameSize())
            , originalFrameSize_(videoCapture.GetOriginalFrameSize())
            , reverseTransformer_(videoCapture.GetReverseTransformer())
            , doneReadingFuture_(std::async(
                    std::launch::async,
                    frameReader, std::move(videoCapture), std::ref(frameQueue_)))
    {
    }


    MPFAsyncVideoCapture::~MPFAsyncVideoCapture() {
        frameQueue_.halt();
    }


    MPFFrame MPFAsyncVideoCapture::Read() {
        try {
            auto frame = frameQueue_.pop();
            if (!frame) {
                // If frameReader ended with an exception it will be re-thrown here.
                doneReadingFuture_.get();
            }
            return frame;
        }
        catch (QueueHaltedException&) {
            // If frameReader ended with an exception it will be re-thrown here.
            doneReadingFuture_.get();
            // If you try to read past the end of a video with cv::VideoCapture,
            // it stops incrementing the frame count and keeps reporting
            // (last_frame_index + 1) or equivalently the total number of frames.
            return MPFFrame(frameCount_);
        }
    }


    void MPFAsyncVideoCapture::ReverseTransform(MPFVideoTrack &videoTrack) const {
        return reverseTransformer_(videoTrack);
    }

    ReverseTransformer MPFAsyncVideoCapture::GetReverseTransformer() const {
        return reverseTransformer_;
    }

    int MPFAsyncVideoCapture::GetFrameCount() const {
        return frameCount_;
    }

    double MPFAsyncVideoCapture::GetFrameRate() const {
        return frameRate_;
    }

    cv::Size MPFAsyncVideoCapture::GetFrameSize() const {
        return frameSize_;
    }

    cv::Size MPFAsyncVideoCapture::GetOriginalFrameSize() const {
        return originalFrameSize_;
    }
}}
