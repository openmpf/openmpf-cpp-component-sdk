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

#include <gtest/gtest.h>
#include <vector>

#include <opencv2/video.hpp>

#include "MPFVideoCapture.h"
#include "FrameSkipper.h"

using namespace MPF::COMPONENT;
using std::map;
using std::vector;




void assertFrameCount(FrameSkipper frameSkipper, int expected) {
    ASSERT_EQ(expected, frameSkipper.GetSegmentFrameCount());
}

TEST(FrameSkipTest, CanCalculateFrameCount) {

    assertFrameCount({0, 20, 4}, 6);
    assertFrameCount({0, 20, 3}, 7);
    assertFrameCount({4, 10 , 1}, 7);
    assertFrameCount({4, 10 , 2}, 4);
    assertFrameCount({4, 10, 3}, 3);
    assertFrameCount({4, 10, 20}, 1);
    assertFrameCount({0, 9, 1}, 10);
    assertFrameCount({0, 9, 2}, 5);
    assertFrameCount({0, 9, 3}, 4);
    assertFrameCount({3, 22, 7}, 3);
    assertFrameCount({3, 22, 4}, 5);
}



void assertSegmentToOriginal(FrameSkipper skipper, int segmentIdx, int expectedOriginalIdx) {
    ASSERT_EQ(expectedOriginalIdx, skipper.SegmentToOriginalFramePosition(segmentIdx));

}


TEST(FrameSkipTest, CanMapSegmentToOriginalIndices) {
    assertSegmentToOriginal({0, 10, 1}, 0, 0);
    assertSegmentToOriginal({0, 10, 1}, 1, 1);

    assertSegmentToOriginal({4, 10, 1}, 0, 4);
    assertSegmentToOriginal({4, 10, 1}, 2, 6);

    assertSegmentToOriginal({0, 20, 3}, 0, 0);
    assertSegmentToOriginal({0, 20, 3}, 2, 6);

    assertSegmentToOriginal({2, 20, 3}, 0, 2);
    assertSegmentToOriginal({2, 20, 3}, 4, 14);
}


void assertOriginalToSegment(FrameSkipper skipper, int originalIdx, int expectedSegmentIdx) {
    ASSERT_EQ(expectedSegmentIdx, skipper.OriginalToSegmentFramePosition(originalIdx));
}


TEST(FrameSkipTest, CanMapOriginalToSegmentIndices) {
    assertOriginalToSegment({0, 10, 1}, 0, 0);
    assertOriginalToSegment({0, 10, 1}, 1, 1);

    assertOriginalToSegment({4, 10, 1}, 4, 0);
    assertOriginalToSegment({4, 10, 1}, 6, 2);

    assertOriginalToSegment({0, 20, 3}, 0, 0);
    assertOriginalToSegment({0, 20, 3}, 3, 1);

    assertOriginalToSegment({2, 20, 3}, 2, 0);
    assertOriginalToSegment({2, 20, 3}, 14, 4);
}


void assertDuration(FrameSkipper skipper, double originalFrameRate, double expectedDuration) {
    ASSERT_DOUBLE_EQ(expectedDuration, skipper.GetSegmentDuration(originalFrameRate));
}

TEST(FrameSkipTest, CanGetSegmentDuration) {
    assertDuration({0, 9, 100}, 10, 1);
    assertDuration({0, 9, 100}, 30, 1.0 / 3);
    assertDuration({0, 199, 100}, 10, 20);
    assertDuration({0, 149, 100}, 10, 15);

    assertDuration({7, 16, 100}, 10, 1);
    assertDuration({7, 16, 100}, 30, 1.0 / 3);
    assertDuration({7, 206, 100}, 10, 20);
    assertDuration({7, 156, 100}, 10, 15);
}


void assertFrameRate(FrameSkipper skipper, double originalFrameRate, double expectedFrameRate) {
    ASSERT_DOUBLE_EQ(expectedFrameRate, skipper.GetSegmentFrameRate(originalFrameRate));
}


TEST(FrameSkipTest, CanCalculateFrameRate) {
    assertFrameRate({0, 9, 1}, 30, 30);
    assertFrameRate({100, 9000, 1}, 30, 30);

    assertFrameRate({0, 9, 2}, 30, 15);
    assertFrameRate({0, 10, 2}, 30, 6.0 / 11 * 30);
    assertFrameRate({1, 12, 2}, 30, 15);

    assertFrameRate({0, 8, 3}, 30, 10);
    assertFrameRate({0, 9, 3}, 30, 4.0 / 10 * 30);
}


void assertCurrentTime(FrameSkipper skipper, int originalPosition, double originalFrameRate,
                       double expectedTimeInMillis) {
    ASSERT_DOUBLE_EQ(expectedTimeInMillis, skipper.GetCurrentSegmentTimeInMillis(originalPosition, originalFrameRate));
}

TEST(FrameSkipTest, CanGetCurrentTime) {
    assertCurrentTime({0, 9, 1}, 0, 30, 0);
    assertCurrentTime({0, 9, 2}, 0, 30, 0);
    assertCurrentTime({1, 10, 2}, 1, 30, 0);

    assertCurrentTime({0, 9, 1}, 1, 30, 100.0 / 3);
    assertCurrentTime({1, 10, 1}, 2, 30, 100.0 / 3);

    assertCurrentTime({0, 9, 1}, 2, 30, 200.0 / 3);
    assertCurrentTime({0, 9, 2}, 2, 30, 200.0 / 3);
    assertCurrentTime({1, 10, 2}, 3, 30, 200.0 / 3);

    assertCurrentTime({0, 9, 1}, 8, 30, 800.0 / 3);
    assertCurrentTime({0, 9, 2}, 8, 30, 800.0 / 3);
    assertCurrentTime({1, 10, 2}, 9, 30, 800.0 / 3);

    assertCurrentTime({0, 9, 1}, 9, 30, 300);
    assertCurrentTime({1, 10, 1}, 10, 30, 300);


    assertCurrentTime({2, 12, 1}, 6, 30, 400.0 / 3);
    assertCurrentTime({2, 12, 2}, 6, 30, 1100.0 / 9);
}



void assertPositionRatio(FrameSkipper skipper, int originalPosition, double expectedRatio) {
    ASSERT_DOUBLE_EQ(expectedRatio, skipper.GetSegmentFramePositionRatio(originalPosition));
}


TEST(FrameSkipTest, CanCalculateFramePositionRatio) {
    assertPositionRatio({0, 9, 1}, 0, 0);
    assertPositionRatio({0, 9, 1}, 2, 0.2);
    assertPositionRatio({0, 9, 1}, 10, 1);

    assertPositionRatio({10, 29, 1}, 10, 0);
    assertPositionRatio({10, 29, 1}, 14, 0.2);
    assertPositionRatio({10, 29, 1}, 30, 1);

    assertPositionRatio({10, 29, 2}, 10, 0);
    assertPositionRatio({10, 29, 2}, 14, 0.2);
    assertPositionRatio({10, 29, 2}, 30, 1);

    assertPositionRatio({1, 11, 2}, 1, 0);
    assertPositionRatio({1, 11, 2}, 3, 1.0 / 6);
    assertPositionRatio({1, 11, 2}, 5, 1.0 / 3);
}


void assertPositionForRatio(FrameSkipper skipper, double ratio, int expectedFramePosition) {
    ASSERT_EQ(expectedFramePosition, skipper.RatioToOriginalFramePosition(ratio));

}

TEST(FrameSkipTest, CanCalculateFramePositionForRatio) {
    assertPositionForRatio({0, 4, 1}, 0.5, 2);
    assertPositionForRatio({0, 5, 1}, 0.5, 3);

    assertPositionForRatio({0, 4, 1}, 1.0 / 3, 1);
    assertPositionForRatio({0, 5, 1}, 1.0 / 3, 2);

    assertPositionForRatio({3, 14, 2}, 0.5, 9);
    assertPositionForRatio({3, 15, 2}, 0.5, 9);
}


void assertPositionForMillis(FrameSkipper skipper, double originalFrameRate, double millis, int expectedSegmentPos) {
    int actualSegmentPos = skipper.MillisToSegmentFramePosition(originalFrameRate, millis);
    ASSERT_EQ(expectedSegmentPos, actualSegmentPos);
}


TEST(FrameSkipTest, CanCalculateFramePositionForMillis) {

    assertPositionForMillis({0, 21, 1}, 10, 600, 6);
    assertPositionForMillis({0, 21, 2}, 10, 600, 3);
    assertPositionForMillis({0, 21, 3}, 10, 600, 2);

    assertPositionForMillis({5, 26, 1}, 10, 600, 6);
    assertPositionForMillis({5, 26, 2}, 10, 600, 3);
    assertPositionForMillis({5, 26, 3}, 10, 600, 2);

    assertPositionForMillis({5, 260, 1}, 10, 600, 6);
    assertPositionForMillis({5, 260, 2}, 10, 600, 3);
    assertPositionForMillis({5, 260, 3}, 10, 600, 2);
}


void assertAvailableInitializationFrames(int startFrame, int numberOfRequestedFrames, int expectedFirstInitFrame,
                                         int expectedNumAvailable) {
    FrameSkipper skipper(startFrame, startFrame * 10, 5);
    int actualFirstInitFrame;
    int actualNumAvailable;
    skipper.GetAvailableInitializationFrames(numberOfRequestedFrames, actualFirstInitFrame, actualNumAvailable);
    ASSERT_EQ(expectedFirstInitFrame, actualFirstInitFrame);
    ASSERT_EQ(expectedNumAvailable, actualNumAvailable);

}

TEST(FrameSkipTest, CanDetermineAvailableInitializationFrames) {
    assertAvailableInitializationFrames(0, 100, 0, 0);
    assertAvailableInitializationFrames(1, 100, 0, 1);
    assertAvailableInitializationFrames(2, 100, 0, 2);

    assertAvailableInitializationFrames(10, 1, 9, 1);
    assertAvailableInitializationFrames(10, 3, 7, 3);
    assertAvailableInitializationFrames(10, 10, 0, 10);

}


int GetFrameNumber(const cv::Mat &frame)  {
    cv::Scalar_<uchar> color = frame.at<cv::Scalar_<uchar>>(0, 0);
    return (int) color.val[0];
}



MPFVideoCapture CreateVideoCapture(int startFrame, int stopFrame) {
    MPFVideoJob job("Test", "test/test_vids/frame_skip_test.avi", startFrame, stopFrame, {}, {} );
    return MPFVideoCapture(job, true, true);
}


MPFVideoJob CreateVideoJob(int startFrame, int stopFrame, int frameInterval) {
    return {"Test", "test/test_vids/frame_skip_test.avi", startFrame, stopFrame,
            {{"FRAME_INTERVAL", std::to_string(frameInterval)}}, {}};

}

MPFVideoCapture CreateVideoCapture(int startFrame, int stopFrame, int frameInterval) {
    return MPFVideoCapture(CreateVideoJob(startFrame, stopFrame, frameInterval), true, true);
}


void assertReadFails(MPFVideoCapture &cap) {
    cv::Mat invalidFrame;
    bool wasRead = cap.Read(invalidFrame);
    ASSERT_FALSE(wasRead);
    ASSERT_TRUE(invalidFrame.empty());
}




TEST(FrameSkipTest, NoFramesSkippedWhenSkipParametersProvidedButFrameSkippingDisabled) {
    MPFVideoCapture cap(CreateVideoJob(0, 1, 3), true, false);

    for (int i = 0; i < 30; i++) {
        cv::Mat frame;
        bool wasRead = cap.Read(frame);
        ASSERT_TRUE(wasRead);

        ASSERT_EQ(i, GetFrameNumber(frame));
    }
    assertReadFails(cap);
}



TEST(FrameSkipTest, NoFramesSkippedWhenDefaultValues) {
    auto cap = CreateVideoCapture(0, 29);

    for (int i = 0; i < 30; i++) {
        cv::Mat frame;
        bool wasRead = cap.Read(frame);
        ASSERT_TRUE(wasRead);

        ASSERT_EQ(i, GetFrameNumber(frame));
    }
    assertReadFails(cap);
}



void assertExpectedFramesShown(int startFrame, int stopFrame, int frameInterval,
                               const std::vector<int> &expectedFrames) {

    auto cap = CreateVideoCapture(startFrame, stopFrame, frameInterval);

    ASSERT_EQ(expectedFrames.size(), cap.GetFrameCount());

    for (int expectedFrame : expectedFrames) {
        cv::Mat frame;
        bool wasRead = cap.Read(frame);
        ASSERT_TRUE(wasRead);
        ASSERT_EQ(expectedFrame, GetFrameNumber(frame));
    }
    assertReadFails(cap);
}



TEST(FrameSkipTest, CanHandleStartStopFrame) {
    assertExpectedFramesShown(10, 16, 1, {10, 11, 12, 13, 14, 15, 16});
    assertExpectedFramesShown(26, 29, 1, {26, 27, 28, 29});
}


TEST(FrameSkipTest, CanSkipFrames) {
    assertExpectedFramesShown(0, 19, 2, {0, 2, 4, 6, 8, 10, 12, 14, 16, 18});

    assertExpectedFramesShown(0, 19, 3, {0, 3, 6, 9, 12, 15, 18});

    assertExpectedFramesShown(0, 19, 4, {0, 4, 8, 12, 16});

    assertExpectedFramesShown(0, 19, 5, {0, 5, 10, 15});
}




TEST(FrameSkipTest, CanHandleStartStopFrameWithInterval) {
    assertExpectedFramesShown(15, 29, 2, {15, 17, 19, 21, 23, 25, 27, 29});

    assertExpectedFramesShown(15, 29, 3, {15, 18, 21, 24, 27});

    assertExpectedFramesShown(15, 29, 4, {15, 19, 23, 27});

    assertExpectedFramesShown(15, 29, 5, {15, 20, 25});

    assertExpectedFramesShown(20, 29, 4, {20, 24, 28});

    assertExpectedFramesShown(21, 29, 4, {21, 25, 29});

    assertExpectedFramesShown(20, 28, 4, {20, 24, 28});

    assertExpectedFramesShown(21, 28, 4, {21, 25});
}


TEST(FrameSkipTest, CanNotSetPositionBeyondSegment) {
    auto cap = CreateVideoCapture(10, 15);

    ASSERT_TRUE(cap.SetFramePosition(4));
    ASSERT_EQ(4, cap.GetCurrentFramePosition());

    ASSERT_FALSE(cap.SetFramePosition(10));
    ASSERT_EQ(4, cap.GetCurrentFramePosition());

    ASSERT_FALSE(cap.SetFramePosition(-1));
    ASSERT_EQ(4, cap.GetCurrentFramePosition());

    ASSERT_TRUE(cap.SetFramePosition(6));
    ASSERT_EQ(6, cap.GetCurrentFramePosition());
    assertReadFails(cap);
}



TEST(FrameSkipTest, CanFixFramePosInReverseTransform) {
    auto cap = CreateVideoCapture(5, 19, 2);

    MPFVideoTrack track(1, 6);
    track.frame_locations = {
            {1, {}},
            {2, {}},
            {6, {}}
    };

    cap.ReverseTransform(track);

    ASSERT_EQ(7, track.start_frame);
    ASSERT_EQ(17, track.stop_frame);
    ASSERT_NE(track.frame_locations.end(), track.frame_locations.find(7));
    ASSERT_NE(track.frame_locations.end(), track.frame_locations.find(9));
    ASSERT_NE(track.frame_locations.end(), track.frame_locations.find(17));
}



void assertInitializationFrameIds(int startFrame, int numberRequested,
                                  const std::vector<int> &expectedInitFrames) {
    auto cap = CreateVideoCapture(startFrame, 29, 4);

    const std::vector<cv::Mat> &initFrames = cap.GetInitializationFramesIfAvailable(numberRequested);

    ASSERT_EQ(expectedInitFrames.size(), initFrames.size());

    auto initFramesIter = initFrames.begin();
    for (int expectedFrameNum : expectedInitFrames) {
        ASSERT_EQ(expectedFrameNum, GetFrameNumber(*initFramesIter));
        initFramesIter++;
    }
}


TEST(FrameSkipTest, CanGetInitializationFrames) {
    assertInitializationFrameIds(0, 100, {});
    assertInitializationFrameIds(1, 100, {0});
    assertInitializationFrameIds(2, 100, {0, 1});
    assertInitializationFrameIds(3, 100, {0, 1, 2});

    assertInitializationFrameIds(10, 1, {9});
    assertInitializationFrameIds(10, 2, {8, 9});
    assertInitializationFrameIds(10, 5, {5, 6, 7, 8, 9});
}


//TEST(FrameSkipTest, CreateTestVideo) {
//
//    cv::VideoCapture cap("/home/mpf/sample-data/sample.mp4");
//    int width = (int) cap.get(cv::CAP_PROP_FRAME_WIDTH);
//    int height = (int) cap.get(cv::CAP_PROP_FRAME_HEIGHT);
//
//    cv::VideoWriter writer("/tmp/out.avi", cv::VideoWriter::fourcc('M','J','P','G'), cap.get(cv::CAP_PROP_FPS), cv::Size(width, height), true);
//
//    cv::Mat frame;
//    cap.read(frame);
//
//    for (int i = 0; i < 30; i++) {
//        frame = cv::Scalar(i, i, i);
//        std::string text = "Frame #" + std::to_string(i);
//        cv::putText(frame, text, cv::Point(50, 50), cv::HersheyFonts::FONT_HERSHEY_COMPLEX, .5, cv::Scalar(255, 255, 255));
//        writer << frame;
//    }
//}


//TEST(FrameSkipTest, VerifyTestVideo) {
//
//    cv::VideoCapture cap("/tmp/out.avi");
//
//    cv::Mat frame;
//    while (cap.read(frame)) {
//        cv::Scalar_<uchar> color = frame.at<cv::Scalar_<uchar>>(0, 0);
//        int frameNumber = (int) color.val[0];
//        log("frameNumber", frameNumber);
//    }
//}
