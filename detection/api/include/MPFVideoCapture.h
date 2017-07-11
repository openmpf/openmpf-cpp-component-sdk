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


#ifndef OPENMPF_CPP_COMPONENT_SDK_MPFVIDEOCAPTURE_H
#define OPENMPF_CPP_COMPONENT_SDK_MPFVIDEOCAPTURE_H


#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>

#include "MPFDetectionComponent.h"
#include "frame_transformers/IFrameTransformer.h"


namespace MPF { namespace COMPONENT {

    class MPFVideoCapture {

    public:
        /**
         * Initializes a new MPFVideoCapture instance, using the frame
         * transformers specified in jobProperties, to be used for video
         * processing jobs.
         * @param videoJob
         * @throws std::invalid_argument videoJob contains invalid property
         */
        explicit MPFVideoCapture(const MPFVideoJob &videoJob);

        // NOTE: There are two problems with the imread() function in
        // OpenCV 3.1.0. Functionality was introduced in that release so
        // that when a jpeg file with EXIF information is read by
        // cv::imread(), it automatically uses the EXIF information to
        // return a transformed image. The first problem was that the new
        // code may hang when reading certain jpeg files
        // (https://github.com/opencv/opencv/issues?utf8=✓&q=6641). The
        // second problem is that you cannot tell imread() to ignore the
        // EXIF data and return an untransformed image
        // (https://github.com/opencv/opencv/issues?utf8=✓&q=6348). For
        // these reasons, we have disabled the use of the
        // MPFImageReader. To process MPFImageJobs, use the following
        // version of the MPFVideoCapture. The MPFImageReader will be
        // re-enabled when we update to OpenCV 3.2.0, where these problems
        // have been fixed.

        /**
         * Initializes a new MPFVideoCapture instance, using the frame
         * transformers specified in jobProperties, to be used for image
         * processing jobs.
         * @param imageJob
         * @throws std::invalid_argument imageJob contains invalid property
         */
        explicit MPFVideoCapture(const MPFImageJob &imageJob);

        bool Read(cv::Mat &frame);

        MPFVideoCapture &operator>>(cv::Mat &frame);

        bool IsOpened() const;

        int GetFrameCount();

        void SetFramePosition(int frameIdx);

        int GetCurrentFramePosition();

        void Release();

        double GetFrameRate();

        cv::Size GetFrameSize();

        cv::Size GetOriginalFrameSize();

        double GetProperty(int propId);

        bool SetProperty(int propId, double value);

        int GetFourCharCodecCode();

        void ReverseTransform(MPFVideoTrack &videoTrack);

        void ReverseTransform(MPFImageLocation &imageLocation);

    private:
        cv::VideoCapture cvVideoCapture_;
        const IFrameTransformer::Ptr frameTransformer_;

        double GetPropertyInternal(int propId);

        IFrameTransformer::Ptr GetFrameTransformer(const MPFJob &job);
    };
}}

#endif //OPENMPF_CPP_COMPONENT_SDK_MPFVIDEOCAPTURE_H