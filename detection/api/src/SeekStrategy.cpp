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


#include <iostream>
#include "SeekStrategy.h"

namespace MPF { namespace COMPONENT {

    int SetFramePositionSeek::ChangePosition(cv::VideoCapture &cap, int currentPosition, int requestedPosition) const {
        int frameDiff = requestedPosition - currentPosition;
        if (frameDiff > 0 && frameDiff <= SET_POS_MIN_FRAMES) {
            return grabSeek_.ChangePosition(cap, currentPosition, requestedPosition);
        }
        else if (cap.set(cv::VideoCaptureProperties::CAP_PROP_POS_FRAMES, requestedPosition)) {
            return requestedPosition;
        }
        else {
            return currentPosition;
        }
    }

    SeekStrategy::CPtr SetFramePositionSeek::fallback() const {
        std::cerr << "SetFramePositionSeek failed: falling back to GrabSeek" << std::endl;
        return SeekStrategy::CPtr(new GrabSeek);
    }




    int SequentialSeek::ChangePosition(cv::VideoCapture &cap, int currentPosition, int requestedPosition) const {
        bool newPositionInFuture = requestedPosition > currentPosition;

        int start;
        if (newPositionInFuture) {
            start = currentPosition;
        }
        else {
            // The current position is past the requested position. We need to start reading from the very
            // beginning of the video. If it were possible to set the frame position to a frame in the middle
            // of the video, then SetFramePositionSeek would not have failed.
            if (!cap.set(cv::VideoCaptureProperties::CAP_PROP_POS_FRAMES, 0)) {
                return currentPosition;
            }
            start = 0;
        }

        int numFramesToDiscard = requestedPosition - start;

        int numSuccess = 0;
        for (int i = 0; i < numFramesToDiscard; i++) {
            if (Advance(cap)) {
                numSuccess++;
            }
            else {
                break;
            }
        }

        return start + numSuccess;
    }



    bool GrabSeek::Advance(cv::VideoCapture &cap) const {
        return cap.grab();
    }

    SeekStrategy::CPtr GrabSeek::fallback() const {
        std::cerr << "GrabSeek failed: falling back to ReadSeek" << std::endl;
        return SeekStrategy::CPtr(new ReadSeek);
    }



    bool ReadSeek::Advance(cv::VideoCapture &cap) const {
        cv::Mat frame;
        return cap.read(frame);
    }

    SeekStrategy::CPtr ReadSeek::fallback() const {
        std::cerr << "ReadSeek failed: No more fallbacks" << std::endl;
        return SeekStrategy::CPtr(nullptr);
    }
}}
