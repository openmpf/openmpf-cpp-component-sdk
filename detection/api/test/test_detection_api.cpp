/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2019 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2019 The MITRE Corporation                                       *
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
#include <opencv2/opencv.hpp>
#include <frame_transformers/NoOpFrameTransformer.h>
#include <detectionComponentUtils.h>
#include <frame_transformers/FrameTransformerFactory.h>
#include <MPFImageReader.h>
#include <frame_transformers/AffineFrameTransformer.h>
#include <frame_transformers/FrameRotator.h>

#include "KeyFrameFilter.h"
#include "FeedForwardFrameFilter.h"
#include "MPFVideoCapture.h"
#include "IntervalFrameFilter.h"
#include "FrameFilter.h"

using namespace MPF::COMPONENT;
using namespace std;


const char frameFilterTestVideo[] = "test/test_vids/frame_filter_test.mp4";

const char videoWithFramePositionIssues[] = "test/test_vids/vid-with-set-position-issues.mov";




FeedForwardFrameFilter toFeedForwardFilter(const IntervalFrameFilter &filter) {
    int frameCount = filter.GetSegmentFrameCount();

    MPFVideoTrack feedForwardTrack;
    for (int i = 0; i < frameCount; i++) {
        feedForwardTrack.frame_locations[filter.SegmentToOriginalFramePosition(i)] = { };
    }

    MPFVideoJob job("Test", "Test", 0, 0, feedForwardTrack, {}, {});
    return FeedForwardFrameFilter(job.feed_forward_track);
}



void assertSegmentFrameCount(const IntervalFrameFilter &frameFilter, int expected) {
    ASSERT_EQ(frameFilter.GetSegmentFrameCount(), expected);
    ASSERT_EQ(toFeedForwardFilter(frameFilter).GetSegmentFrameCount(), expected);
}

TEST(FrameFilterTest, CanCalculateSegmentFrameCount) {
    assertSegmentFrameCount({ 0, 0, 1}, 1);
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



void assertSegmentToOriginalFramePosition(const IntervalFrameFilter &filter, int segmentPosition,
                                          int expectedOriginalPosition) {

    ASSERT_EQ(filter.SegmentToOriginalFramePosition(segmentPosition), expectedOriginalPosition);
    ASSERT_EQ(toFeedForwardFilter(filter).SegmentToOriginalFramePosition(segmentPosition), expectedOriginalPosition);
}


TEST(FrameFilterTest, CanMapSegmentToOriginalFramePosition) {
    assertSegmentToOriginalFramePosition({ 0, 10, 1 }, 0, 0);
    assertSegmentToOriginalFramePosition({ 0, 10, 1 }, 1, 1);

    assertSegmentToOriginalFramePosition({ 4, 10, 1 }, 0, 4);
    assertSegmentToOriginalFramePosition({ 4, 10, 1 }, 2, 6);

    assertSegmentToOriginalFramePosition({ 0, 20, 3 }, 0, 0);
    assertSegmentToOriginalFramePosition({ 0, 20, 3 }, 2, 6);

    assertSegmentToOriginalFramePosition({ 2, 20, 3 }, 0, 2);
    assertSegmentToOriginalFramePosition({ 2, 20, 3 }, 4, 14);
}


void assertOriginalToSegmentFramePosition(const IntervalFrameFilter &filter, int originalPosition,
                                          int expectedSegmentPosition) {

    ASSERT_EQ(filter.OriginalToSegmentFramePosition(originalPosition), expectedSegmentPosition);
    ASSERT_EQ(toFeedForwardFilter(filter).OriginalToSegmentFramePosition(originalPosition), expectedSegmentPosition);
}


TEST(FrameFilterTest, CanMapOriginalToSegmentIndices) {
    assertOriginalToSegmentFramePosition({ 0, 10, 1 }, 0, 0);
    assertOriginalToSegmentFramePosition({ 0, 10, 1 }, 1, 1);

    assertOriginalToSegmentFramePosition({ 4, 10, 1 }, 4, 0);
    assertOriginalToSegmentFramePosition({ 4, 10, 1 }, 6, 2);

    assertOriginalToSegmentFramePosition({ 0, 20, 3 }, 0, 0);
    assertOriginalToSegmentFramePosition({ 0, 20, 3 }, 3, 1);

    assertOriginalToSegmentFramePosition({ 2, 20, 3 }, 2, 0);
    assertOriginalToSegmentFramePosition({ 2, 20, 3 }, 14, 4);
}


void assertSegmentDuration(const IntervalFrameFilter &filter, double originalFrameRate,
                           double expectedIntervalDuration, double expectedFeedForwardDuration) {

    ASSERT_DOUBLE_EQ(filter.GetSegmentDuration(originalFrameRate), expectedIntervalDuration);
    ASSERT_DOUBLE_EQ(toFeedForwardFilter(filter).GetSegmentDuration(originalFrameRate), expectedFeedForwardDuration);
}

TEST(FrameFilterTest, CanGetSegmentDuration) {
    assertSegmentDuration({ 0, 9, 100 }, 10, 1, 0.1);
    assertSegmentDuration({ 0, 9, 100 }, 30, 1.0 / 3, 1.0 / 30);
    assertSegmentDuration({ 0, 199, 100 }, 10, 20, 10.1);
    assertSegmentDuration({ 0, 149, 100 }, 10, 15, 10.1);

    assertSegmentDuration({ 7, 16, 100 }, 10, 1, 0.1);
    assertSegmentDuration({ 7, 16, 100 }, 30, 1.0 / 3, 1.0 / 30);
    assertSegmentDuration({ 7, 206, 100 }, 10, 20, 10.1);
    assertSegmentDuration({ 7, 156, 100 }, 10, 15, 10.1);
}


void assertSegmentFrameRate(const IntervalFrameFilter &filter, double expectedIntervalFrameRate,
                            double expectedFeedForwardFrameRate) {

    ASSERT_DOUBLE_EQ(filter.GetSegmentFrameRate(30), expectedIntervalFrameRate);
    ASSERT_DOUBLE_EQ(toFeedForwardFilter(filter).GetSegmentFrameRate(30), expectedFeedForwardFrameRate);
}

TEST(FrameFilterTest, CanCalculateSegmentFrameRate) {
    assertSegmentFrameRate({ 0, 9, 1 }, 30, 30);
    assertSegmentFrameRate({ 100, 9000, 1 }, 30, 30);

    assertSegmentFrameRate({ 0, 9, 2 }, 15, 50.0 / 3);
    assertSegmentFrameRate({ 0, 10, 2 }, 180.0 / 11, 180.0 / 11);
    assertSegmentFrameRate({ 1, 12, 2 }, 15, 180.0 / 11);

    assertSegmentFrameRate({ 0, 8, 3 }, 10, 90.0 / 7);
    assertSegmentFrameRate({ 0, 9, 3 }, 12, 12);
}


void assertCurrentSegmentTime(const IntervalFrameFilter &filter, int originalPosition,
                              double expectedIntervalTimeInMillis, double expectedFeedForwardTimeInMillis) {

    ASSERT_DOUBLE_EQ(filter.GetCurrentSegmentTimeInMillis(originalPosition, 30), expectedIntervalTimeInMillis);

    ASSERT_DOUBLE_EQ(toFeedForwardFilter(filter).GetCurrentSegmentTimeInMillis(originalPosition, 30),
                     expectedFeedForwardTimeInMillis);
}


TEST(FrameFilterTest, CanGetCurrentSegmentTimeInMillis) {
    assertCurrentSegmentTime({ 0, 9, 1 }, 0, 0, 0);
    assertCurrentSegmentTime({ 0, 9, 2 }, 0, 0, 0);
    assertCurrentSegmentTime({ 1, 10, 2 }, 1, 0, 0);

    assertCurrentSegmentTime({ 0, 9, 1 }, 1, 100.0 / 3, 100.0 / 3);
    assertCurrentSegmentTime({ 1, 10, 1 }, 2, 100.0 / 3, 100.0 / 3);

    assertCurrentSegmentTime({ 0, 9, 1 }, 2, 200.0 / 3, 200.0 / 3);
    assertCurrentSegmentTime({ 0, 9, 2 }, 2, 200.0 / 3, 60);
    assertCurrentSegmentTime({ 1, 10, 2 }, 3, 200.0 / 3, 60);

    assertCurrentSegmentTime({ 0, 9, 1 }, 8, 800.0 / 3, 800.0 / 3);
    assertCurrentSegmentTime({ 0, 9, 2 }, 8, 800.0 / 3, 240);
    assertCurrentSegmentTime({ 1, 10, 2 }, 9, 800.0 / 3, 240);

    assertCurrentSegmentTime({ 0, 9, 1 }, 9, 300, 300);
    assertCurrentSegmentTime({ 1, 10, 1 }, 10, 300, 300);


    assertCurrentSegmentTime({ 2, 12, 1 }, 6, 400.0 / 3, 400.0 / 3);
    assertCurrentSegmentTime({ 2, 12, 2 }, 6, 1100.0 / 9, 1100.0 / 9);
}



void assertSegmentFramePositionRatio(const IntervalFrameFilter &filter, int originalPosition,
                                     double expectedRatio) {

    ASSERT_DOUBLE_EQ(filter.GetSegmentFramePositionRatio(originalPosition), expectedRatio);

    const auto &feedForwardFilter = toFeedForwardFilter(filter);
    ASSERT_DOUBLE_EQ(feedForwardFilter.GetSegmentFramePositionRatio(originalPosition),
                     expectedRatio);

    if (expectedRatio >= 1) {
        ASSERT_TRUE(filter.IsPastEndOfSegment(originalPosition));
        ASSERT_TRUE(feedForwardFilter.IsPastEndOfSegment(originalPosition));
    }
    else {
        ASSERT_FALSE(filter.IsPastEndOfSegment(originalPosition));
        ASSERT_FALSE(feedForwardFilter.IsPastEndOfSegment(originalPosition));

    }
}


TEST(FrameFilterTest, CanCalculateSegmentFramePositionRatio) {
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



void assertRatioToOriginalFramePosition(const IntervalFrameFilter &filter, double ratio, int expectedFramePosition) {
    ASSERT_EQ(filter.RatioToOriginalFramePosition(ratio), expectedFramePosition);
    ASSERT_EQ(toFeedForwardFilter(filter).RatioToOriginalFramePosition(ratio), expectedFramePosition);

}

TEST(FrameFilterTest, CanCalculateRatioToOriginalFramePosition) {
    assertRatioToOriginalFramePosition({ 0, 4, 1 }, 0.5, 2);
    assertRatioToOriginalFramePosition({ 0, 5, 1 }, 0.5, 3);

    assertRatioToOriginalFramePosition({ 0, 4, 1 }, 1.0 / 3, 1);
    assertRatioToOriginalFramePosition({ 0, 5, 1 }, 1.0 / 3, 2);

    assertRatioToOriginalFramePosition({ 3, 14, 2 }, 0.5, 9);
    assertRatioToOriginalFramePosition({ 3, 15, 2 }, 0.5, 9);
}


void assertMillisToSegmentFramePosition(const IntervalFrameFilter &filter, double originalFrameRate,
                                        double segmentMillis, int expectedSegmentPos) {

    ASSERT_EQ(filter.MillisToSegmentFramePosition(originalFrameRate, segmentMillis), expectedSegmentPos);
    ASSERT_EQ(toFeedForwardFilter(filter).MillisToSegmentFramePosition(originalFrameRate, segmentMillis),
              expectedSegmentPos);
}


TEST(FrameFilterTest, CanCalculateMillisToSegmentFramePosition) {

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

    IntervalFrameFilter filter(startFrame, startFrame + 10, frameInterval);
    int actualNumAvailable = filter.GetAvailableInitializationFrameCount();
    ASSERT_EQ(expectedNumAvailable, actualNumAvailable);
}


TEST(FrameFilterTest, CanDetermineAvailableInitializationFrames) {
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
    MPFVideoJob job("Test", frameFilterTestVideo, startFrame, stopFrame, {}, {} );
    return MPFVideoCapture(job, true, true);
}


MPFVideoJob CreateVideoJob(int startFrame, int stopFrame, int frameInterval) {
    return {"Test", frameFilterTestVideo, startFrame, stopFrame,
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




TEST(FrameFilterTest, NoFramesSkippedWhenFilterParametersProvidedButFrameFilteringDisabled) {
    MPFVideoCapture cap(CreateVideoJob(0, 1, 3), true, false);

    for (int i = 0; i < 30; i++) {
        cv::Mat frame;
        bool wasRead = cap.Read(frame);
        ASSERT_TRUE(wasRead);

        ASSERT_EQ(i, GetFrameNumber(frame));
    }
    assertReadFails(cap);
}



TEST(FrameFilterTest, NoFramesSkippedWhenDefaultValues) {
    auto cap = CreateVideoCapture(0, 29);

    for (int i = 0; i < 30; i++) {
        cv::Mat frame;
        bool wasRead = cap.Read(frame);
        ASSERT_TRUE(wasRead);

        ASSERT_EQ(i, GetFrameNumber(frame));
    }
    assertReadFails(cap);
}


void assertExpectedFramesShown(MPFVideoCapture &cap, const vector<int> &expectedFrames) {
    ASSERT_EQ(expectedFrames.size(), cap.GetFrameCount());

    for (int expectedFrame : expectedFrames) {
        cv::Mat frame;
        bool wasRead = cap.Read(frame);
        ASSERT_TRUE(wasRead);
        ASSERT_EQ(expectedFrame, GetFrameNumber(frame));
    }
    assertReadFails(cap);
}


void assertExpectedFramesShown(int startFrame, int stopFrame, int frameInterval,
                               const vector<int> &expectedFrames) {

    auto cap = CreateVideoCapture(startFrame, stopFrame, frameInterval);
    assertExpectedFramesShown(cap, expectedFrames);
}



TEST(FrameFilterTest, CanHandleStartStopFrame) {
    assertExpectedFramesShown(10, 16, 1, {10, 11, 12, 13, 14, 15, 16});
    assertExpectedFramesShown(26, 29, 1, {26, 27, 28, 29});
}


TEST(FrameFilterTest, CanFilterFrames) {
    assertExpectedFramesShown(0, 19, 2, {0, 2, 4, 6, 8, 10, 12, 14, 16, 18});

    assertExpectedFramesShown(0, 19, 3, {0, 3, 6, 9, 12, 15, 18});

    assertExpectedFramesShown(0, 19, 4, {0, 4, 8, 12, 16});

    assertExpectedFramesShown(0, 19, 5, {0, 5, 10, 15});
}




TEST(FrameFilterTest, CanHandleStartStopFrameWithInterval) {
    assertExpectedFramesShown(15, 29, 2, {15, 17, 19, 21, 23, 25, 27, 29});

    assertExpectedFramesShown(15, 29, 3, {15, 18, 21, 24, 27});

    assertExpectedFramesShown(15, 29, 4, {15, 19, 23, 27});

    assertExpectedFramesShown(15, 29, 5, {15, 20, 25});

    assertExpectedFramesShown(20, 29, 4, {20, 24, 28});

    assertExpectedFramesShown(21, 29, 4, {21, 25, 29});

    assertExpectedFramesShown(20, 28, 4, {20, 24, 28});

    assertExpectedFramesShown(21, 28, 4, {21, 25});
}


TEST(FrameFilterTest, CanNotSetPositionBeyondSegment) {
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



void assertMapContainsKeys(const map<int, MPFImageLocation> &map, const std::vector<int> &expectedKeys) {

    ASSERT_EQ(map.size(), expectedKeys.size());
    for (const int key : expectedKeys) {
        bool keyInMap = map.find(key) != map.end();
        ASSERT_TRUE(keyInMap) << "Expected to map to have key: " << key;
    }
}


TEST(FrameFilterTest, CanFixFramePosInReverseTransform) {
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
    assertMapContainsKeys(track.frame_locations, {7, 9, 17});
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


TEST(FrameFilterTest, CanGetInitializationFrames) {
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


TEST(FrameFilterTest, VerifyInitializationFramesIndependentOfCurrentPosition) {

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



void assertFrameRead(MPFVideoCapture &cap, int expectedFrameNumber, const cv::Size &expectedSize,
                     double expectedRatio) {
    ASSERT_EQ(cap.GetFramePositionRatio(), expectedRatio);

    cv::Mat frame;
    ASSERT_TRUE(cap.Read(frame));
    ASSERT_EQ(GetFrameNumber(frame), expectedFrameNumber);
    ASSERT_EQ(frame.rows, expectedSize.height);
    ASSERT_EQ(frame.cols, expectedSize.width);
}



TEST(FrameFilterTest, CanHandleFeedForwardTrack) {
    MPFVideoTrack feedForwardTrack(0, 29);
    feedForwardTrack.frame_locations = {
        { 1, {5, 5, 5, 10} },
        { 3, {4, 4, 5, 6} },
        { 7, {5, 5, 8, 9} },
        { 11, {4, 5, 5, 6} },
        { 12, {4, 4, 1, 2} },
        { 20, {5, 5, 5, 5} },
        { 25, {4, 4, 5, 5} }
    };
    MPFVideoJob job("Test", frameFilterTestVideo, 0, -1, feedForwardTrack,
                    {{"FEED_FORWARD_TYPE", "SUPERSET_REGION"}}, {});

    MPFVideoCapture cap(job);
    ASSERT_EQ(cap.GetFrameCount(), 7);
    ASSERT_TRUE(cap.GetInitializationFramesIfAvailable(100).empty());

    int minX = feedForwardTrack.frame_locations[3].x_left_upper;
    int maxX = feedForwardTrack.frame_locations[7].x_left_upper + feedForwardTrack.frame_locations[7].width;

    int minY = feedForwardTrack.frame_locations[3].y_left_upper;
    int maxY = feedForwardTrack.frame_locations[1].y_left_upper + feedForwardTrack.frame_locations[1].height;

    cv::Size expectedSize(maxX - minX, maxY - minY);
    ASSERT_EQ(cap.GetFrameSize(), expectedSize);


    assertFrameRead(cap, 1, expectedSize, 0);
    assertFrameRead(cap, 3, expectedSize, 1.0 / 7);
    assertFrameRead(cap, 7, expectedSize, 2.0 / 7);
    assertFrameRead(cap, 11, expectedSize, 3.0 / 7);
    assertFrameRead(cap, 12, expectedSize, 4.0 / 7);
    assertFrameRead(cap, 20, expectedSize, 5.0 / 7);
    assertFrameRead(cap, 25, expectedSize, 6.0 / 7);

    ASSERT_DOUBLE_EQ(cap.GetFramePositionRatio(), 1);

    assertReadFails(cap);


    MPFVideoTrack track(0, 6);
    track.frame_locations = {
        { 1, {} },
        { 2, {} },
        { 4, {} },
        { 5, {} },
    };

    cap.ReverseTransform(track);

    ASSERT_EQ(track.start_frame, 1);
    ASSERT_EQ(track.stop_frame, 25);
    assertMapContainsKeys(track.frame_locations, {3, 7, 12, 20});
}



TEST(FrameFilterTest, CanUseSearchRegionWithFeedForwardFrameType) {
    MPFVideoTrack feedForwardTrack(0, 15);
    feedForwardTrack.frame_locations = {
        { 1, { 5, 5, 5, 5 } }
    };
    Properties jobProperties {
        { "FEED_FORWARD_TYPE", "FRAME" },
        { "SEARCH_REGION_ENABLE_DETECTION", "true" },
        { "SEARCH_REGION_TOP_LEFT_X_DETECTION", "3" },
        { "SEARCH_REGION_TOP_LEFT_Y_DETECTION", "3" },
        { "SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION", "6" },
        { "SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION", "8" },
    };

    MPFVideoJob job("Test", frameFilterTestVideo, 0, -1, feedForwardTrack, jobProperties, {});
    MPFVideoCapture cap(job);

    cv::Size expectedSize(3, 5);
    ASSERT_EQ(cap.GetFrameSize(), expectedSize);

    cv::Mat frame;
    cap.Read(frame);
    ASSERT_EQ(frame.cols, expectedSize.width);
    ASSERT_EQ(frame.rows, expectedSize.height);
}


TEST(FrameFilterTest, VerifyCvVideoCaptureGetFramePositionIssue) {
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


TEST(FrameFilterTest, VerifyMpfVideoCaptureDoesNotHaveGetFramePositionIssue) {
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



TEST(FrameFilterTest, VerifyCvVideoCaptureSetFramePositionIssue) {
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


TEST(FrameFilterTest, VerifyMPfVideoCaptureDoesNotHaveSetFramePositionIssue) {
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
    cv::VideoCapture cap(frameFilterTestVideo);
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
}



TEST(FrameFilterTest, TestSetFramePositionSeek) {
    assertCanChangeFramePosition(SetFramePositionSeek());
}


TEST(FrameFilterTest, TestGrabSeek) {
    assertCanChangeFramePosition(GrabSeek());
}


TEST(FrameFilterTest, TestReadSeek) {
    assertCanChangeFramePosition(ReadSeek());
}


TEST(FrameFilterTest, CanFilterOnKeyFrames) {
    MPFVideoJob job("Test", frameFilterTestVideo, 0, 1000, {{"USE_KEY_FRAMES", "true"}}, {});
    MPFVideoCapture cap(job);
    assertExpectedFramesShown(cap, {0, 5, 10, 15, 20, 25});
}



TEST(FrameFilterTest, CanFilterOnKeyFramesAndStartStopFrame) {
    MPFVideoJob job("Test", frameFilterTestVideo, 6, 21, {{"USE_KEY_FRAMES", "true"}}, {});
    MPFVideoCapture cap(job);
    assertExpectedFramesShown(cap, {10, 15, 20});
}


TEST(FrameFilterTest, CanFilterOnKeyFramesAndInterval) {
    MPFVideoJob job("Test", frameFilterTestVideo, 0, 1000,
                    {{"USE_KEY_FRAMES", "true"}, {"FRAME_INTERVAL", "2"} }, {});
    MPFVideoCapture cap(job);
    assertExpectedFramesShown(cap, {0, 10, 20});

    MPFVideoJob job2("Test", frameFilterTestVideo, 0, 1000,
                    {{"USE_KEY_FRAMES", "true"}, {"FRAME_INTERVAL", "3"} }, {});
    MPFVideoCapture cap2(job2);
    assertExpectedFramesShown(cap2, {0, 15});
}



TEST(FrameFilterTest, CanFilterOnKeyFramesAndStartStopFrameAndInterval) {
    MPFVideoJob job("Test", frameFilterTestVideo, 5, 21,
                    {{"USE_KEY_FRAMES", "true"}, {"FRAME_INTERVAL", "2"}}, {});
    MPFVideoCapture cap(job);
    assertExpectedFramesShown(cap, {5, 15});
}


/**
 * Creates the frame_filter_test.avi video.
 */
//TEST(FrameFilterTest, CreateTestVideo) {
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


//TEST(FrameFilterTest, VerifyTestVideo) {
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

MPFVideoTrack createTestTrack() {
    MPFVideoTrack track(5, 10);
    MPFImageLocation location(20, 30, 15, 5);
    track.frame_locations = {
            { 5, location },
            { 7, location },
            {10, location }
    };
    return track;
}



TEST(ReverseTransformTest, NoFeedForwardNoSearchRegion) {
    MPFVideoJob job("Test", frameFilterTestVideo, 0, 30, {}, {});
    MPFVideoCapture cap(job);

    auto track = createTestTrack();
    cap.ReverseTransform(track);

    ASSERT_EQ(track.start_frame, 5);
    ASSERT_EQ(track.stop_frame, 10);

    assertMapContainsKeys(track.frame_locations, {5, 7, 10});

    const auto &location = track.frame_locations.begin()->second;
    ASSERT_EQ(location.x_left_upper, 20);
    ASSERT_EQ(location.y_left_upper, 30);
    ASSERT_EQ(location.width, 15);
    ASSERT_EQ(location.height, 5);
}



TEST(ReverseTransformTest, NoFeedForwardWithSearchRegion) {
    MPFVideoJob job("Test", frameFilterTestVideo, 0, 30, {
            { "SEARCH_REGION_ENABLE_DETECTION", "true" },
            { "SEARCH_REGION_TOP_LEFT_X_DETECTION", "3"},
            { "SEARCH_REGION_TOP_LEFT_Y_DETECTION", "4"},
            { "SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION", "40" },
            { "SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION", "50" },
    }, {});
    MPFVideoCapture cap(job);

    ASSERT_EQ(cap.GetFrameSize(), cv::Size(37, 46));

    auto track = createTestTrack();
    cap.ReverseTransform(track);

    ASSERT_EQ(track.start_frame, 5);
    ASSERT_EQ(track.stop_frame, 10);

    assertMapContainsKeys(track.frame_locations, {5, 7, 10});

    const auto &location = track.frame_locations.begin()->second;
    ASSERT_EQ(location.x_left_upper, 23);
    ASSERT_EQ(location.y_left_upper, 34);
    ASSERT_EQ(location.width, 15);
    ASSERT_EQ(location.height, 5);
}


void assertDetectionLocationsMatch(const MPFImageLocation &loc1, const MPFImageLocation &loc2) {
    ASSERT_EQ(loc1.x_left_upper, loc2.x_left_upper);
    ASSERT_EQ(loc1.y_left_upper, loc2.y_left_upper);
    ASSERT_EQ(loc1.width, loc2.width);
    ASSERT_EQ(loc1.height, loc2.height);
}


TEST(FeedForwardFrameCropperTest, CanCropToExactRegion) {
    MPFVideoTrack feedForwardTrack(4, 29, 1, {});
    feedForwardTrack.frame_locations = {
            {4, MPFImageLocation(10, 60, 65, 125)},
            {15, MPFImageLocation(60, 20, 100, 200)},
            {29, MPFImageLocation(70, 0, 30, 240)}
    };

    MPFVideoJob job("Test", frameFilterTestVideo, 4, 29, { {"FEED_FORWARD_TYPE", "REGION"} }, {});
    job.has_feed_forward_track = true;
    job.feed_forward_track = feedForwardTrack;

    MPFVideoCapture cap(job);

    MPFVideoTrack outputTrack(0, 2, 1, {});

    cv::Mat frame;
    int framePos = cap.GetCurrentFramePosition();
    ASSERT_TRUE(cap.Read(frame));
    ASSERT_EQ(GetFrameNumber(frame), 4);
    ASSERT_EQ(frame.size(), cv::Size(65, 125));
    outputTrack.frame_locations[framePos] = MPFImageLocation(0, 0, frame.cols, frame.rows);

    framePos = cap.GetCurrentFramePosition();
    ASSERT_TRUE(cap.Read(frame));
    ASSERT_EQ(GetFrameNumber(frame), 15);
    ASSERT_EQ(frame.size(), cv::Size(100, 200));
    outputTrack.frame_locations[framePos] = MPFImageLocation(0, 0, frame.cols, frame.rows);

    framePos = cap.GetCurrentFramePosition();
    ASSERT_TRUE(cap.Read(frame));
    ASSERT_EQ(GetFrameNumber(frame), 29);
    ASSERT_EQ(frame.size(), cv::Size(30, 240));
    outputTrack.frame_locations[framePos] = MPFImageLocation(5, 40, 15, 60);

    assertReadFails(cap);

    cap.ReverseTransform(outputTrack);

    ASSERT_EQ(outputTrack.frame_locations.size(), feedForwardTrack.frame_locations.size());

    assertDetectionLocationsMatch(outputTrack.frame_locations.at(4), feedForwardTrack.frame_locations.at(4));
    assertDetectionLocationsMatch(outputTrack.frame_locations.at(15), feedForwardTrack.frame_locations.at(15));

    const auto &lastDetection = outputTrack.frame_locations.at(29);
    ASSERT_EQ(lastDetection.x_left_upper, 75);
    ASSERT_EQ(lastDetection.y_left_upper, 40);
    ASSERT_EQ(lastDetection.width, 15);
    ASSERT_EQ(lastDetection.height, 60);
}





bool allPixelsEqualTo(const cv::Mat &actualImg, const cv::Vec<uint8_t, 3> &color, int width, int height) {
    cv::Mat expectedImg(cv::Size(width, height), CV_8UC3, color);
    return 0 == cv::countNonZero(actualImg != expectedImg);
}


//bool allPixelsEqualTo(const cv::Mat &actualImg, const cv::Vec<uint8_t, 3> &color, int width, int height) {
//    cv::Mat expectedImg(cv::Size(width, height), CV_8UC3, color);
//    cv::Mat diff = cv::abs(actualImg - expectedImg);
//    int numWrong = cv::countNonZero(diff > 6);
//    return 0 == numWrong;
//}


//bool allPixelsEqualTo(const cv::Mat &actualImg, const cv::Vec<uint8_t, 3> &color, int width, int height) {
//    cv::Mat expectedImg(cv::Size(width - 1, height - 1), CV_8UC3, color);
//
//    cv::Mat cropped = actualImg(cv::Rect(1, 1, width - 1, height - 1));
//    return 0 == cv::countNonZero(cropped != expectedImg);
//}


//bool allPixelsEqualTo(const cv::Mat &actualImg, const cv::Vec<uint8_t, 3> &color, int width, int height) {
//    using Pixel = cv::Vec<uint8_t, 3>;
//
//    for (int row = 0; row < actualImg.rows; row++) {
//        for (int col = 0; col < actualImg.cols; col++) {
//            const auto& actualColor = actualImg.at<Pixel>(row, col);
//            if (actualColor != color) {
//                std::cout << "wrong: " << col << ", " << row << ": " << actualColor << std::endl;
//            }
//
//        }
//    }
//
//    return true;
//}


void verifyCorrectlyRotated(const std::string &fileName,  const MPFImageLocation &feedForwardDetection) {
//    MPFImageJob job("Test", "test/test_imgs/rotation/" + fileName, feedForwardDetection,
//                    { { "FEED_FORWARD_TYPE", "REGION" } }, {});

    MPFImageJob job("Test", "/home/mpf/openmpf-projects/openmpf-cpp-component-sdk/detection/api/test/test_imgs/rotation/" + fileName, feedForwardDetection,
                    { { "FEED_FORWARD_TYPE", "REGION" } }, {});

    MPFImageReader imageReader(job);

    cv::Mat img = imageReader.GetImage();
    ASSERT_EQ(feedForwardDetection.width, img.cols);
    ASSERT_EQ(feedForwardDetection.height, img.rows);
    cv::imshow("test", img);
    cv::waitKey();


    using Pixel = cv::Vec<uint8_t, 3>;

    for (int col = 1; col < img.cols; col++) {
        for (int row = 1; row < img.rows; row++)  {
            ASSERT_EQ(Pixel(255, 0, 0), img.at<Pixel>(row, col)) << "row = " << row << ", col = " << col;
        }
    }



//    ASSERT_TRUE(allPixelsEqualTo(actualImg, {255, 0, 0}, feedForwardDetection.width, feedForwardDetection.height));
}


TEST(AffineFrameTransformerTest, CanHandleRotatedDetectionNearMiddle) {
//    verifyCorrectlyRotated("20deg-bounding-box.png",
//                           MPFImageLocation(120, 200, 100, 40, -1, { { "ROTATION", "20" } }));
    verifyCorrectlyRotated("20deg-bounding-box.png",
                           MPFImageLocation(116, 218, 100, 40, -1, { { "ROTATION", "20" } }));
}


TEST(AffineFrameTransformerTest, CanHandleRotatedDetectionTouchingCorner) {
    verifyCorrectlyRotated("30deg-bounding-box-top-left-corner.png",
                           MPFImageLocation(3, 23, 100, 40, -1, { { "ROTATION", "30.5" } }));

    verifyCorrectlyRotated("60deg-bounding-box-top-left-corner.png",
                           MPFImageLocation(-8, 33, 100, 40, -1, { { "ROTATION", "60" } }));

    verifyCorrectlyRotated("200deg-bounding-box-top-left-corner.png",
                           MPFImageLocation(4, 16, 100, 40, -1, { { "ROTATION", "200" } }));

    verifyCorrectlyRotated("20deg-bounding-box-bottom-left-corner.png",
                           MPFImageLocation(20, 358, 30, 120, -1, { { "ROTATION", "20" } }));

    verifyCorrectlyRotated("160deg-bounding-box-bottom-right-corner.png",
                           MPFImageLocation(590, 358, 30, 120, -1, { { "ROTATION", "160" } }));

    verifyCorrectlyRotated("260deg-bounding-box-top-right-corner.png",
                           MPFImageLocation(563, -35, 30, 120, -1, { { "ROTATION", "260" } }));

    verifyCorrectlyRotated("270deg-bounding-box-top-right-corner.png",
                           MPFImageLocation(565, -45, 30, 120, -1, { { "ROTATION", "270" } }));
}




void draw_point(cv::Mat &img, const cv::Point2f &point, const cv::Scalar &color={0, 0, 255}) {
    cv::rectangle(img, point - cv::Point2f(1, 1), point + cv::Point2f(1, 1), color, 2);
}


cv::Point2d getCenter(const MPFImageLocation &loc) {
    double degrees = DetectionComponentUtils::GetProperty(loc.detection_properties, "ROTATION", 0.0);
    double radians = degrees * M_PI / 180.0;

    double sin_val = std::sin(radians);
    double cos_val = std::cos(radians);

    double center_x = loc.x_left_upper + loc.width / 2.0 * cos_val + loc.height / 2.0 * sin_val;
    double center_y = loc.y_left_upper - loc.width / 2.0 * sin_val + loc.height / 2.0 * cos_val;

    return cv::Point(cv::saturate_cast<int>(center_x), cv::saturate_cast<int>(center_y));
}


void draw_rotated_location(const MPFImageLocation &loc, cv::Mat &frame, const cv::Scalar& color = {255, 0, 0}) {
    const cv::Point2f top_left(loc.x_left_upper, loc.y_left_upper);
//    cv::Point2f center = top_left + cv::Point2f(loc.width, loc.height) / 2.0;
    cv::Point2d center = getCenter(loc);


    double angle = DetectionComponentUtils::GetProperty(loc.detection_properties, "ROTATION", 0.0);
    cv::RotatedRect rrect(center, cv::Size2d(loc.width, loc.height), 360 - angle);

    cv::Point2f corners[4];
    rrect.points(corners);
    draw_point(frame, center);

    cv::line(frame, corners[0], corners[1], color, 2);
    cv::line(frame, corners[1], corners[2], color, 2);
    cv::line(frame, corners[2], corners[3], color, 2);
    cv::line(frame, corners[3], corners[0], color, 2);

    draw_point(frame, top_left, {255, 255, 0});
}







double fix_angle(double angle) {
    return DetectionComponentUtils::NormalizeAngle(angle);
}

TEST(kfjlas, kjltk) {
    double angle1 = 20.5;
    double angle2 = 380.5;
    double angle3 = -339.5;
    double angle4 = -699.5;
    double angle5 = -1059.5;

    std::cout << "\nx: " << std::fmod(-1059.5, 360) << std::endl;

    ASSERT_DOUBLE_EQ(angle1, fix_angle(angle1));
    ASSERT_DOUBLE_EQ(angle1, fix_angle(angle2));
    ASSERT_DOUBLE_EQ(angle1, fix_angle(angle3));
    ASSERT_DOUBLE_EQ(angle1, fix_angle(angle4));
    ASSERT_DOUBLE_EQ(angle1, fix_angle(angle5));

    ASSERT_DOUBLE_EQ(0, fix_angle(0));
    ASSERT_DOUBLE_EQ(0, fix_angle(360));

    std::cout << "\n: " << fix_angle(angle2) << std::endl;
}


IFrameTransformer::Ptr getNoOp(const cv::Size &sz) {
    return IFrameTransformer::Ptr(new NoOpFrameTransformer(sz));
}


IFrameTransformer::Ptr getNoOp(const cv::Mat &img) {
    return getNoOp(img.size());
}



MPFImageLocation getFullFrameDetection(const std::string &imagePath, double rotation) {
//    rotation = 360 - rotation;
    cv::Mat img = cv::imread(imagePath);

    cv::Vec2d frameSize(img.cols, img.rows);

    cv::Point2d center = frameSize / 2.0;
    cv::Matx23d M = cv::getRotationMatrix2D(center, rotation, 1);

    cv::Vec2d newPosition = M * cv::Vec3d(0, 0, 1);

    std::cout << newPosition << std::endl;

    double radians = rotation * M_PI / 180.0;
    double sin_val = std::abs(std::sin(radians));
    double cos_val = std::abs(std::cos(radians));

    double newWidth = std::abs(frameSize[0] * cos_val + frameSize[1] * sin_val);
    double newHeight = std::abs(frameSize[0] * sin_val + frameSize[1] * cos_val);

    MPFImageLocation loc(
            cv::saturate_cast<int>(newPosition[0]),
            cv::saturate_cast<int>(newPosition[1]),
            cv::saturate_cast<int>(newWidth),
            cv::saturate_cast<int>(newHeight), -1, { { "ROTATION", std::to_string(rotation)} });
    return loc;
}



TEST(asdf, rotation_with_xformers2) {

    // input data from track
//    std::string test_file = "/home/mpf/sample-data/text-rotation/helloworld-small.png";
//    MPFImageLocation detection(156, 138, 175, 45, -1, { {"ROTATION", "-340"} });

    // input data from track
//    std::string test_file = "/home/mpf/sample-data/text-rotation/helloworld-cutoff.png";
//    MPFImageLocation detection(-57, -7, 175, 48, -1, { {"ROTATION", "20"} });


    // input data from track
//    std::string test_file = "/home/mpf/sample-data/text-rotation/helloworld-corner.png";
//    MPFImageLocation detection(0, 61, 170, 45, -1, { {"ROTATION", "20"} });


//    // full frame 90deg
//    std::string test_file = "/home/mpf/sample-data/text-rotation/90deg-text.png";
//    MPFImageLocation detection(0, 480, 480, 640, -1, { {"ROTATION", "90"} });
//    getFullFrameDetection(test_file, 90);

    // full frame 270deg (upside down)
//    std::string test_file = "/home/mpf/sample-data/text-rotation/90deg-text.png";
//    MPFImageLocation detection(640, 0, 480, 640, -1, { {"ROTATION", "270"} });

    // full frame 20deg
    std::string test_file = "/home/mpf/sample-data/text-rotation/helloworld-small.png";
//    MPFImageLocation detection(-37, 112, 596, 324, -1, { {"ROTATION", "20"} });
//    MPFImageLocation detection(-37, 112, 671, 508, -1, { {"ROTATION", "20"} });
    MPFImageLocation detection2(-103, 38, 671, 508, -1, { {"ROTATION", "20"} });
    MPFImageLocation detection = getFullFrameDetection(test_file, 20);

    // full frame
//    std::string test_file = "/home/mpf/sample-data/text-rotation/helloworld-small.png";
//    MPFImageLocation detection(0, 0, 596, 324, -1, { {"ROTATION", "0"} });


    cv::Mat img = cv::imread(test_file);
    cv::Mat original = img.clone();
    {
        cv::Mat temp = img.clone();
        cv::imshow("1. original", temp);
    }

    AffineFrameTransformer xformer(
            cv::Rect(detection.x_left_upper, detection.y_left_upper, detection.width, detection.height),
            std::stod(detection.detection_properties.at("ROTATION")),
            false,
            getNoOp(img));


    xformer.TransformFrame(img, 0);
    {
        cv::Mat temp = img.clone();
        std::cout << "transformed size: " << img.size() << std::endl;
        cv::imshow("2. transformed", temp);
    }

    MPFImageLocation downstream_detection(10, 8, 22, 30);
    {
        cv::Mat temp = img.clone();
        cv::Rect downstream_rect(downstream_detection.x_left_upper, downstream_detection.y_left_upper,
                                 downstream_detection.width, downstream_detection.height);
        cv::rectangle(temp, downstream_rect, {255, 0, 0}, 2);
        draw_point(temp, downstream_rect.tl() + cv::Point(downstream_rect.size() / 2));
        draw_point(temp, cv::Point2f(downstream_rect.tl()), {255, 255 , 0});
        cv::imshow("-2. transformed", temp);
    }

    {
        cv::Mat temp = original.clone();
        xformer.ReverseTransform(downstream_detection, 0);
        draw_rotated_location(downstream_detection, temp);
        draw_point(temp, cv::Point2f(downstream_detection.x_left_upper, downstream_detection.y_left_upper), {255, 255 , 0});
        cv::imshow("-1. original", temp);
    }

    cv::waitKey();
}

TEST(affine, test_affine_flip2) {
    std::cout << "\n" << std::endl;

    // [864 x 582]
//    std::string test_file = "/home/mpf/sample-data/Obama_Biden.jpg";
//    int rotation = 0;

    std::string test_file = "/home/mpf/openmpf-projects/openmpf/trunk/mpf-system-tests/src/test/resources/samples/face/meds-aa-S001-01-exif-rotation.jpg";
    int rotation = 90;

    cv::Mat img = cv::imread(test_file, cv::IMREAD_IGNORE_ORIENTATION + cv::IMREAD_COLOR);

    AffineFrameTransformer xformer(cv::Rect(cv::Point(0, 0), img.size()), rotation, true, getNoOp(img));
    std::cout << "img size: " << img.size() << std::endl;

    MPFImageLocation detection(10, 20, 400, 200);
    cv::Rect detectionRect(detection.x_left_upper, detection.y_left_upper, detection.width, detection.height);

    cv::Mat xformed = img.clone();
    xformer.TransformFrame(xformed, 0);
    MPFImageLocation xformed_detection = detection;
    xformer.ReverseTransform(xformed_detection, 0);
    cv::Rect xformedDetectionRect(detection.x_left_upper, detection.y_left_upper, detection.width, detection.height);

    draw_rotated_location(detection, xformed);
//    cv::rectangle(xformed, detectionRect, {0, 0, 255});
    draw_rotated_location(xformed_detection, img);
//    cv::rectangle(img, xformedDetectionRect, {0, 0, 255});

    cv::imshow("original", img);
    cv::imshow("xformed", xformed);


    cv::waitKey();
}

cv::Rect toRect(const MPFImageLocation &il) {
    return cv::Rect(il.x_left_upper, il.y_left_upper, il.width, il.height);
}


void print_corners(const std::string &msg, const cv::Rect &r) {
    std::cout << msg << ": " <<  std::endl;

    std::cout << "top left: " << r.tl() << std::endl;

    cv::Point topRight(r.x + r.width, r.y);
    std::cout << "top right: " << topRight << std::endl;

    std::cout << "bottom right: " << r.br() << std::endl;

    cv::Point bottomLeft(r.x, r.y + r.height);

    std::cout << "bottom left: " << bottomLeft << std::endl;

}


TEST(diagram, diagram_co_ords) {
    std::cout << "" << std::endl;
    cv::Size frameSize(4, 4);
    cv::Matx23d rotationMat = cv::getRotationMatrix2D(cv::Point2f(2, 2), 360 - 90, 1);

    rotationMat(0, 2) -= 1;
    rotationMat(1, 2) -= 1;

//    cv::Matx<char, 4, 4> mat('A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P');

    cv::Matx<char, 4, 4> mat('X', 'X', 'X', 'X',
                             'X', 'A', 'B', 'C',
                             'X', 'D', 'E', 'F',
                             'X', 'G', 'H', 'I' );

    std::cout << mat << std::endl;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << mat(i, j) << " ";
        }
        std::cout << "\n";
    }


    cv::Matx<char, 4, 4> result;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            result(i, j) = '?';
        }
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            cv::Vec2i newLocation = rotationMat * cv::Vec3d(i, j, 1);
            if (newLocation[0] >= 0 && newLocation[0] < 4 && newLocation[1] >= 0 && newLocation[1] < 4) {
                result(newLocation[0], newLocation[1]) = mat(i, j);
            }
        }
        std::cout << "\n";
    }

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            std::cout << result(i, j) << " ";
        }
        std::cout << "\n";
    }
}

/*

std::array<cv::Point, 4> draw_detection_filled_in(cv::Mat &img, const MPFImageLocation &loc) {
    cv::Point2d top_left(loc.x_left_upper, loc.y_left_upper);
    cv::Point2d center = top_left + cv::Point2d(loc.width, loc.height) / 2.0;
    double angle = DetectionComponentUtils::GetProperty(loc.detection_properties, "ROTATION", 0.0);

    int padding = 0;
    cv::RotatedRect rrectToDraw(center, cv::Size2d(loc.width + padding, loc.height + padding), 360 - angle);

    cv::Point2f pointsToDraw2f[4];
    rrectToDraw.points(pointsToDraw2f);

    std::array<cv::Point, 4> pointsToDraw;
    for (int i = 0; i < 4; i++) {
        pointsToDraw[i] = pointsToDraw2f[i];
    }

//    cv::fillConvexPoly(img, pointsToDraw, cv::Scalar(255, 0, 0));
    cv::fillConvexPoly(img, pointsToDraw, cv::Scalar(255, 0, 0), cv::LINE_4);


    cv::RotatedRect actualRect(center, cv::Size2d(loc.width, loc.height), 360 - angle);
    cv::Point2f tempPoints[4];
    actualRect.points(tempPoints);

    std::array<cv::Point, 4> resultPoints;
    for (int i = 0; i < 4; i++) {
        resultPoints[i] = tempPoints[i];
    }

    return resultPoints;
}

std::string get_corners_string(const std::array<cv::Point, 4> &points) {
    std::stringstream ss;
    ss << "Corners: ";
    ss << "(" << points[0].x << ", " << points[0].y << "), ";
    ss << "(" << points[1].x << ", " << points[1].y << "), ";
    ss << "(" << points[2].x << ", " << points[2].y << "), ";
    ss << "(" << points[3].x << ", " << points[3].y << ")";

    return ss.str();
}


TEST(generate_test_media, create_media) {
    std::cout << "\n" << std::endl;

    cv::Mat img(cv::Size(640, 480), CV_8UC3, cv::Scalar(255, 255, 255));

//    MPFImageLocation detection(-8, 33, 100, 40, -1, { { "ROTATION", "60" } });
    MPFImageLocation detection(0, 86, 100, 40, -1, { { "ROTATION", "60" } });

    std::array<cv::Point, 4> points = draw_detection_filled_in(img, detection);

    std::stringstream det_ss;
    det_ss << "MPFImageLocation(" << detection.x_left_upper << ", " << detection.y_left_upper << ", "
            << detection.width << ", " << detection.height << ", " << detection.confidence
            << ", { { \"ROTATION\", \"" << detection.detection_properties["ROTATION"] << "\" } })"; // NOLINT(modernize-raw-string-literal)


    cv::Point text_point(30, 120);
    cv::putText(img, det_ss.str(), text_point,
                cv::HersheyFonts::FONT_HERSHEY_DUPLEX, .5, {0, 0, 0}, 1, cv::LINE_AA);


    std::string corners_str = get_corners_string(points);
    std::cout << corners_str << std::endl;


    cv::putText(img, corners_str, cv::Point(text_point.x, text_point.y + 30),
                cv::HersheyFonts::FONT_HERSHEY_DUPLEX, .5, {0, 0, 0}, 1, cv::LINE_AA);


//    std::string filename = "x270deg-bounding-box-top-right-corner.png";
//    cv::imwrite("/home/mpf/openmpf-projects/openmpf-cpp-component-sdk/detection/api/test/test_imgs/rotation/" + filename, img);

    cv::imshow("Test", img);
    cv::waitKey();
}
*/


/*

TEST(rotation, compare_rotation_methods) {
    std::cout << "\n" << std::endl;

//    MPFImageLocation orginal(10, 20, 100, 300);
//    MPFImageLocation orginal(0, 0, 100, 200);
    MPFImageLocation orginal(1, 2, 150, 250);
    cv::Size frameSize(640, 480);
    cv::Rect frameRect(cv::Point(0, 0), frameSize);
    double angle = 90;
//    double angle = 270;


    AffineFrameTransformer xformer(frameRect, angle, false, getNoOp(frameSize));

    FrameRotator rotator(getNoOp(frameSize), angle);

    cv::Mat originalImg(frameSize, CV_8UC3, {0, 0, 0});


    cv::Rect affineRect;
    {
        MPFImageLocation temp = orginal;
        xformer.ReverseTransform(temp, 0);
        affineRect = toRect(temp);
        std::cout << "affine: " << affineRect << std::endl;

        cv::Mat tmpImg = originalImg.clone();
        draw_rotated_location(temp, tmpImg);

        temp.detection_properties["ROTATION"] = "";
        draw_rotated_location(temp, tmpImg);

        cv::imshow("affine", tmpImg);
    }

    cv::Rect rotatorRect;
    {
        MPFImageLocation temp = orginal;
        rotator.ReverseTransform(temp, 0);
        rotatorRect = toRect(temp);
        std::cout << "rotator: " << rotatorRect << std::endl;

        cv::Mat tmpImg = originalImg.clone();
        draw_rotated_location(temp, tmpImg);
        cv::imshow("rotator", tmpImg);
    }
    cv::waitKey();

    print_corners("affine", affineRect);
    ASSERT_EQ(affineRect.x, rotatorRect.x);
    ASSERT_EQ(affineRect.y, rotatorRect.y);
    ASSERT_EQ(affineRect.width, rotatorRect.width);
    ASSERT_EQ(affineRect.height, rotatorRect.height);

}

*/

