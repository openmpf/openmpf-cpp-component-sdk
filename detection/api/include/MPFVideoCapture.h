/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2021 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2021 The MITRE Corporation                                       *
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

#include <memory>
#include <string>
#include <vector>

#include <opencv2/videoio.hpp>
#include <opencv2/core.hpp>

#include "frame_transformers/IFrameTransformer.h"
#include "FrameFilter.h"
#include "MPFDetectionComponent.h"
#include "SeekStrategy.h"


namespace MPF { namespace COMPONENT {

    class ReverseTransformer;

    class MPFVideoCapture {

    public:
        /**
         * Initializes a new MPFVideoCapture instance, using the frame
         * transformers specified in jobProperties, to be used for video
         * processing jobs.
         * @param videoJob
         * @param enableFrameTransformers Automatically transform frames based on job properties
         * @param enableFrameFiltering Automatically skip frames based on job properties
         * @throws std::invalid_argument videoJob contains invalid property
         */
        explicit MPFVideoCapture(const MPFVideoJob &videoJob, bool enableFrameTransformers=true,
                                 bool enableFrameFiltering=true);

        explicit MPFVideoCapture(std::string videoPath);


        bool Read(cv::Mat &frame);

        MPFVideoCapture &operator>>(cv::Mat &frame);

        bool IsOpened() const;

        int GetFrameCount() const;

        bool SetFramePosition(int frameIdx);

        int GetCurrentFramePosition() const;

        /**
         * Release the underlying cv::VideoCapture. It is generally not necessary call this
         * manually, because the destructor will take care of it. This only needs to be called if
         * you want to release resources prior to destroying MPFVideoCapture.
         */
        void Release();

        double GetFrameRate() const;

        cv::Size GetFrameSize() const;

        cv::Size GetOriginalFrameSize() const;

        double GetFramePositionRatio() const;

        bool SetFramePositionRatio(double positionRatio);

        double GetCurrentTimeInMillis() const;

        bool SetFramePositionInMillis(double milliseconds);

        double GetProperty(int propId) const;

        bool SetProperty(int propId, double value);

        int GetFourCharCodecCode() const;

        void ReverseTransform(MPFVideoTrack &videoTrack) const;

        /**
         * @return An object that can do the reverse transform even after MPFVideoCapture has been
         *         destroyed
         */
        ReverseTransformer GetReverseTransformer() const;

        /**
         * Gets up to numberOfRequestedFrames frames before beginning of segment, skipping frameInterval frames.
         * If less than numberOfRequestedFrames are available, returned vector will have as many initialization frames
         * as are available.
         * If the job's start frame is less than the frame interval, the returned vector will be empty.
         * @param numberOfRequestedFrames
         * @return Vector that contains between 0 and numberOfRequestedFrames frames
         */
        std::vector<cv::Mat> GetInitializationFramesIfAvailable(int numberOfRequestedFrames);



    private:
        std::string videoPath_;

        cv::VideoCapture cvVideoCapture_;

        std::shared_ptr<const FrameFilter> frameFilter_;

        std::shared_ptr<const IFrameTransformer> frameTransformer_;

        SeekStrategy::CPtr seekStrategy_;

        /**
         * MPFVideoCapture keeps track of the frame position instead of depending on
         * cv::VideoCapture::get(cv::VideoCaptureProperties::CAP_PROP_POS_FRAMES)
         * because for certain videos it does not correctly report the frame position.
         */
        int framePosition_ = 0;


        double GetPropertyInternal(int propId) const;

        bool SetPropertyInternal(int propId, double value);

        IFrameTransformer::Ptr GetFrameTransformer(bool frameTransformersEnabled, const MPFVideoJob &job) const;

        bool ReadAndTransform(cv::Mat &frame);

        void MoveToNextFrameInSegment();

        bool SeekFallback();

        /**
         * Attempts to update the frame position using seekStrategy_. If the current seekStrategy_ fails,
         * it will attempt to fall back to the next SeekStrategy until it tries all the strategies.
         * If this method fails that means it will have attempted to use ReadSeek. If ReadSeek fails,
         * then it is not possible to read the video any further.
         * @param newOriginalFramePosition
         * @return true if the frame position was successfully set to newOriginalFramePosition
         */
        bool UpdateOriginalFramePosition(int newOriginalFramePosition);

        static int GetFrameCount(const MPFVideoJob &job, const cv::VideoCapture &cvVideoCapture);

        static FrameFilter::CPtr GetFrameFilter(bool frameFilteringEnabled, const MPFVideoJob &job,
                                                const cv::VideoCapture &cvVideoCapture);

        static SeekStrategy::CPtr GetSeekStrategy(const MPFVideoJob &job);
    };



    class ReverseTransformer {
    public:
        ReverseTransformer(std::shared_ptr<const IFrameTransformer> frameTransformer,
                           std::shared_ptr<const FrameFilter> frameFilter);

        void operator()(MPFVideoTrack &track) const;

        static void ReverseTransform(MPFVideoTrack &track,
                                     const IFrameTransformer& frameTransformer,
                                     const FrameFilter& frameFilter);

    private:
        std::shared_ptr<const IFrameTransformer> frameTransformer_;
        std::shared_ptr<const FrameFilter> frameFilter_;
    };
}}

#endif //OPENMPF_CPP_COMPONENT_SDK_MPFVIDEOCAPTURE_H
