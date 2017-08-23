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

#include <gtest/gtest.h>
#include <vector>

#include <opencv2/video.hpp>

#include "MPFVideoCapture.h"
#include "FrameSkipper.h"

using namespace MPF::COMPONENT;
using namespace std;


const char frameSkipTestVideo[] = "test/test_vids/frame_skip_test.avi";

const char videoWithFramePositionIssues[] = "test/test_vids/vid-with-set-position-issues.mov";


void assertSegmentFrameCount(FrameSkipper frameSkipper, int expected) {
    ASSERT_EQ(expected, frameSkipper.GetSegmentFrameCount());
}

TEST(FrameSkipTest, CanCalculateSegmentFrameCount) {

    assertSegmentFrameCount({ 0, 20, 4 }, 6);
    assertSegmentFrameCount({ 0, 20, 3 }, 7);
    assertSegmentFrameCount({ 4, 10, 1 }, 7);
    assertSegmentFrameCount({ 4, 10, 2 }, 4);
    assertSegmentFrameCount({ 4, 10, 3 }, 3);
    assertSegmentFrameCount({ 4, 10, 20 }, 1);
    assertSegmentFrameCount({ 0, 9, 1 }, 10);
    assertSegmentFrameCount({ 0, 9, 2 }, 5);
    assertSegmentFrameCount({ 0, 9, 3 }, 4);
    assertSegmentFrameCount({ 3, 22, 7 }, 3);
    assertSegmentFrameCount({ 3, 22, 4 }, 5);
}



void assertSegmentToOriginalFramePosition(FrameSkipper skipper, int segmentPosition, int expectedOriginalPosition) {
    ASSERT_EQ(expectedOriginalPosition, skipper.SegmentToOriginalFramePosition(segmentPosition));

}


TEST(FrameSkipTest, CanMapSegmentToOriginalFramePosition) {
    assertSegmentToOriginalFramePosition({ 0, 10, 1 }, 0, 0);
    assertSegmentToOriginalFramePosition({ 0, 10, 1 }, 1, 1);

    assertSegmentToOriginalFramePosition({ 4, 10, 1 }, 0, 4);
    assertSegmentToOriginalFramePosition({ 4, 10, 1 }, 2, 6);

    assertSegmentToOriginalFramePosition({ 0, 20, 3 }, 0, 0);
    assertSegmentToOriginalFramePosition({ 0, 20, 3 }, 2, 6);

    assertSegmentToOriginalFramePosition({ 2, 20, 3 }, 0, 2);
    assertSegmentToOriginalFramePosition({ 2, 20, 3 }, 4, 14);
}


void assertOriginalToSegmentFramePosition(FrameSkipper skipper, int originalPosition, int expectedSegmentPosition) {
    ASSERT_EQ(expectedSegmentPosition, skipper.OriginalToSegmentFramePosition(originalPosition));
}


TEST(FrameSkipTest, CanMapOriginalToSegmentIndices) {
    assertOriginalToSegmentFramePosition({ 0, 10, 1 }, 0, 0);
    assertOriginalToSegmentFramePosition({ 0, 10, 1 }, 1, 1);

    assertOriginalToSegmentFramePosition({ 4, 10, 1 }, 4, 0);
    assertOriginalToSegmentFramePosition({ 4, 10, 1 }, 6, 2);

    assertOriginalToSegmentFramePosition({ 0, 20, 3 }, 0, 0);
    assertOriginalToSegmentFramePosition({ 0, 20, 3 }, 3, 1);

    assertOriginalToSegmentFramePosition({ 2, 20, 3 }, 2, 0);
    assertOriginalToSegmentFramePosition({ 2, 20, 3 }, 14, 4);
}


void assertSegmentDuration(FrameSkipper skipper, double originalFrameRate, double expectedDuration) {
    ASSERT_DOUBLE_EQ(expectedDuration, skipper.GetSegmentDuration(originalFrameRate));
}

TEST(FrameSkipTest, CanGetSegmentDuration) {
    assertSegmentDuration({ 0, 9, 100 }, 10, 1);
    assertSegmentDuration({ 0, 9, 100 }, 30, 1.0 / 3);
    assertSegmentDuration({ 0, 199, 100 }, 10, 20);
    assertSegmentDuration({ 0, 149, 100 }, 10, 15);

    assertSegmentDuration({ 7, 16, 100 }, 10, 1);
    assertSegmentDuration({ 7, 16, 100 }, 30, 1.0 / 3);
    assertSegmentDuration({ 7, 206, 100 }, 10, 20);
    assertSegmentDuration({ 7, 156, 100 }, 10, 15);
}


void assertSegmentFrameRate(FrameSkipper skipper, double originalFrameRate, double expectedFrameRate) {
    ASSERT_DOUBLE_EQ(expectedFrameRate, skipper.GetSegmentFrameRate(originalFrameRate));
}


TEST(FrameSkipTest, CanCalculateSegmentFrameRate) {
    assertSegmentFrameRate({ 0, 9, 1 }, 30, 30);
    assertSegmentFrameRate({ 100, 9000, 1 }, 30, 30);

    assertSegmentFrameRate({ 0, 9, 2 }, 30, 15);
    assertSegmentFrameRate({ 0, 10, 2 }, 30, 6.0 / 11 * 30);
    assertSegmentFrameRate({ 1, 12, 2 }, 30, 15);

    assertSegmentFrameRate({ 0, 8, 3 }, 30, 10);
    assertSegmentFrameRate({ 0, 9, 3 }, 30, 4.0 / 10 * 30);
}


void assertCurrentSegmentTime(FrameSkipper skipper, int originalPosition, double originalFrameRate,
                              double expectedTimeInMillis) {
    ASSERT_DOUBLE_EQ(expectedTimeInMillis, skipper.GetCurrentSegmentTimeInMillis(originalPosition, originalFrameRate));
}

TEST(FrameSkipTest, CanGetCurrentSegmentTimeInMillis) {
    assertCurrentSegmentTime({ 0, 9, 1 }, 0, 30, 0);
    assertCurrentSegmentTime({ 0, 9, 2 }, 0, 30, 0);
    assertCurrentSegmentTime({ 1, 10, 2 }, 1, 30, 0);

    assertCurrentSegmentTime({ 0, 9, 1 }, 1, 30, 100.0 / 3);
    assertCurrentSegmentTime({ 1, 10, 1 }, 2, 30, 100.0 / 3);

    assertCurrentSegmentTime({ 0, 9, 1 }, 2, 30, 200.0 / 3);
    assertCurrentSegmentTime({ 0, 9, 2 }, 2, 30, 200.0 / 3);
    assertCurrentSegmentTime({ 1, 10, 2 }, 3, 30, 200.0 / 3);

    assertCurrentSegmentTime({ 0, 9, 1 }, 8, 30, 800.0 / 3);
    assertCurrentSegmentTime({ 0, 9, 2 }, 8, 30, 800.0 / 3);
    assertCurrentSegmentTime({ 1, 10, 2 }, 9, 30, 800.0 / 3);

    assertCurrentSegmentTime({ 0, 9, 1 }, 9, 30, 300);
    assertCurrentSegmentTime({ 1, 10, 1 }, 10, 30, 300);


    assertCurrentSegmentTime({ 2, 12, 1 }, 6, 30, 400.0 / 3);
    assertCurrentSegmentTime({ 2, 12, 2 }, 6, 30, 1100.0 / 9);
}



void assertSegmentFramePositionRatio(FrameSkipper skipper, int originalPosition, double expectedRatio) {
    ASSERT_DOUBLE_EQ(expectedRatio, skipper.GetSegmentFramePositionRatio(originalPosition));
}


TEST(FrameSkipTest, CanCalculateSegmentFramePositionRatio) {
    assertSegmentFramePositionRatio({ 0, 9, 1 }, 0, 0);
    assertSegmentFramePositionRatio({ 0, 9, 1 }, 2, 0.2);
    assertSegmentFramePositionRatio({ 0, 9, 1 }, 10, 1);

    assertSegmentFramePositionRatio({ 10, 29, 1 }, 10, 0);
    assertSegmentFramePositionRatio({ 10, 29, 1 }, 14, 0.2);
    assertSegmentFramePositionRatio({ 10, 29, 1 }, 30, 1);

    assertSegmentFramePositionRatio({ 10, 29, 2 }, 10, 0);
    assertSegmentFramePositionRatio({ 10, 29, 2 }, 14, 0.2);
    assertSegmentFramePositionRatio({ 10, 29, 2 }, 30, 1);

    assertSegmentFramePositionRatio({ 1, 11, 2 }, 1, 0);
    assertSegmentFramePositionRatio({ 1, 11, 2 }, 3, 1.0 / 6);
    assertSegmentFramePositionRatio({ 1, 11, 2 }, 5, 1.0 / 3);
}


void assertRatioToOriginalFramePosition(FrameSkipper skipper, double ratio, int expectedFramePosition) {
    ASSERT_EQ(expectedFramePosition, skipper.RatioToOriginalFramePosition(ratio));

}

TEST(FrameSkipTest, CanCalculateRatioToOriginalFramePosition) {
    assertRatioToOriginalFramePosition({ 0, 4, 1 }, 0.5, 2);
    assertRatioToOriginalFramePosition({ 0, 5, 1 }, 0.5, 3);

    assertRatioToOriginalFramePosition({ 0, 4, 1 }, 1.0 / 3, 1);
    assertRatioToOriginalFramePosition({ 0, 5, 1 }, 1.0 / 3, 2);

    assertRatioToOriginalFramePosition({ 3, 14, 2 }, 0.5, 9);
    assertRatioToOriginalFramePosition({ 3, 15, 2 }, 0.5, 9);
}


void assertMillisToSegmentFramePosition(FrameSkipper skipper, double originalFrameRate, double segmentMillis,
                                        int expectedSegmentPos) {
    int actualSegmentPos = skipper.MillisToSegmentFramePosition(originalFrameRate, segmentMillis);
    ASSERT_EQ(expectedSegmentPos, actualSegmentPos);
}


TEST(FrameSkipTest, CanCalculateMillisToSegmentFramePosition) {

    assertMillisToSegmentFramePosition({ 0, 21, 1 }, 10, 600, 6);
    assertMillisToSegmentFramePosition({ 0, 21, 2 }, 10, 600, 3);
    assertMillisToSegmentFramePosition({ 0, 21, 3 }, 10, 600, 2);

    assertMillisToSegmentFramePosition({ 5, 26, 1 }, 10, 600, 6);
    assertMillisToSegmentFramePosition({ 5, 26, 2 }, 10, 600, 3);
    assertMillisToSegmentFramePosition({ 5, 26, 3 }, 10, 600, 2);

    assertMillisToSegmentFramePosition({ 5, 260, 1 }, 10, 600, 6);
    assertMillisToSegmentFramePosition({ 5, 260, 2 }, 10, 600, 3);
    assertMillisToSegmentFramePosition({ 5, 260, 3 }, 10, 600, 2);
}


void assertAvailableInitializationFrames(int startFrame, int frameInterval, int expectedNumAvailable) {

    FrameSkipper skipper(startFrame, startFrame + 10, frameInterval);
    int actualNumAvailable = skipper.GetAvailableInitializationFrameCount();
    ASSERT_EQ(expectedNumAvailable, actualNumAvailable);
}


TEST(FrameSkipTest, CanDetermineAvailableInitializationFrames) {
    assertAvailableInitializationFrames(0, 1, 0);
    assertAvailableInitializationFrames(1, 1, 1);
    assertAvailableInitializationFrames(2, 1, 2);
    assertAvailableInitializationFrames(10, 1, 10);

    assertAvailableInitializationFrames(0, 2, 0);
    assertAvailableInitializationFrames(10, 2, 5);
    assertAvailableInitializationFrames(11, 2, 5);

    assertAvailableInitializationFrames(2, 3, 0);
    assertAvailableInitializationFrames(5, 3, 1);
    assertAvailableInitializationFrames(10, 3, 3);

    assertAvailableInitializationFrames(10, 12, 0);
}


int GetFrameNumber(const cv::Mat &frame)  {
    cv::Scalar_<uchar> color = frame.at<cv::Scalar_<uchar>>(0, 0);
    return static_cast<int>(color.val[0]);
}



MPFVideoCapture CreateVideoCapture(int startFrame, int stopFrame) {
    MPFVideoJob job("Test", frameSkipTestVideo, startFrame, stopFrame, {}, {} );
    return MPFVideoCapture(job, true, true);
}


MPFVideoJob CreateVideoJob(int startFrame, int stopFrame, int frameInterval) {
    return {"Test", frameSkipTestVideo, startFrame, stopFrame,
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
                               const vector<int> &expectedFrames) {

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

    // cv::VideoCapture does not allow you to set the frame position to the number of frames in the video.
    // However, after you read the last frame in the video, cv::VideoCapture's frame position will equal the number
    // of frames in the video.
    ASSERT_FALSE(cap.SetFramePosition(cap.GetFrameCount()));
    ASSERT_EQ(4, cap.GetCurrentFramePosition());

    ASSERT_TRUE(cap.SetFramePosition(5));
    ASSERT_EQ(5, cap.GetCurrentFramePosition());

    cv::Mat frame;
    ASSERT_TRUE(cap.Read(frame));
    ASSERT_EQ(15, GetFrameNumber(frame));

    ASSERT_EQ(cap.GetFrameCount(), cap.GetCurrentFramePosition());

    // At end of segment so there shouldn't be any frames left to process.
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



void assertInitializationFrameIds(int startFrame, int frameInterval, int numberRequested,
                                  const vector<int> &expectedInitFrames) {
    auto cap = CreateVideoCapture(startFrame, 29, frameInterval);

    const vector<cv::Mat> &initFrames = cap.GetInitializationFramesIfAvailable(numberRequested);

    ASSERT_EQ(expectedInitFrames.size(), initFrames.size());

    auto initFramesIter = initFrames.begin();
    for (int expectedFrameNum : expectedInitFrames) {
        ASSERT_EQ(expectedFrameNum, GetFrameNumber(*initFramesIter));
        initFramesIter++;
    }
}


TEST(FrameSkipTest, CanGetInitializationFrames) {
    assertInitializationFrameIds(0, 1, 100, {});
    assertInitializationFrameIds(1, 1, 100, {0});
    assertInitializationFrameIds(2, 1, 100, {0, 1});
    assertInitializationFrameIds(3, 1, 100, {0, 1, 2});

    assertInitializationFrameIds(10, 1, 1, {9});
    assertInitializationFrameIds(10, 1, 2, {8, 9});
    assertInitializationFrameIds(10, 1, 5, {5, 6, 7, 8, 9});


    assertInitializationFrameIds(0, 4, 100, {});
    assertInitializationFrameIds(3, 4, 100, {});
    assertInitializationFrameIds(4, 4, 100, {0});
    assertInitializationFrameIds(7, 4, 100, {3});
    assertInitializationFrameIds(8, 4, 100, {0, 4});
    assertInitializationFrameIds(9, 4, 100, {1, 5});

    assertInitializationFrameIds(10, 3, 1, {7});
    assertInitializationFrameIds(10, 3, 2, {4, 7});
    assertInitializationFrameIds(10, 3, 3, {1, 4, 7});
    assertInitializationFrameIds(10, 3, 4, {1, 4, 7});
    assertInitializationFrameIds(10, 3, 5, {1, 4, 7});
    assertInitializationFrameIds(10, 3, 100, {1, 4, 7});
}


TEST(FrameSkipTest, VerifyInitializationFramesIndependentOfCurrentPosition) {

    auto cap = CreateVideoCapture(10, 29, 5);
    cap.SetFramePosition(2);

    cv::Mat frame;
    cap.Read(frame);
    ASSERT_EQ(20, GetFrameNumber(frame));
    ASSERT_EQ(3, cap.GetCurrentFramePosition());

    const auto &initFrames = cap.GetInitializationFramesIfAvailable(2);

    ASSERT_EQ(2, initFrames.size());
    ASSERT_EQ(0, GetFrameNumber(initFrames[0]));
    ASSERT_EQ(5, GetFrameNumber(initFrames[1]));

    ASSERT_EQ(3, cap.GetCurrentFramePosition());
    cap.Read(frame);
    ASSERT_EQ(25, GetFrameNumber(frame));
}


TEST(FrameSkipTest, VerifyCvVideoCaptureGetFramePositionIssue) {
    // This test demonstrates the issue that led us to keep track of frame position in MPFVideoCapture instead of
    // depending on cv::VideoCapture.
    // This test may fail in a future version of OpenCV. If this test fails, then MPFVideoCapture no longer needs to
    // handle frame position.
    // The VerifyMpfVideoCaptureDoesNotHaveGetFramePositionIssue shows that MPFVideoCapture does not have the
    // same issue demonstrated here.
    cv::VideoCapture cap(videoWithFramePositionIssues);

    cv::Mat frame;
    cap.read(frame);

    cap.set(cv::CAP_PROP_POS_FRAMES, 10);
    cap.read(frame);

    auto framePosition = static_cast<int>(cap.get(cv::CAP_PROP_POS_FRAMES));

    ASSERT_NE(framePosition, 11) << "If this test fails, then a bug with OpenCV has been fixed. See test for details";
}


TEST(FrameSkipTest, VerifyMpfVideoCaptureDoesNotHaveGetFramePositionIssue) {
    // This test verifies that MPFVideoCapture does not have the same issue demonstrated in the
    // VerifyCvVideoCaptureGetFramePositionIssue test

    MPFVideoJob job("Test", videoWithFramePositionIssues, 0, 1000, {}, {});
    MPFVideoCapture cap(job, false, false);

    cv::Mat frame;
    cap.Read(frame);

    cap.SetFramePosition(10);
    cap.Read(frame);

    auto framePosition = cap.GetCurrentFramePosition();

    ASSERT_EQ(framePosition, 11);
}



TEST(FrameSkipTest, VerifyCvVideoCaptureSetFramePositionIssue) {
    // This test demonstrates the issue that led us to implement SeekStrategy with fall-backs instead of just
    // using cv::VideoCapture.set(cv::CAP_PROP_POS_FRAMES, int).
    // This test may fail in a future version of OpenCV. If this test fails, then MPFVideoCapture no longer needs
    // use the SeekStrategy classes.
    // The VerifyMPfVideoCaptureDoesNotHaveSetFramePositionIssue shows that MPFVideoCapture does not have the
    // same issue demonstrated here.

    cv::VideoCapture cap(videoWithFramePositionIssues);

    auto frameCount = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_COUNT));

    cap.set(cv::CAP_PROP_POS_FRAMES, frameCount - 5);

    cv::Mat frame;
    bool wasRead = cap.read(frame);
    ASSERT_FALSE(wasRead) << "If this test fails, then a bug with OpenCV has been fixed. See test for details";
}


TEST(FrameSkipTest, VerifyMPfVideoCaptureDoesNotHaveSetFramePositionIssue) {
    // This test verifies that MPFVideoCapture does not have the same issue demonstrated in the
    // VerifyCvVideoCaptureSetFramePositionIssue test
    MPFVideoJob job("Test", videoWithFramePositionIssues, 0, 1000, {}, {});
    MPFVideoCapture cap(job, false, false);

    auto frameCount = cap.GetFrameCount();

    cap.SetFramePosition(frameCount - 5);

    cv::Mat frame;
    bool wasRead = cap.Read(frame);
    ASSERT_TRUE(wasRead);
}


void assertCanChangeFramePosition(const SeekStrategy& seekStrategy) {
    cv::VideoCapture cap(frameSkipTestVideo);
    int framePosition = 0;

    framePosition = seekStrategy.ChangePosition(cap, framePosition, 28);
    ASSERT_EQ(framePosition, 28) << "Failed to seek forward";
    cv::Mat frame;
    ASSERT_TRUE(cap.read(frame)) << "Failed to read frame after forward seek";
    ASSERT_EQ(GetFrameNumber(frame), 28) << "Incorrect frame read after forward seek";
    framePosition++;


    framePosition = seekStrategy.ChangePosition(cap, framePosition, 5);
    ASSERT_EQ(framePosition, 5) << "Failed to seek backward";
    ASSERT_TRUE(cap.read(frame)) << "Failed to read frame after backward seek";
    ASSERT_EQ(GetFrameNumber(frame), 5) << "Incorrect frame read after backward seek";
    framePosition++;


    framePosition = seekStrategy.ChangePosition(cap, framePosition, 20);
    ASSERT_EQ(framePosition, 20) << "Failed to seek forward after backward seek";
    ASSERT_TRUE(cap.read(frame)) << "Failed to read frame when seeking forward after backward seek";
    ASSERT_EQ(GetFrameNumber(frame), 20) << "Incorrect frame read when seeking forward after backward seek";
    framePosition++;
}



TEST(FrameSkipTest, TestSetFramePositionSeek) {
    assertCanChangeFramePosition(SetFramePositionSeek());
}


TEST(FrameSkipTest, TestGrabSeek) {
    assertCanChangeFramePosition(GrabSeek());
}


TEST(FrameSkipTest, TestReadSeek) {
    assertCanChangeFramePosition(ReadSeek());
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
