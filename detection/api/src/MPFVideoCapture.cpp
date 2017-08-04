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

#include "frame_transformers/FrameTransformerFactory.h"
#include "frame_transformers/NoOpFrameTransformer.h"

#include "MPFVideoCapture.h"
#include "detectionComponentUtils.h"



namespace MPF { namespace COMPONENT {



    MPFVideoCapture::MPFVideoCapture(const MPFVideoJob &videoJob, bool enableFrameTransformers,
                                     bool enableFrameSkipper)
        : cvVideoCapture_(videoJob.data_uri)
        , frameSkipper_(GetFrameSkipper(enableFrameSkipper, videoJob, cvVideoCapture_))
        , frameTransformer_(GetFrameTransformer(enableFrameTransformers, videoJob)){

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
        int frameCount = DetectionComponentUtils::GetProperty<int>(job.media_properties, "FRAME_COUNT", -1);
        if (frameCount > 0) {
            return frameCount;
        }

        return (int) cvVideoCapture.get(cv::CAP_PROP_FRAME_COUNT);
    }




    bool MPFVideoCapture::IsOpened() const {
        return cvVideoCapture_.isOpened();
    }

    int MPFVideoCapture::GetFrameCount() const {
        return frameSkipper_.GetSegmentFrameCount();
    }


    bool MPFVideoCapture::SetFramePosition(int frameIdx) {
        if (frameIdx < 0 || frameIdx > frameSkipper_.GetSegmentFrameCount()) {
            return false;
        }

        int originalPos = frameSkipper_.SegmentToOriginalFramePosition(frameIdx);
        return SetPropertyInternal(cv::CAP_PROP_POS_FRAMES, originalPos);
    }


    int MPFVideoCapture::GetCurrentFramePosition() const {
        int originalFramePosition = GetOriginalFramePosition();
        return frameSkipper_.OriginalToSegmentFramePosition(originalFramePosition);
    }

    int MPFVideoCapture::GetOriginalFramePosition() const {
        return (int) GetPropertyInternal(cv::CAP_PROP_POS_FRAMES);
    }


    bool MPFVideoCapture::Read(cv::Mat &frame) {
        int originalPosBeforeRead = GetOriginalFramePosition();
        if (frameSkipper_.IsPastEndOfSegment(originalPosBeforeRead)) {
            frame.release();
            return false;
        }

        bool wasRead = ReadAndTransform(frame);
        if (wasRead) {
            int segPosBeforeRead = frameSkipper_.OriginalToSegmentFramePosition(originalPosBeforeRead);
            int nextOriginalFrame = frameSkipper_.SegmentToOriginalFramePosition(segPosBeforeRead + 1);
            if (nextOriginalFrame != originalPosBeforeRead + 1) {
                // Only set property if we skipped more than one frame.
                SetPropertyInternal(cv::CAP_PROP_POS_FRAMES, nextOriginalFrame);
            }
        }
        return wasRead;
    }



    bool MPFVideoCapture::ReadAndTransform(cv::Mat &frame) {
        bool wasRead = cvVideoCapture_.read(frame);
        if (wasRead) {
            frameTransformer_->TransformFrame(frame);
        }
        return wasRead;
    }


    MPFVideoCapture &MPFVideoCapture::operator>>(cv::Mat &frame) {
        Read(frame);
        return *this;
    }

    void MPFVideoCapture::Release() {
        cvVideoCapture_.release();
    }

    double MPFVideoCapture::GetFrameRate() const {
        double originalFrameRate = GetPropertyInternal(cv::CAP_PROP_FPS);
        return frameSkipper_.GetSegmentFrameRate(originalFrameRate);
    }


    cv::Size MPFVideoCapture::GetFrameSize() const {
        return frameTransformer_->GetFrameSize();
    }

    cv::Size MPFVideoCapture::GetOriginalFrameSize() const {
        int width = (int) GetPropertyInternal(cv::CAP_PROP_FRAME_WIDTH);
        int height = (int) GetPropertyInternal(cv::CAP_PROP_FRAME_HEIGHT);
        return cv::Size(width, height);
    }


    double MPFVideoCapture::GetFramePositionRatio() const {
        return frameSkipper_.GetSegmentFramePositionRatio(GetOriginalFramePosition());
    }


    bool MPFVideoCapture::SetFramePositionRatio(double positionRatio) {
        if (positionRatio < 0 || positionRatio > 1) {
            return false;
        }
        int framePos = frameSkipper_.RatioToOriginalFramePosition(positionRatio);
        return SetPropertyInternal(cv::CAP_PROP_POS_FRAMES, framePos);
    }


    double MPFVideoCapture::GetCurrentTimeInMillis() const {
        double originalFrameRate = GetPropertyInternal(cv::CAP_PROP_FPS);
        return frameSkipper_.GetCurrentSegmentTimeInMillis(GetOriginalFramePosition(), originalFrameRate);
    }


    bool MPFVideoCapture::SetFramePositionInMillis(double milliseconds) {
        double originalFrameRate = GetPropertyInternal(cv::CAP_PROP_FPS);
        return SetFramePosition(frameSkipper_.MillisToSegmentFramePosition(originalFrameRate, milliseconds));
    }


    double MPFVideoCapture::GetProperty(int propId) const {
        switch (propId) {
            case cv::CAP_PROP_FRAME_WIDTH:
                return GetFrameSize().width;
            case cv::CAP_PROP_FRAME_HEIGHT:
                return GetFrameSize().height;
            case cv::CAP_PROP_POS_FRAMES:
                return GetCurrentFramePosition();
            case cv::CAP_PROP_FPS:
                return GetFrameRate();
            case cv::CAP_PROP_POS_AVI_RATIO:
                return GetFramePositionRatio();
            case cv::CAP_PROP_POS_MSEC:
                return GetCurrentTimeInMillis();
            default:
                return GetPropertyInternal(propId);
        }
    }

    bool MPFVideoCapture::SetProperty(int propId, double value) {
        switch (propId) {
            case cv::CAP_PROP_POS_FRAMES:
                return SetFramePosition((int) value);
            case cv::CAP_PROP_POS_AVI_RATIO:
                return SetFramePositionRatio(value);
            case cv::CAP_PROP_POS_MSEC:
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
        return (int) GetPropertyInternal(cv::CAP_PROP_FOURCC);
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
        int firstFrame;
        int frameCount;
        frameSkipper_.GetAvailableInitializationFrames(numberOfRequestedFrames, firstFrame, frameCount);
        if (frameCount < 1) {
            return {};
        }

        int initialFramePos = (int) GetPropertyInternal(cv::CAP_PROP_POS_FRAMES);

        std::vector<cv::Mat> initializationFrames;
        for (int i = firstFrame; i < (firstFrame + frameCount); i++) {
            SetPropertyInternal(cv::CAP_PROP_POS_FRAMES, i);
            cv::Mat frame;
            if (ReadAndTransform(frame)) {
                initializationFrames.push_back(frame);
            }
        }

        SetPropertyInternal(cv::CAP_PROP_POS_FRAMES, initialFramePos);

        return initializationFrames;
    }

}}
