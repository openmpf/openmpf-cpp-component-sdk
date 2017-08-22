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

#include "frame_transformers/FrameTransformerFactory.h"
#include "frame_transformers/NoOpFrameTransformer.h"

#include "MPFVideoCapture.h"
#include "detectionComponentUtils.h"



namespace MPF { namespace COMPONENT {
    using cv::VideoCaptureProperties;


    MPFVideoCapture::MPFVideoCapture(const MPFVideoJob &videoJob, bool enableFrameTransformers,
                                     bool enableFrameSkipper)
        : cvVideoCapture_(videoJob.data_uri)
        , frameSkipper_(GetFrameSkipper(enableFrameSkipper, videoJob, cvVideoCapture_))
        , frameTransformer_(GetFrameTransformer(enableFrameTransformers, videoJob)) {

        SetFramePosition(0);
    }


    MPFVideoCapture::MPFVideoCapture(const MPFImageJob &imageJob,  bool enableFrameTransformers)
            : cvVideoCapture_(imageJob.data_uri)
            , frameSkipper_(0, 1, 1)
            , frameTransformer_(GetFrameTransformer(enableFrameTransformers, imageJob)) {
    }



    IFrameTransformer::Ptr MPFVideoCapture::GetFrameTransformer(bool enableFrameTransformers, const MPFJob &job) const {
        if (enableFrameTransformers) {
            return FrameTransformerFactory::GetTransformer(job, GetOriginalFrameSize());
        }
        return IFrameTransformer::Ptr(new NoOpFrameTransformer(GetOriginalFrameSize()));
    }


    FrameSkipper MPFVideoCapture::GetFrameSkipper(bool enableFrameSkipper, const MPFVideoJob &job,
                                                  const cv::VideoCapture &cvVideoCapture) {

        int frameCount = GetFrameCount(job, cvVideoCapture);
        if (enableFrameSkipper) {
            return {job, frameCount};
        }
        return {0, frameCount - 1, 1};
    }


    int MPFVideoCapture::GetFrameCount(const MPFVideoJob &job, const cv::VideoCapture &cvVideoCapture) {
        // use the frame count provided by the media inspector if possible
        int frameCount = DetectionComponentUtils::GetProperty(job.media_properties, "FRAME_COUNT", -1);
        if (frameCount > 0) {
            return frameCount;
        }

        return static_cast<int>(cvVideoCapture.get(VideoCaptureProperties::CAP_PROP_FRAME_COUNT));
    }



    bool MPFVideoCapture::IsOpened() const {
        return cvVideoCapture_.isOpened();
    }

    int MPFVideoCapture::GetFrameCount() const {
        return frameSkipper_.GetSegmentFrameCount();
    }


    bool MPFVideoCapture::SetFramePosition(int frameIdx) {
        if (frameIdx < 0 || frameIdx >= frameSkipper_.GetSegmentFrameCount()) {
            return false;
        }

        int originalPos = frameSkipper_.SegmentToOriginalFramePosition(frameIdx);
        return UpdateOriginalFramePosition(originalPos);
    }



    bool MPFVideoCapture::UpdateOriginalFramePosition(int requestedOriginalPosition) {
        if (framePosition_ == requestedOriginalPosition) {
            return true;
        }

        while (seekStrategy_) {

            framePosition_ = seekStrategy_->ChangePosition(cvVideoCapture_, framePosition_, requestedOriginalPosition);
            if (framePosition_ == requestedOriginalPosition) {
                return true;
            }

            SeekFallback();
        }
        return false;
    }


    bool MPFVideoCapture::SeekFallback() {
        if (!seekStrategy_) {
            return false;
        }

        seekStrategy_ = seekStrategy_->fallback();
        if (!seekStrategy_) {
            return false;
        }

        bool wasSet = SetPropertyInternal(VideoCaptureProperties::CAP_PROP_POS_FRAMES, 0);
        if (wasSet) {
            framePosition_ = 0;
            return true;
        }

        seekStrategy_ = nullptr;
        return false;
    }


    int MPFVideoCapture::GetCurrentFramePosition() const {
        return frameSkipper_.OriginalToSegmentFramePosition(framePosition_);
    }


    bool MPFVideoCapture::Read(cv::Mat &frame) {
        int originalPosBeforeRead = framePosition_;
        if (frameSkipper_.IsPastEndOfSegment(originalPosBeforeRead)) {
            frame.release();
            return false;
        }

        bool wasRead = ReadAndTransform(frame);
        if (wasRead) {
            MoveToNextFrameInSegment();
            return true;
        }

        if (SeekFallback() && UpdateOriginalFramePosition(originalPosBeforeRead)) {
            return Read(frame);
        }
        return false;
    }


    bool MPFVideoCapture::ReadAndTransform(cv::Mat &frame) {
        bool wasRead = cvVideoCapture_.read(frame);
        if (wasRead) {
            framePosition_++;
            frameTransformer_->TransformFrame(frame);
        }
        return wasRead;
    }



    void MPFVideoCapture::MoveToNextFrameInSegment() {
        if (!frameSkipper_.IsPastEndOfSegment(framePosition_)) {
            int segPosBeforeRead = frameSkipper_.OriginalToSegmentFramePosition(framePosition_ - 1);
            int nextOriginalFrame = frameSkipper_.SegmentToOriginalFramePosition(segPosBeforeRead + 1);
            UpdateOriginalFramePosition(nextOriginalFrame);
        }
    }



    MPFVideoCapture &MPFVideoCapture::operator>>(cv::Mat &frame) {
        Read(frame);
        return *this;
    }

    void MPFVideoCapture::Release() {
        cvVideoCapture_.release();
    }

    double MPFVideoCapture::GetFrameRate() const {
        double originalFrameRate = GetPropertyInternal(VideoCaptureProperties::CAP_PROP_FPS);
        return frameSkipper_.GetSegmentFrameRate(originalFrameRate);
    }


    cv::Size MPFVideoCapture::GetFrameSize() const {
        return frameTransformer_->GetFrameSize();
    }

    cv::Size MPFVideoCapture::GetOriginalFrameSize() const {
        auto width = static_cast<int>(GetPropertyInternal(VideoCaptureProperties::CAP_PROP_FRAME_WIDTH));
        auto height = static_cast<int>(GetPropertyInternal(VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT));
        return {width, height};
    }


    double MPFVideoCapture::GetFramePositionRatio() const {
        return frameSkipper_.GetSegmentFramePositionRatio(framePosition_);
    }


    bool MPFVideoCapture::SetFramePositionRatio(double positionRatio) {
        if (positionRatio < 0 || positionRatio > 1) {
            return false;
        }
        int framePos = frameSkipper_.RatioToOriginalFramePosition(positionRatio);
        return UpdateOriginalFramePosition(framePos);
    }



    double MPFVideoCapture::GetCurrentTimeInMillis() const {
        double originalFrameRate = GetPropertyInternal(VideoCaptureProperties::CAP_PROP_FPS);
        return frameSkipper_.GetCurrentSegmentTimeInMillis(framePosition_, originalFrameRate);
    }


    bool MPFVideoCapture::SetFramePositionInMillis(double milliseconds) {
        double originalFrameRate = GetPropertyInternal(VideoCaptureProperties::CAP_PROP_FPS);
        return SetFramePosition(frameSkipper_.MillisToSegmentFramePosition(originalFrameRate, milliseconds));
    }


    double MPFVideoCapture::GetProperty(int propId) const {
        switch (propId) {
            case VideoCaptureProperties::CAP_PROP_FRAME_WIDTH:
                return GetFrameSize().width;
            case VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT:
                return GetFrameSize().height;
            case VideoCaptureProperties::CAP_PROP_FPS:
                return GetFrameRate();
            case VideoCaptureProperties::CAP_PROP_POS_FRAMES:
                return GetCurrentFramePosition();
            case VideoCaptureProperties::CAP_PROP_POS_AVI_RATIO:
                return GetFramePositionRatio();
            case VideoCaptureProperties::CAP_PROP_POS_MSEC:
                return GetCurrentTimeInMillis();
            default:
                return GetPropertyInternal(propId);
        }
    }

    bool MPFVideoCapture::SetProperty(int propId, double value) {
        switch (propId) {
            case VideoCaptureProperties::CAP_PROP_FRAME_WIDTH:
            case VideoCaptureProperties::CAP_PROP_FRAME_HEIGHT:
            case VideoCaptureProperties::CAP_PROP_FPS:
                return false;
            case VideoCaptureProperties::CAP_PROP_POS_FRAMES:
                return SetFramePosition(static_cast<int>(value));
            case VideoCaptureProperties::CAP_PROP_POS_AVI_RATIO:
                return SetFramePositionRatio(value);
            case VideoCaptureProperties::CAP_PROP_POS_MSEC:
                return SetFramePositionInMillis(value);
            default:
                return SetPropertyInternal(propId, value);
        }
    }


    double MPFVideoCapture::GetPropertyInternal(int propId) const {
        return cvVideoCapture_.get(propId);
    }

    bool MPFVideoCapture::SetPropertyInternal(int propId, double value) {
        return cvVideoCapture_.set(propId, value);
    }


    int MPFVideoCapture::GetFourCharCodecCode() const {
        return static_cast<int>(GetPropertyInternal(VideoCaptureProperties::CAP_PROP_FOURCC));
    }


    void MPFVideoCapture::ReverseTransform(MPFVideoTrack &videoTrack) const {
        videoTrack.start_frame = frameSkipper_.SegmentToOriginalFramePosition(videoTrack.start_frame);
        videoTrack.stop_frame = frameSkipper_.SegmentToOriginalFramePosition(videoTrack.stop_frame);

        std::map<int, MPFImageLocation> newFrameLocations;
        for (auto &frameLocationPair : videoTrack.frame_locations) {
            frameTransformer_->ReverseTransform(frameLocationPair.second);

            int fixedFrameIndex = frameSkipper_.SegmentToOriginalFramePosition(frameLocationPair.first);
            newFrameLocations[fixedFrameIndex] = frameLocationPair.second;
        }

        videoTrack.frame_locations = std::move(newFrameLocations);
    }



    std::vector<cv::Mat> MPFVideoCapture::GetInitializationFramesIfAvailable(int numberOfRequestedFrames) {
        int initFramesAvailable = frameSkipper_.GetAvailableInitializationFrameCount();
        int numFramesToGet = std::min(initFramesAvailable, numberOfRequestedFrames);
        if (numFramesToGet < 1) {
            return {};
        }


        int initialFramePos = framePosition_;

        int firstInitFrameIdx = frameSkipper_.SegmentToOriginalFramePosition(-1 * numFramesToGet);
        if (!UpdateOriginalFramePosition(firstInitFrameIdx)) {
            return {} ;
        }

        std::vector<cv::Mat> initializationFrames;
        for (int i = 0; i < numFramesToGet; i++) {
            cv::Mat frame;
            if (Read(frame)) {
                initializationFrames.push_back(std::move(frame));
            }
        }

        UpdateOriginalFramePosition(initialFramePos);

        return initializationFrames;
    }
}}
