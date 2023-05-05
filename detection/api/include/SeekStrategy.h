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


#ifndef OPENMPF_CPP_COMPONENT_SDK_SEEKSTRATEGY_H
#define OPENMPF_CPP_COMPONENT_SDK_SEEKSTRATEGY_H


#include <opencv2/videoio.hpp>
#include <memory>

namespace MPF { namespace COMPONENT {

    /**
     * For certain videos cv::VideoCapture::set(cv::VideoCaptureProperties::CAP_PROP_POS_FRAMES, int)
     * does not work properly. If MPFVideoCapture detects that cv::VideoCapture is not setting the frame position
     * properly it will fallback to different SeekStrategy.
     */
    class SeekStrategy {
    public:
        typedef std::unique_ptr<const SeekStrategy> CPtr;

        virtual ~SeekStrategy() = default;

        virtual int ChangePosition(cv::VideoCapture &cap, int currentPosition, int requestedPosition) const = 0;

        virtual SeekStrategy::CPtr fallback() const = 0;
    };


    class SequentialSeek : public SeekStrategy {
    public:
        int ChangePosition(cv::VideoCapture &cap, int currentPosition, int requestedPosition) const final;

    private:
        virtual bool Advance(cv::VideoCapture &cap) const = 0;
    };



    class GrabSeek : public SequentialSeek {
    public:
        SeekStrategy::CPtr fallback() const override;

    private:
        bool Advance(cv::VideoCapture &cap) const override;
    };


    class ReadSeek : public SequentialSeek {
    public:
        SeekStrategy::CPtr fallback() const override;

    private:
        bool Advance(cv::VideoCapture &cap) const override;
    };


    class SetFramePositionSeek : public SeekStrategy {
    public:
        int ChangePosition(cv::VideoCapture &cap, int currentPosition, int requestedPosition) const override;

        SeekStrategy::CPtr fallback() const override;

    private:
        GrabSeek grabSeek_;

        // When setting frame position, OpenCV sets the frame position to 16 frames before the
        // requested frame in order to locate the closest key frame. Once OpenCV locates the key
        // frame, it uses cv::VideoCapture::grab to advance cv::VideoCapture's position.
        // This means that when you need to advance 16 or fewer frames, it is more efficient to
        // just use cv::VideoCapture::grab.
        // https://github.com/opencv/opencv/blob/4.5.0/modules/videoio/src/cap_ffmpeg_impl.hpp#L1459
        static constexpr int SET_POS_MIN_FRAMES = 16;
    };
}}



#endif //OPENMPF_CPP_COMPONENT_SDK_SEEKSTRATEGY_H
