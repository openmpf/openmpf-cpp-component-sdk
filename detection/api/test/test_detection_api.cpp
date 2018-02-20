/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2018 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2018 The MITRE Corporation                                       *
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
    long frameCount = filter.GetSegmentFrameCount();

    MPFVideoTrack feedForwardTrack;
    for (long i = 0; i < frameCount; i++) {
        feedForwardTrack.frame_locations[filter.SegmentToOriginalFramePosition(i)] = { };
    }

    MPFVideoJob job("Test", "Test", 0, 0, feedForwardTrack, {}, {});
    return FeedForwardFrameFilter(job.feed_forward_track);
}



void assertSegmentFrameCount(const IntervalFrameFilter &frameFilter, long expected) {
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



void assertSegmentToOriginalFramePosition(const IntervalFrameFilter &filter, long segmentPosition,
                                          long expectedOriginalPosition) {

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


void assertOriginalToSegmentFramePosition(const IntervalFrameFilter &filter, long originalPosition,
                                          long expectedSegmentPosition) {

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


void assertCurrentSegmentTime(const IntervalFrameFilter &filter, long originalPosition,
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



void assertSegmentFramePositionRatio(const IntervalFrameFilter &filter, long originalPosition,
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



void assertRatioToOriginalFramePosition(const IntervalFrameFilter &filter, double ratio, long expectedFramePosition) {
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
                                        double segmentMillis, long expectedSegmentPos) {

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


void assertAvailableInitializationFrames(long startFrame, long frameInterval, long expectedNumAvailable) {

    IntervalFrameFilter filter(startFrame, startFrame + 10, frameInterval);
    long actualNumAvailable = filter.GetAvailableInitializationFrameCount();
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


long GetFrameNumber(const cv::Mat &frame)  {
    cv::Scalar_<uchar> color = frame.at<cv::Scalar_<uchar>>(0, 0);
    return static_cast<long>(color.val[0]);
}



MPFVideoCapture CreateVideoCapture(long startFrame, long stopFrame) {
    MPFVideoJob job("Test", frameFilterTestVideo, startFrame, stopFrame, {}, {} );
    return MPFVideoCapture(job, true, true);
}


MPFVideoJob CreateVideoJob(long startFrame, long stopFrame, long frameInterval) {
    return {"Test", frameFilterTestVideo, startFrame, stopFrame,
            {{"FRAME_INTERVAL", std::to_string(frameInterval)}}, {}};

}

MPFVideoCapture CreateVideoCapture(long startFrame, long stopFrame, long frameInterval) {
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


void assertExpectedFramesShown(MPFVideoCapture &cap, const vector<long> &expectedFrames) {
    ASSERT_EQ(expectedFrames.size(), cap.GetFrameCount());

    for (long expectedFrame : expectedFrames) {
        cv::Mat frame;
        bool wasRead = cap.Read(frame);
        ASSERT_TRUE(wasRead);
        ASSERT_EQ(expectedFrame, GetFrameNumber(frame));
    }
    assertReadFails(cap);
}


void assertExpectedFramesShown(long startFrame, long stopFrame, long frameInterval,
                               const vector<long> &expectedFrames) {

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



void assertMapContainsKeys(const map<long, MPFImageLocation> &map, const std::vector<long> &expectedKeys) {

    ASSERT_EQ(map.size(), expectedKeys.size());
    for (const long key : expectedKeys) {
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



void assertInitializationFrameIds(long startFrame, long frameInterval, long numberRequested,
                                  const vector<long> &expectedInitFrames) {
    auto cap = CreateVideoCapture(startFrame, 29, frameInterval);

    const vector<cv::Mat> &initFrames = cap.GetInitializationFramesIfAvailable(numberRequested);

    ASSERT_EQ(expectedInitFrames.size(), initFrames.size());

    auto initFramesIter = initFrames.begin();
    for (long expectedFrameNum : expectedInitFrames) {
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



void assertFrameRead(MPFVideoCapture &cap, long expectedFrameNumber, const cv::Size &expectedSize,
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

    auto framePosition = static_cast<long>(cap.get(cv::CAP_PROP_POS_FRAMES));

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

    auto frameCount = static_cast<long>(cap.get(cv::CAP_PROP_FRAME_COUNT));

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
    long framePosition = 0;

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
    long framePos = cap.GetCurrentFramePosition();
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

    assertDetectionLocationsMatch(outputTrack.frame_locations.at(4), outputTrack.frame_locations.at(4));
    assertDetectionLocationsMatch(outputTrack.frame_locations.at(15), outputTrack.frame_locations.at(15));

    const auto &lastDetection = outputTrack.frame_locations.at(29);
    ASSERT_EQ(lastDetection.x_left_upper, 75);
    ASSERT_EQ(lastDetection.y_left_upper, 40);
    ASSERT_EQ(lastDetection.width, 15);
    ASSERT_EQ(lastDetection.height, 60);
}