/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2023 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2023 The MITRE Corporation                                       *
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


namespace MPF::COMPONENT {

    MPFFrame::MPFFrame(int index, cv::Mat data)
            : index(index)
            , data(std::move(data)) {

    }

    namespace {

        void frameReader(MPFVideoCapture videoCapture,
                         BlockingQueue<std::optional<MPFFrame>> &queue) {
            try {
                while (true) {
                    int frameIndex = videoCapture.GetCurrentFramePosition();
                    cv::Mat frameData;
                    if (videoCapture.Read(frameData)) {
                        queue.emplace(MPFFrame(frameIndex, std::move(frameData)));
                    }
                    else {
                        // Add empty optional to indicate that the end of the video has been reached.
                        queue.push(std::nullopt);
                        queue.complete_adding();
                        return;
                    }
                }
            }
            catch (const QueueHaltedException&) {
                // Other side requested early exit.
            }
            catch (...) {
                try {
                    // Add empty optional to make sure other side doesn't get stuck if an exception
                    // is thrown.
                    queue.push(std::nullopt);
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


    MPFAsyncVideoCapture::MPFAsyncVideoCapture(std::string videoPath, int frameQueueSize)
            : MPFAsyncVideoCapture(MPFVideoCapture(std::move(videoPath)), frameQueueSize) {
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


    std::optional<MPFFrame> MPFAsyncVideoCapture::Read() {
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
            return std::nullopt;
        }
    }


    void MPFAsyncVideoCapture::ReverseTransform(MPFVideoTrack &videoTrack) const {
        reverseTransformer_(videoTrack);
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
}
