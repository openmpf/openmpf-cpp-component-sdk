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


#include "MPFDetectionObjects.h"
#include "detectionComponentUtils.h"

#include <chrono>

#include <gtest/gtest.h>

#include <opencv2/opencv.hpp>
#include <frame_transformers/AffineFrameTransformer.h>
#include <frame_transformers/NoOpFrameTransformer.h>

using namespace MPF::COMPONENT;
using DetectionComponentUtils::GetProperty;


std::string get_corners_string(const std::array<cv::Point, 4> &points);
std::array<cv::Point, 4> draw_detection_filled_in(cv::Mat &img, const MPFImageLocation &loc);
void draw_point(cv::Mat &img, const cv::Point2f &point, const cv::Scalar &color={0, 0, 255});


TEST(generate_test_media, create_media) {
    std::cout << "\n" << std::endl;

    cv::Mat img(cv::Size(640, 480), CV_8UC3, cv::Scalar(255, 255, 255));

//    MPFImageLocation detection(60, 80, 100, 40, -1, { { "ROTATION", "260" } });
    MPFImageLocation detection(60, 80, 100, 40, -1, { { "ROTATION", "260" } });

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



std::array<cv::Point, 4> draw_detection_filled_in(cv::Mat &img, const MPFImageLocation &loc) {
    cv::Point2d top_left(loc.x_left_upper, loc.y_left_upper);
    cv::Point2d center = top_left + cv::Point2d(loc.width, loc.height) / 2.0;
    double angle = DetectionComponentUtils::GetProperty(loc.detection_properties, "ROTATION", 0.0);


    int padding = 0;
    cv::RotatedRect rrectToDraw(center, cv::Size2d(loc.width + padding, loc.height + padding), 360 - angle);

//    cv::RotatedRect rrectToDraw({0, 86}, {50, 0}, {84, 20});


    cv::Point2f pointsToDraw2f[4];
    rrectToDraw.points(pointsToDraw2f);

    std::array<cv::Point, 4> pointsToDraw;
    for (int i = 0; i < 4; i++) {
        pointsToDraw[i] = pointsToDraw2f[i];
    }

//    cv::fillConvexPoly(img, pointsToDraw, cv::Scalar(255, 0, 0));
    cv::fillConvexPoly(img, pointsToDraw, cv::Scalar(255, 0, 0), cv::LINE_4);
//    cv::drawMarker(img, cv::Point(42, 53), {0, 0, 255});
    std::cout << "center: " << center << std::endl;
    draw_point(img, center, { 0, 0, 255 });

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

void draw_point(cv::Mat &img, const cv::Point2f &point, const cv::Scalar &color) {
    cv::rectangle(img, point - cv::Point2f(1, 1), point + cv::Point2f(1, 1), color, 2);
}



cv::Point getExpectedCenterOld(const MPFImageLocation &loc) {
    cv::Point2d top_left(loc.x_left_upper, loc.y_left_upper);
    cv::Point2d center = top_left + cv::Point2d(loc.width, loc.height) / 2.0;
    return center;
}


double sin_deg(double degrees) {
    double radians = degrees * M_PI / 180;
    return std::sin(radians);
}

double cos_deg(double degrees) {
    double radians = degrees * M_PI / 180;
    return std::cos(radians);
}



cv::Point getActualCenter(const MPFImageLocation &loc) {
    double degrees = GetProperty(loc.detection_properties, "ROTATION", 0.0);

    double sin_val = sin_deg(degrees);
    double cos_val = cos_deg(degrees);

    double center_x = loc.x_left_upper + loc.width / 2.0 * cos_val + loc.height / 2.0 * sin_val;
    double center_y = loc.y_left_upper - loc.width / 2.0 * sin_val + loc.height / 2.0 * cos_val;

    return cv::Point(cv::saturate_cast<int>(center_x), cv::saturate_cast<int>(center_y));
}



TEST(Rotation, FindCenter) {
    MPFImageLocation old_detection(565, -45, 30, 120, -1, { { "ROTATION", "270" } });

    MPFImageLocation new_detection(640, 0, old_detection.width, old_detection.height, -1,
                                   old_detection.detection_properties);

    ASSERT_EQ(getExpectedCenterOld(old_detection), getActualCenter(new_detection));
}


TEST(Rotation, FindTopLeft) {
    std::cout << "\n" << std::endl;
    cv::Vec2d frameSize(480, 640);
    cv::Point2d pointToMove(0, 0);
    double angle = 90;

//    cv::Vec2d frameSize(596, 324);
//    cv::Point2d pointToMove(0, 0);
//    double angle = 20;

    cv::Point2d center = frameSize / 2.0;
    cv::Matx23d M = cv::getRotationMatrix2D(center, angle, 1);

    cv::Vec2d newPosition = M * cv::Vec3d(pointToMove.x, pointToMove.y, 1);

    std::cout << newPosition << std::endl;
    std::cout << cv::Vec2i(newPosition) << std::endl;
}


TEST(asdf, huge_image_time) {
    std::cout << "\n" << std::endl;
    using namespace std::chrono;

//    cv::Mat image(10000, 10001, CV_8UC3, {255, 0, 0});
    cv::Mat image(4096, 2160, CV_8UC3, {255, 0, 0});
    std::cout << "size: " << image.size() << std::endl;

    AffineFrameTransformer xformer(cv::Rect(0, 0, image.cols, image.rows), 90, true,
                                   IFrameTransformer::Ptr(new NoOpFrameTransformer(image.size())));

    steady_clock::time_point t0 = steady_clock::now();

    xformer.TransformFrame(image, 0);

//    cv::transpose(image, image);
//    cv::flip(image, image, 1);

    steady_clock::time_point t1 = steady_clock::now();

    std::cout << "size: " << image.size() << std::endl;

    milliseconds duration = duration_cast<milliseconds>(t1 - t0);

    std::cout << "time: " << duration.count() << "ms" << std::endl;



}