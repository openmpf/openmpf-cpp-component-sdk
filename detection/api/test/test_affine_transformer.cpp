/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2024 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2024 The MITRE Corporation                                       *
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
#include <cmath>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include <gtest/gtest.h>

#include "detectionComponentUtils.h"
#include "frame_transformers/AffineFrameTransformer.h"
#include "frame_transformers/FrameTransformerFactory.h"
#include "frame_transformers/NoOpFrameTransformer.h"
#include "MPFAsyncVideoCapture.h"
#include "MPFDetectionComponent.h"
#include "MPFDetectionObjects.h"
#include "MPFImageReader.h"
#include "MPFVideoCapture.h"


using namespace MPF::COMPONENT;

using Pixel = cv::Vec<uint8_t, 3>;

Pixel closestColor(const Pixel &sample) {
    std::vector<Pixel> colors {
            { 0, 0, 0 },
            { 255, 255, 255},
            { 255, 0, 0},
            { 0, 255, 0},
            { 0, 0, 255}
    };

    int s1 = sample[0];
    int s2 = sample[1];
    int s3 = sample[2];

    double minDist = 255 * 3;
    Pixel closestColor;

    for (const auto &color : colors) {
        int c1 = color[0];
        int c2 = color[1];
        int c3 = color[2];

        double distSquared = (c1 - s1) * (c1 - s1) + (c2 - s2) * (c2 - s2) + (c3 - s3) * (c3 - s3);

        double dist = std::sqrt(distSquared);

        if (dist < minDist) {
            minDist = dist;
            closestColor = color;
        }
    }
    return closestColor;
}


void assertImageColorFuzzy(const cv::Mat &img, const Pixel &expectedColor) {
    ASSERT_FALSE(img.empty());
    // Color of pixels along edges gets blended with nearby pixels during interpolation.
    for (int col = 1; col < img.cols; col++) {
        for (int row = 1; row < img.rows; row++)  {
            ASSERT_EQ(closestColor(img.at<Pixel>(row, col)), expectedColor) << "row = " << row << ", col = " << col;
        }
    }
}

void assertImageColor(const cv::Mat &img, const Pixel &expectedColor) {
    ASSERT_FALSE(img.empty());
    for (int col = 0; col < img.cols; col++) {
        for (int row = 0; row < img.rows; row++) {
            ASSERT_EQ(expectedColor, img.at<Pixel>(row, col));
        }
    }
}


namespace {
    bool isSameImage(const cv::Mat &im1, const cv::Mat &im2) {
        if (im1.size() != im2.size()) {
            return false;
        }
        return std::equal(im1.begin<Pixel>(), im1.end<Pixel>(), im2.begin<Pixel>());
    }
}


void assertDetectionsSameLocation(const MPFImageLocation &il1, const MPFImageLocation &il2) {
    ASSERT_NEAR(il1.x_left_upper, il2.x_left_upper, 1);
    ASSERT_NEAR(il1.y_left_upper, il2.y_left_upper, 1);
    ASSERT_NEAR(il1.width, il2.width, 1);
    ASSERT_NEAR(il1.height, il2.height, 1);
}


void verifyCorrectlyRotated(const std::string &fileName,  const MPFImageLocation &feedForwardDetection) {
    MPFImageJob job("Test", "test/test_imgs/rotation/" + fileName, feedForwardDetection,
                    { { "FEED_FORWARD_TYPE", "REGION" } }, {});

    MPFImageReader imageReader(job);

    cv::Mat img = imageReader.GetImage();


    ASSERT_EQ(feedForwardDetection.width, img.cols);
    ASSERT_EQ(feedForwardDetection.height, img.rows);

    assertImageColorFuzzy(img, Pixel(255, 0, 0));

    MPFImageLocation detection(0, 0, img.size().width, img.size().height);
    imageReader.ReverseTransform(detection);

    assertDetectionsSameLocation(detection, feedForwardDetection);
}



TEST(AffineFrameTransformerTest, CanHandleRotatedDetectionNearMiddle) {
    verifyCorrectlyRotated("20deg-bounding-box.png",
                           MPFImageLocation(116, 218, 100, 40, -1, { { "ROTATION", "20" } }));
}


TEST(AffineFrameTransformerTest, CanHandleRotatedDetectionTouchingCorner) {
    verifyCorrectlyRotated("30deg-bounding-box-top-left-corner.png",
                           MPFImageLocation(0, 51, 100, 40, -1, { { "ROTATION", "30.5" } }));

    verifyCorrectlyRotated("60deg-bounding-box-top-left-corner.png",
                           MPFImageLocation(0, 86, 100, 40, -1, { { "ROTATION", "60" } }));

    verifyCorrectlyRotated("200deg-bounding-box-top-left-corner.png",
                           MPFImageLocation(108, 38, 100, 40, -1, { { "ROTATION", "200" } }));

    verifyCorrectlyRotated("20deg-bounding-box-bottom-left-corner.png",
                           MPFImageLocation(0, 367, 30, 120, -1, { { "ROTATION", "20" } }));

    verifyCorrectlyRotated("160deg-bounding-box-bottom-right-corner.png",
                           MPFImageLocation(599, 480, 30, 120, -1, { { "ROTATION", "160" } }));

    verifyCorrectlyRotated("260deg-bounding-box-top-right-corner.png",
                           MPFImageLocation(640, 21, 30, 120, -1, { { "ROTATION", "260" } }));

    verifyCorrectlyRotated("270deg-bounding-box-top-right-corner.png",
                           MPFImageLocation(640, 0, 30, 120, -1, { { "ROTATION", "270" } }));
}


TEST(AffineFrameTransformerTest, FullFrameRotationNonOrthogonal) {
    Pixel white(255, 255, 255);

    cv::Size size(640, 480);
    cv::Mat img(size, CV_8UC3, white);

    AffineFrameTransformer transformer(20, false, {0, 0, 0},
                                       IFrameTransformer::Ptr(new NoOpFrameTransformer(size)));

    transformer.TransformFrame(img, 0);

    long numWhite = std::count(img.begin<Pixel>(), img.end<Pixel>(), white);

    // Color of pixels along edges gets blended with nearby pixels during interpolation.
    ASSERT_GE(numWhite, size.area() - size.width - size.height);
    ASSERT_LE(numWhite, size.area());
    ASSERT_EQ(cv::Size(765, 670), img.size());
}



TEST(AffineFrameTransformerTest, FullFrameRotationOrthogonal) {
    Pixel white(255, 255, 255);

    cv::Size size(640, 480);
    const cv::Mat img(size, CV_8UC3, white);

    for (int rotation : { 0, 90, 180, 270 }) {
        AffineFrameTransformer transformer(rotation, false, {0, 0, 0},
                                           IFrameTransformer::Ptr(new NoOpFrameTransformer(size)));

        cv::Mat transformed = img.clone();
        transformer.TransformFrame(transformed, 0);

        long numWhite = std::count(transformed.begin<Pixel>(), transformed.end<Pixel>(), white);
        ASSERT_EQ(numWhite, size.area());

        if (rotation == 90 || rotation == 270) {
            ASSERT_EQ(cv::Size(size.height, size.width), transformed.size());
        }
        else {
            ASSERT_EQ(size, transformed.size());
        }

    }
}


TEST(AffineFrameTransformerTest, TestFeedForwardExactRegion) {
    MPFVideoTrack ffTrack(0, 2);
    ffTrack.frame_locations.emplace(0, MPFImageLocation(60, 300, 100, 40, -1, { { "ROTATION", "260" } }));
    ffTrack.frame_locations.emplace(1, MPFImageLocation(160, 350, 130, 20, -1, { { "ROTATION", "60" } }));
    ffTrack.frame_locations.emplace(2, MPFImageLocation(260, 340, 60, 60, -1, { { "ROTATION", "20" } }));

    MPFVideoJob job("Test", "test/test_imgs/rotation/feed-forward-rotation-test.png",
                    ffTrack.start_frame, ffTrack.stop_frame, ffTrack,
                    { {"FEED_FORWARD_TYPE", "REGION"} }, {});

    const cv::Mat testImg = cv::imread(job.data_uri);

    IFrameTransformer::Ptr transformer = FrameTransformerFactory::GetTransformer(job, testImg.size());

    for (const auto &pair : ffTrack.frame_locations) {
        int frameNumber = pair.first;
        const auto &ffDetection = pair.second;

        cv::Mat frame = testImg.clone();
        transformer->TransformFrame(frame, frameNumber);
        ASSERT_EQ(frame.size(), cv::Size(ffDetection.width, ffDetection.height));

        assertImageColorFuzzy(frame, Pixel(255, 0, 0));

        MPFImageLocation newDetection(0, 0, frame.size().width, frame.size().height);
        transformer->ReverseTransform(newDetection, frameNumber);
        assertDetectionsSameLocation(newDetection, ffDetection);
    }
}


TEST(AffineFrameTransformerTest, TestFeedForwardSupersetRegion) {
    MPFVideoTrack ffTrack(0, 2);
    ffTrack.frame_locations.emplace(0, MPFImageLocation(60, 300, 100, 40, -1, { { "ROTATION", "260" } }));
    ffTrack.frame_locations.emplace(1, MPFImageLocation(160, 350, 130, 20, -1, { { "ROTATION", "60" } }));
    ffTrack.frame_locations.emplace(2, MPFImageLocation(260, 340, 60, 60, -1, { { "ROTATION", "20" } }));


    for (int rotation = 0; rotation <= 360; rotation += 20) {
        MPFVideoJob job("Test", "test/test_imgs/rotation/feed-forward-rotation-test.png",
                        ffTrack.start_frame, ffTrack.stop_frame, ffTrack,
                        { {"FEED_FORWARD_TYPE", "SUPERSET_REGION" }, { "ROTATION", std::to_string(rotation) } }, {});

        int expectedMinNumBlue = 0;
        int expectedMaxNumBlue = 0;
        for (const auto &pair : ffTrack.frame_locations) {
            int area = pair.second.height * pair.second.width;
            int perimeter = 2 * pair.second.height + 2 * pair.second.width;
            expectedMinNumBlue += area - perimeter;
            expectedMaxNumBlue += area + perimeter;
        }

        MPFVideoCapture cap(job);

        cv::Mat img;
        cap.Read(img);

        long actualNumBlue = std::count(img.begin<Pixel>(), img.end<Pixel>(), Pixel(255, 0, 0));

        // Color of pixels along edges gets blended with nearby pixels during interpolation.
        ASSERT_LE(actualNumBlue, expectedMaxNumBlue);
        ASSERT_GE(actualNumBlue, expectedMinNumBlue);
    }
}


TEST(AffineFrameTransformerTest, TestFeedForwardSupersetRegionWithAsyncVideoCapture) {
    MPFVideoTrack ffTrack(0, 2);
    ffTrack.frame_locations.emplace(0, MPFImageLocation(60, 300, 100, 40, -1, { { "ROTATION", "260" } }));
    ffTrack.frame_locations.emplace(1, MPFImageLocation(160, 350, 130, 20, -1, { { "ROTATION", "60" } }));
    ffTrack.frame_locations.emplace(2, MPFImageLocation(260, 340, 60, 60, -1, { { "ROTATION", "20" } }));


    for (int rotation = 0; rotation <= 360; rotation += 20) {
        MPFVideoJob job("Test", "test/test_imgs/rotation/feed-forward-rotation-test.png",
                        ffTrack.start_frame, ffTrack.stop_frame, ffTrack,
                        { {"FEED_FORWARD_TYPE", "SUPERSET_REGION" }, { "ROTATION", std::to_string(rotation) } }, {});

        int expectedMinNumBlue = 0;
        int expectedMaxNumBlue = 0;
        for (const auto &pair : ffTrack.frame_locations) {
            int area = pair.second.height * pair.second.width;
            int perimeter = 2 * pair.second.height + 2 * pair.second.width;
            expectedMinNumBlue += area - perimeter;
            expectedMaxNumBlue += area + perimeter;
        }

        MPFAsyncVideoCapture cap(job);
        auto frame = cap.Read();

        long actualNumBlue = std::count(frame->data.begin<Pixel>(), frame->data.end<Pixel>(),
                                        Pixel(255, 0, 0));

        // Color of pixels along edges gets blended with nearby pixels during interpolation.
        ASSERT_LE(actualNumBlue, expectedMaxNumBlue);
        ASSERT_GE(actualNumBlue, expectedMinNumBlue);
    }
}


TEST(AffineFrameTransformerTest, ReverseTransformWithFlip) {
    int frameWidth = 100;
    int frameHeight = 200;

    AffineTransformation transformation(
            { MPFRotatedRect(0, 0, frameWidth, frameHeight, 0, false) },
            0, true, {0, 0, 0});

    {
        // Test without existing flip.
        MPFImageLocation detection(10, 20, 40, 50);
        MPFImageLocation detectionReversed = detection;
        transformation.ApplyReverse(detectionReversed);

        ASSERT_EQ(frameWidth - detection.x_left_upper - 1, detectionReversed.x_left_upper);
        ASSERT_EQ(detection.y_left_upper, detectionReversed.y_left_upper);
        ASSERT_EQ(detection.width, detectionReversed.width);
        ASSERT_EQ(detection.height, detectionReversed.height);
        ASSERT_EQ(1, detectionReversed.detection_properties.count("HORIZONTAL_FLIP"));
        ASSERT_TRUE(DetectionComponentUtils::GetProperty(
                detectionReversed.detection_properties, "HORIZONTAL_FLIP", false));
    }


    {
        // Test with existing flip.
        MPFImageLocation detection(10, 20, 40, 50, -1, { { "HORIZONTAL_FLIP", "true" } });
        MPFImageLocation detectionReversed = detection;
        transformation.ApplyReverse(detectionReversed);

        ASSERT_EQ(frameWidth - detection.x_left_upper - 1, detectionReversed.x_left_upper);
        ASSERT_EQ(detection.y_left_upper, detectionReversed.y_left_upper);
        ASSERT_EQ(detection.width, detectionReversed.width);
        ASSERT_EQ(detection.height, detectionReversed.height);
        ASSERT_EQ(0, detectionReversed.detection_properties.count("HORIZONTAL_FLIP"));
    }
}


TEST(AffineFrameTransformerTest, FlipRotateFullFrame) {
    double frameRotation = 345;
    MPFImageJob job("test", "test/test_imgs/rotation/hello-world-flip.png", {
            { "HORIZONTAL_FLIP", "true" },
            { "ROTATION", std::to_string(frameRotation) }
    }, {});

    MPFImageReader imageReader(job);
    auto image = imageReader.GetImage();
    MPFImageLocation il(0, 0, image.cols, image.rows);
    imageReader.ReverseTransform(il);

    ASSERT_EQ(836, il.x_left_upper);
    ASSERT_EQ(38, il.y_left_upper);
    ASSERT_EQ(image.cols, il.width);
    ASSERT_EQ(image.rows, il.height);
    ASSERT_EQ("true", il.detection_properties.at("HORIZONTAL_FLIP"));
    ASSERT_DOUBLE_EQ(360 - frameRotation, std::stod(il.detection_properties.at("ROTATION")));
}


TEST(AffineFrameTransformerTest, RotateFullFrame) {
    double frame_rotation = 15;
    MPFImageJob job("test", "test/test_imgs/rotation/hello-world.png", {
            { "ROTATION", std::to_string(frame_rotation) }
    }, {});

    MPFImageReader image_reader(job);
    auto image = image_reader.GetImage();
    MPFImageLocation il(0, 0, image.cols, image.rows, 1, {});
    image_reader.ReverseTransform(il);

    ASSERT_EQ(-141, il.x_left_upper);
    ASSERT_EQ(38, il.y_left_upper);
    ASSERT_EQ(image.cols, il.width);
    ASSERT_EQ(image.rows, il.height);
    ASSERT_EQ(0, il.detection_properties.count("HORIZONTAL_FLIP"));
    ASSERT_DOUBLE_EQ(frame_rotation, std::stod(il.detection_properties.at("ROTATION")));
}


TEST(AffineFrameTransformerTest, TestRotationThreshold) {
    const auto *test_img_path = "test/test_imgs/rotation/hello-world.png";
    auto original_img = cv::imread(test_img_path);
    {
        MPFImageJob job("test", test_img_path,
                        { {"ROTATION", "10"}, {"ROTATION_THRESHOLD", "10.001"} }, {});

        auto img = MPFImageReader(job).GetImage();
        ASSERT_TRUE(isSameImage(original_img, img));
    }
    {
        MPFImageJob job("test", test_img_path,
                        { {"ROTATION", "10"}, {"ROTATION_THRESHOLD", "9.99"} }, {});

        auto img = MPFImageReader(job).GetImage();
        ASSERT_FALSE(isSameImage(original_img, img));
    }
}


TEST(AffineFrameTransformerTest, TestRotationThresholdWithFeedForward) {
    const auto *test_img_path = "test/test_imgs/rotation/hello-world.png";
    auto original_img = cv::imread(test_img_path);

    MPFImageLocation ff_img_loc(0, 0, original_img.cols, original_img.rows, -1,
                                { {"ROTATION", "354.9"} });
    {
        MPFImageJob job("test", test_img_path, ff_img_loc,
                        { {"ROTATION_THRESHOLD", "5.12"}, {"FEED_FORWARD_TYPE", "REGION"} }, {});
        auto img = MPFImageReader(job).GetImage();
        ASSERT_TRUE(isSameImage(original_img, img));
    }
    {
        MPFImageJob job("test", test_img_path, ff_img_loc,
                        { {"ROTATION_THRESHOLD", "5.00"}, {"FEED_FORWARD_TYPE", "REGION"} }, {});
        auto img = MPFImageReader(job).GetImage();
        ASSERT_FALSE(isSameImage(original_img, img));
    }
}


TEST(AffineFrameTransformerTest, TestRotationFillColor) {
    const auto *test_img_path = "test/test_imgs/rotation/hello-world.png";
    {
        MPFImageJob job("test", test_img_path,
                        { {"ROTATION", "45"} }, {});
        auto img = MPFImageReader(job).GetImage();
        auto top_left_color = img.at<Pixel>(0, 0);
        ASSERT_EQ(Pixel(0, 0, 0), top_left_color);
    }
    {
        MPFImageJob job("test", test_img_path,
                        { {"ROTATION", "45"}, {"ROTATION_FILL_COLOR", "BLACK"} }, {});
        auto img = MPFImageReader(job).GetImage();
        auto top_left_color = img.at<Pixel>(0, 0);
        ASSERT_EQ(Pixel(0, 0, 0), top_left_color);
    }
    {
        MPFImageJob job("test", test_img_path,
                        { {"ROTATION", "45"}, {"ROTATION_FILL_COLOR", "WHITE"} }, {});
        auto img = MPFImageReader(job).GetImage();
        auto top_left_color = img.at<Pixel>(0, 0);
        ASSERT_EQ(Pixel(255, 255, 255), top_left_color);
    }
}


TEST(AffineFrameTransformerTest, SearchRegionWithOrthogonalRotation) {

    Properties absoluteProps {
            {"SEARCH_REGION_ENABLE_DETECTION", "true"},
            {"SEARCH_REGION_TOP_LEFT_X_DETECTION", "100"},
            {"SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION", "50"}
    };

    Properties percentageProps {
            {"SEARCH_REGION_ENABLE_DETECTION", "true"},
            {"SEARCH_REGION_TOP_LEFT_X_DETECTION", "50%"},
            {"SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION", "50%"}
    };

    Properties rotate90props {
            {"ROTATION", "90"},
            {"SEARCH_REGION_ENABLE_DETECTION", "true"},
            {"SEARCH_REGION_TOP_LEFT_X_DETECTION", "50"},
            {"SEARCH_REGION_TOP_LEFT_Y_DETECTION", "50%"},
    };

    Properties rotate180props {
            {"ROTATION", "180"},
            {"SEARCH_REGION_ENABLE_DETECTION", "true"},
            {"SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION", "50%"},
            {"SEARCH_REGION_TOP_LEFT_Y_DETECTION", "50%"},
    };

    Properties rotate270Props {
            {"ROTATION", "270"},
            {"SEARCH_REGION_ENABLE_DETECTION", "true"},
            {"SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION", "50%"},
            {"SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION", "50%"},
    };


    Properties flipProps {
            {"HORIZONTAL_FLIP", "true"},
            {"SEARCH_REGION_ENABLE_DETECTION", "true"},
            {"SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION", "50%"},
            {"SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION", "50%"}
    };


    Properties rotate90AndFlipProps {
            {"ROTATION", "90"},
            {"HORIZONTAL_FLIP", "true"},
            {"SEARCH_REGION_ENABLE_DETECTION", "true"},
            {"SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION", "50%"},
            {"SEARCH_REGION_TOP_LEFT_Y_DETECTION", "50%"},
    };

    Properties rotate180AndFlipProps {
            {"ROTATION", "180"},
            {"HORIZONTAL_FLIP", "true"},
            {"SEARCH_REGION_ENABLE_DETECTION", "true"},
            {"SEARCH_REGION_TOP_LEFT_X_DETECTION", "50%"},
            {"SEARCH_REGION_TOP_LEFT_Y_DETECTION", "50%"},
    };

    Properties rotate270AndFlipProps {
            {"ROTATION", "270"},
            {"HORIZONTAL_FLIP", "true"},
            {"SEARCH_REGION_ENABLE_DETECTION", "true"},
            {"SEARCH_REGION_TOP_LEFT_X_DETECTION", "50%"},
            {"SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION", "50%"},
    };

    const auto propSets = {
            absoluteProps, percentageProps, rotate90props, rotate270Props, rotate180props,
            flipProps, rotate90AndFlipProps, rotate180AndFlipProps, rotate270AndFlipProps };

    for (const auto &props : propSets) {
        MPFImageJob job("Test", "test/test_imgs/rotation/search-region-test.png", props, {});
        cv::Mat img = MPFImageReader(job).GetImage();

        long numGreen = std::count(img.begin<Pixel>(), img.end<Pixel>(), Pixel(0, 255, 0));
        ASSERT_EQ(5000, img.total());
        ASSERT_EQ(5000, numGreen);
    }
}


TEST(AffineFrameTransformerTest, SearchRegionWithNonOrthogonalRotation) {
    MPFImageJob job("Test", "test/test_imgs/rotation/20deg-bounding-box.png",
                    {
                            {"ROTATION", "20"},
                            {"SEARCH_REGION_ENABLE_DETECTION", "true"},
                            {"SEARCH_REGION_TOP_LEFT_X_DETECTION", "199"},
                            {"SEARCH_REGION_TOP_LEFT_Y_DETECTION", "245"},
                            {"SEARCH_REGION_BOTTOM_RIGHT_X_DETECTION", "299"},
                            {"SEARCH_REGION_BOTTOM_RIGHT_Y_DETECTION", "285"},
                    }, {});

    MPFImageReader imageReader(job);
    auto img = imageReader.GetImage();
    assertImageColorFuzzy(img, {255, 0, 0});

    MPFImageLocation il(0, 0, img.cols, img.rows);
    imageReader.ReverseTransform(il);
    ASSERT_EQ(117, il.x_left_upper);
    ASSERT_EQ(218, il.y_left_upper);
    ASSERT_EQ(100, il.width);
    ASSERT_EQ(40, il.height);
    double actualRotation = DetectionComponentUtils::GetProperty(il.detection_properties, "ROTATION", 0.0);
    ASSERT_TRUE(DetectionComponentUtils::RotationAnglesEqual(20, actualRotation));
}


TEST(AffineFrameTransformerTest, AutoOrientationPrefersJobProperty) {
    std::string img_path = "test/test_imgs/test_img.png";
    {
        MPFImageJob job("test", img_path,
                        {{ "ROTATION", "90" }, { "HORIZONTAL_FLIP", "TRUE" }},
                        {{ "ROTATION", "180" }, { "HORIZONTAL_FLIP", "FALSE" }});
        auto img = MPFImageReader(job).GetImage();
        ASSERT_EQ(cv::Size(200, 320), img.size());
        auto black_corner = img(cv::Rect(cv::Point(170, 300), cv::Point(img.cols, img.rows)));
        assertImageColor(black_corner, { 0, 0, 0 });
    }
    {
        MPFImageJob job("test", img_path,
                        {{ "ROTATION", "0" }, { "HORIZONTAL_FLIP", "FALSE" }},
                        {{ "ROTATION", "90" }, { "HORIZONTAL_FLIP", "TRUE" }});
        auto img = MPFImageReader(job).GetImage();
        ASSERT_EQ(cv::Size(320, 200), img.size());
        auto black_corner = img(cv::Rect(cv::Point(300, 170), cv::Point(img.cols, img.rows)));
        assertImageColor(black_corner, { 0, 0, 0 });
    }
}


TEST(AffineFrameTransformerTest, AutoOrientationIsDefault) {
    std::string img_path = "test/test_imgs/test_img.png";
    MPFImageJob job("test", img_path, {}, {{ "ROTATION", "90" }, { "HORIZONTAL_FLIP", "TRUE" }});
    auto img = MPFImageReader(job).GetImage();
    ASSERT_EQ(cv::Size(200, 320), img.size());
    auto black_corner = img(cv::Rect(cv::Point(170, 300), cv::Point(img.cols, img.rows)));
    assertImageColor(black_corner, { 0, 0, 0 });
}


TEST(AffineFrameTransformerTest, MediaPropsIgnoredWithoutAutoOrientation) {
    std::string img_path = "test/test_imgs/test_img.png";
    MPFImageJob job("test", img_path,
                    {{"AUTO_ROTATE", "FALSE"}, {"AUTO_FLIP", "FALSE"}},
                    {{ "ROTATION", "90" }, { "HORIZONTAL_FLIP", "TRUE" }});
    auto img = MPFImageReader(job).GetImage();
    ASSERT_EQ(cv::Size(320, 200), img.size());
    auto black_corner = img(cv::Rect(cv::Point(300, 170), cv::Point(img.cols, img.rows)));
    assertImageColor(black_corner, { 0, 0, 0 });
}
