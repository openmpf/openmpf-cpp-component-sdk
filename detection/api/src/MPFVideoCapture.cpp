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

#include <algorithm>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>

#include "detectionComponentUtils.h"
#include "FeedForwardFrameFilter.h"
#include "frame_transformers/FrameTransformerFactory.h"
#include "frame_transformers/NoOpFrameTransformer.h"
#include "IntervalFrameFilter.h"

#include "MPFVideoCapture.h"



namespace MPF { namespace COMPONENT {
    using cv::VideoCaptureProperties;


    MPFVideoCapture::MPFVideoCapture(const MPFVideoJob &videoJob, bool enableFrameTransformers,
                                     bool enableFrameFiltering)
        : cvVideoCapture_(videoJob.data_uri)
        , frameFilter_(GetFrameFilter(enableFrameFiltering, videoJob, cvVideoCapture_))
        , frameTransformer_(GetFrameTransformer(enableFrameTransformers, videoJob)) {

        SetFramePosition(0);
    }


    IFrameTransformer::Ptr MPFVideoCapture::GetFrameTransformer(bool enableFrameTransformers,
                                                                const MPFVideoJob &job) const {
        if (enableFrameTransformers) {
            return FrameTransformerFactory::GetTransformer(job, GetOriginalFrameSize());
        }
        return IFrameTransformer::Ptr(new NoOpFrameTransformer(GetOriginalFrameSize()));
    }


    FrameFilter::CPtr MPFVideoCapture::GetFrameFilter(bool frameFilteringEnabled, const MPFVideoJob &job,
                                                      const cv::VideoCapture &cvVideoCapture) {

        int frameCount = GetFrameCount(job, cvVideoCapture);
        if (!frameFilteringEnabled) {
            return FrameFilter::GetNoOpFilter(frameCount);
        }
        if (!job.has_feed_forward_track) {
            return FrameFilter::CPtr(new IntervalFrameFilter(job, frameCount));
        }

        if (job.feed_forward_track.frame_locations.empty()) {
            throw std::length_error(
                    "MPFVideoJob::has_feed_forward_track is true for Job: "
                    + job.job_name + ", but the feed forward track is empty.");
        }

        int firstTrackFrame = job.feed_forward_track.frame_locations.begin()->first;
        int lastTrackFrame = job.feed_forward_track.frame_locations.rbegin()->first;
        if (firstTrackFrame != job.start_frame || lastTrackFrame != job.stop_frame) {
            std::cerr << "The feed forward track for Job: " << job.job_name
                << " starts at frame " << firstTrackFrame << " and ends at frame " << lastTrackFrame
                << ", but MPFVideoJob::start_frame = " << job.start_frame
                << " and MPFVideoJob::stop_frame = " << job.stop_frame
                << ". MPFVideoJob::has_feed_forward_track is true so the entire feed forward track will be used."
                << std::endl;
        }

        return FrameFilter::CPtr(new FeedForwardFrameFilter(job.feed_forward_track));
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
        return frameFilter_->GetSegmentFrameCount();
    }


    bool MPFVideoCapture::SetFramePosition(int frameIdx) {
        if (frameIdx < 0 || frameIdx >= frameFilter_->GetSegmentFrameCount()) {
            return false;
        }

        int originalPos = frameFilter_->SegmentToOriginalFramePosition(frameIdx);
        return UpdateOriginalFramePosition(originalPos);
    }


    bool MPFVideoCapture::UpdateOriginalFramePosition(int requestedOriginalPosition) {
        if (framePosition_ == requestedOriginalPosition) {
            return true;
        }

        if (!seekStrategy_) {
            return false;
        }

        framePosition_ = seekStrategy_->ChangePosition(cvVideoCapture_, framePosition_, requestedOriginalPosition);
        if (framePosition_ == requestedOriginalPosition) {
            return true;
        }

        return SeekFallback() && UpdateOriginalFramePosition(requestedOriginalPosition);
    }


    bool MPFVideoCapture::SeekFallback() {
        if (!seekStrategy_) {
            return false;
        }

        seekStrategy_ = seekStrategy_->fallback();
        if (!seekStrategy_) {
            return false;
        }

        // In order to fallback to a different seek strategy, cvVideoCapture_ must be capable of setting the
        // frame position to 0.
        bool wasSet = SetPropertyInternal(VideoCaptureProperties::CAP_PROP_POS_FRAMES, 0);
        if (wasSet) {
            framePosition_ = 0;
            return true;
        }

        seekStrategy_ = nullptr;
        return false;
    }


    int MPFVideoCapture::GetCurrentFramePosition() const {
        return frameFilter_->OriginalToSegmentFramePosition(framePosition_);
    }


    bool MPFVideoCapture::Read(cv::Mat &frame) {
        int originalPosBeforeRead = framePosition_;
        if (frameFilter_->IsPastEndOfSegment(originalPosBeforeRead)) {
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
        if (!frameFilter_->IsPastEndOfSegment(framePosition_)) {
            int segPosBeforeRead = frameFilter_->OriginalToSegmentFramePosition(framePosition_ - 1);
            int nextOriginalFrame = frameFilter_->SegmentToOriginalFramePosition(segPosBeforeRead + 1);
            // At this point a frame was successfully read. If UpdateOriginalFramePosition does not
            // succeed that means it is not possible to read any more frames from the video, so all future
            // reads will fail.
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
        return frameFilter_->GetSegmentFrameRate(originalFrameRate);
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
        return frameFilter_->GetSegmentFramePositionRatio(framePosition_);
    }


    bool MPFVideoCapture::SetFramePositionRatio(double positionRatio) {
        if (positionRatio < 0 || positionRatio > 1) {
            return false;
        }
        int framePos = frameFilter_->RatioToOriginalFramePosition(positionRatio);
        return UpdateOriginalFramePosition(framePos);
    }



    double MPFVideoCapture::GetCurrentTimeInMillis() const {
        double originalFrameRate = GetPropertyInternal(VideoCaptureProperties::CAP_PROP_FPS);
        return frameFilter_->GetCurrentSegmentTimeInMillis(framePosition_, originalFrameRate);
    }


    bool MPFVideoCapture::SetFramePositionInMillis(double milliseconds) {
        double originalFrameRate = GetPropertyInternal(VideoCaptureProperties::CAP_PROP_FPS);
        return SetFramePosition(frameFilter_->MillisToSegmentFramePosition(originalFrameRate, milliseconds));
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
        videoTrack.start_frame = frameFilter_->SegmentToOriginalFramePosition(videoTrack.start_frame);
        videoTrack.stop_frame = frameFilter_->SegmentToOriginalFramePosition(videoTrack.stop_frame);

        std::map<int, MPFImageLocation> newFrameLocations;
        for (auto &frameLocationPair : videoTrack.frame_locations) {
            frameTransformer_->ReverseTransform(frameLocationPair.second);

            int fixedFrameIndex = frameFilter_->SegmentToOriginalFramePosition(frameLocationPair.first);
            newFrameLocations[fixedFrameIndex] = frameLocationPair.second;
        }

        videoTrack.frame_locations = std::move(newFrameLocations);
    }



    std::vector<cv::Mat> MPFVideoCapture::GetInitializationFramesIfAvailable(int numberOfRequestedFrames) {
        int initFramesAvailable = frameFilter_->GetAvailableInitializationFrameCount();
        int numFramesToGet = std::min(initFramesAvailable, numberOfRequestedFrames);
        if (numFramesToGet < 1) {
            return {};
        }


        int initialFramePos = framePosition_;

        int firstInitFrameIdx = frameFilter_->SegmentToOriginalFramePosition(-1 * numFramesToGet);
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

        // If UpdateOriginalFramePosition does not succeed that means it is not possible to read any more frames
        // from the video, so all future reads will fail. If any initialization frames were read, they
        // will be returned.
        UpdateOriginalFramePosition(initialFramePos);

        return initializationFrames;
    }
}}
