/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2020 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2020 The MITRE Corporation                                       *
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

#include <map>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include "Utils.h"

#include "ImageGeneration.h"


using cv::Mat;
using cv::Point;
using cv::Rect;
using cv::Scalar;
using cv::Size;

using std::string;
using std::vector;


namespace MPF { namespace COMPONENT {

    ImageGeneration::ImageGeneration(bool imshow_on)
            : imshow_on(imshow_on) {

    }


    Mat ImageGeneration::Rotate(const Mat &src) {
        Mat rot_mat(2, 3, CV_32FC1);
        Mat warp_rotate_dst;

        Point center = Point(src.cols / 2, src.rows / 2);

        double angle = rand() % 11 - 5;;

        double scale = static_cast<double>(rand() % 10) / 100.0 + 0.9;;

        rot_mat = cv::getRotationMatrix2D(center, angle, scale);

        cv::warpAffine(src, warp_rotate_dst, rot_mat, src.size());

        if (imshow_on) {
            cv::imshow("warp_rotate_window", warp_rotate_dst);
            cv::waitKey(5);
        }

        return warp_rotate_dst;
    }


    Rect ImageGeneration::GetRandomRect(const Mat &image, const Rect &rect, const vector<Rect> &existing_rects) {
        Mat mask;
        mask = Mat::zeros(image.size(), image.type());

        int x_pos = rand() % (image.cols - rect.width);
        int y_pos = rand() % (image.rows - rect.height);

        Rect new_rect(x_pos, y_pos, rect.width, rect.height);

        cv::rectangle(mask, new_rect, Scalar(255, 255, 255), cv::FILLED);

        if (imshow_on) {
            cv::imshow("random mask", mask);
            cv::waitKey(5);
        }

        return new_rect;
    }


    int ImageGeneration::WriteDetectionOutputImage(const string image_in_uri,
                                                   const vector<MPFVideoTrack> &detections,
                                                   const string image_out_filepath) {
        Mat input_image = cv::imread(image_in_uri.c_str(), cv::IMREAD_IGNORE_ORIENTATION + cv::IMREAD_COLOR);
        if (input_image.empty()) {
            printf("Could not open the input image: %s\n", image_in_uri.c_str());
            return -1;
        }

        for (unsigned int i = 0; i < detections.size(); i++) {
            for (std::map<int, MPFImageLocation>::const_iterator it = detections[i].frame_locations.begin();
                 it != detections[i].frame_locations.end(); it++) {
                Rect rect(it->second.x_left_upper, it->second.y_left_upper, it->second.width, it->second.height);
                cv::rectangle(input_image, rect, Scalar(0, 255, 0));
            }
        }

        cv::imwrite(image_out_filepath, input_image);

        return 0;
    }

    int ImageGeneration::WriteDetectionOutputImage(const string image_in_uri,
                                                   const vector<MPFImageLocation> &detections,
                                                   const string image_out_filepath) {
        Mat input_image = cv::imread(image_in_uri.c_str(), cv::IMREAD_IGNORE_ORIENTATION + cv::IMREAD_COLOR);
        if (input_image.empty()) {
            printf("Could not open the input image: %s\n", image_in_uri.c_str());
            return -1;
        }

        for (unsigned int i = 0; i < detections.size(); i++) {
            Rect rect(detections[i].x_left_upper,
                           detections[i].y_left_upper,
                           detections[i].width,
                           detections[i].height);
            cv::rectangle(input_image, rect, Scalar(0, 255, 0));
        }

        cv::imwrite(image_out_filepath, input_image);

        return 0;
    }

    int ImageGeneration::CreateTestImageAndDetectionOutput(const vector<Mat> &objects,
                                                           bool use_scaling_and_rotation,
                                                           const string image_out_filepath,
                                                           vector<MPFImageLocation> &detections) {
        if (objects.empty()) {
            return -1;
        }

        Size image_size = Size(2000, 2000);
        Mat blank_frame = Mat(image_size, CV_8UC3, Scalar(127, 127, 127));
        Mat src = blank_frame.clone();

        vector<Rect> random_rects;

        for (unsigned int i = 0; i < objects.size(); i++) {
            Mat object = Mat(objects[i]);
            Rect rect = Rect(0, 0, object.cols, object.rows);

            Rect random_rect;

            int intersection_index = -1;
            do {
                random_rect = GetRandomRect(blank_frame, rect);
            } while (Utils::IsExistingRectIntersection(random_rect, random_rects, intersection_index));

            random_rects.push_back(random_rect);
            MPFImageLocation detection(random_rect.x, random_rect.y, random_rect.width, random_rect.height);
            detections.push_back(detection);

            Mat subview = src(random_rects[i]);

            if (use_scaling_and_rotation) {
                object = Rotate(object);
            }
            object.copyTo(subview);
        }
        if (imshow_on) {
            cv::imshow("src", src);
            cv::waitKey(5);
        }

        cv::imwrite(image_out_filepath, src);

        return 0;
    }

}}