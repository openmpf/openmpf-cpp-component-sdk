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

#ifndef OPENMPF_CPP_COMPONENT_SDK_MPFASYNCVIDEOCAPTURE_H
#define OPENMPF_CPP_COMPONENT_SDK_MPFASYNCVIDEOCAPTURE_H

#include <future>
#include <string>

#include <opencv2/core.hpp>

#include "MPFDetectionComponent.h"
#include "MPFVideoCapture.h"
#include "BlockingQueue.h"

namespace MPF { namespace COMPONENT {

    struct MPFFrame {
    public:
        // Include index field because we can't query MPFVideoCapture to get the current frame
        // position. This is because MPFVideoCapture will likely be a few frames ahead by the time
        // it is queried.
        int index;
        cv::Mat data;

        explicit MPFFrame(int index = -1, cv::Mat data = {});

        bool isValid() const;

        explicit operator bool() const;
    };


    /**
     * Reads frames from a regular MPFVideoCapture on a background thread. Frames are buffered
     * in a fixed sized BlockingQueue. This class intentionally exposes a subset of the
     * MPFVideoCapture functionality. Functionality that would impossible or difficult to write in
     * a thread-safe manner has been omitted. Frame transformers, frame filters, and feed forward
     * are all supported.
     */
    class MPFAsyncVideoCapture {
    public:
        explicit MPFAsyncVideoCapture(const MPFVideoJob &videoJob,
                                      bool enableFrameTransformers=true,
                                      bool enableFrameFiltering=true);

        explicit MPFAsyncVideoCapture(const std::string &videoPath, int frameQueueSize=4);

        ~MPFAsyncVideoCapture();

        MPFFrame Read();

        void ReverseTransform(MPFVideoTrack &videoTrack) const;

        /**
         * @return An object that can do the reverse transform even after MPFAsyncVideoCapture has
         *         been destroyed
         */
        ReverseTransformer GetReverseTransformer() const;

        int GetFrameCount() const;

        double GetFrameRate() const;

        cv::Size GetFrameSize() const;

        cv::Size GetOriginalFrameSize() const;


    private:
        BlockingQueue<MPFFrame> frameQueue_;

        // Fields for properties of the video that don't change as it is being read.
        // We can't just query the underlying video capture on the fly because it is being used by
        // the frameReader thread. These fields get set prior to handing the video capture
        // over to the frameReader thread.
        int frameCount_;
        double frameRate_;
        cv::Size frameSize_;
        cv::Size originalFrameSize_;

        ReverseTransformer reverseTransformer_;

        std::shared_future<void> doneReadingFuture_;

        MPFAsyncVideoCapture(MPFVideoCapture&& videoCapture, int frameQueueSize);
    };
}}



#endif //OPENMPF_CPP_COMPONENT_SDK_MPFASYNCVIDEOCAPTURE_H
