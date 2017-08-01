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
#include "FrameSkipper.h"


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
        explicit MPFVideoCapture(const MPFVideoJob &videoJob, bool enableFrameTransformers=true,
                                 bool enableFrameSkipper=false);

        /**
         * Initializes a new MPFVideoCapture instance, using the frame
         * transformers specified in jobProperties, to be used for image
         * processing jobs.
         * @param imageJob
         * @throws std::invalid_argument imageJob contains invalid property
         */
        explicit MPFVideoCapture(const MPFImageJob &imageJob,  bool enableFrameTransformers=true);

        bool Read(cv::Mat &frame);

        MPFVideoCapture &operator>>(cv::Mat &frame);

        bool IsOpened() const;

        int GetFrameCount();

        bool SetFramePosition(int frameIdx);


        int GetCurrentFramePosition();

        int GetOriginalFramePosition();

        void Release();

        double GetFrameRate();

        cv::Size GetFrameSize();

        cv::Size GetOriginalFrameSize();

        double GetFramePositionRatio();

        bool SetFramePositionRatio(double positionRatio);

        double GetCurrentTimeInMillis();

        bool SetPositionInMillis(double milliseconds);

        double GetProperty(int propId);

        bool SetProperty(int propId, double value);

        int GetFourCharCodecCode();

        void ReverseTransform(MPFVideoTrack &videoTrack);

        std::vector<cv::Mat> GetInitializationFramesIfAvailable(int numberOfRequestedFrames);



    private:
        cv::VideoCapture cvVideoCapture_;

        const FrameSkipper frameSkipper_;

        IFrameTransformer::Ptr frameTransformer_;

        double GetPropertyInternal(int propId);

        bool SetPropertyInternal(int propId, double value);

        IFrameTransformer::Ptr GetFrameTransformer(bool frameTransformersEnabled, const MPFJob &job);

        static int GetFrameCount(const MPFVideoJob &job, const cv::VideoCapture &cvVideoCapture);

        static FrameSkipper GetFrameSkipper(bool frameSkippingEnabled, const MPFVideoJob &job,
                                            const cv::VideoCapture &cvVideoCapture);

        bool ReadAndTransform(cv::Mat &frame);
    };
}}

#endif //OPENMPF_CPP_COMPONENT_SDK_MPFVIDEOCAPTURE_H
