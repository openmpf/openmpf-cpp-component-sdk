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

#include "frame_transformers/AffineFrameTransformer.h"

#include <utility>
#include <cmath>

#include <opencv2/imgproc.hpp>

#include "detectionComponentUtils.h"


namespace MPF { namespace COMPONENT {


    AffineTransformation::AffineTransformation(const cv::Rect &region, double rotationDegrees, bool flip,
                                               const cv::Size &frameSize)
            : rotationDegrees_(rotationDegrees)
            , sizeAfterRotation(GetSizeAfterRotation(rotationDegrees, region, frameSize))
            , transformationMatrix_(GetTransformationMatrix(region, rotationDegrees, flip, sizeAfterRotation))
            , reverseTransformationMatrix_(GetReverseTransformationMatrix(transformationMatrix_))
    {
    }


    void AffineTransformation::Apply(cv::Mat &frame) const {
        cv::warpAffine(frame, frame, transformationMatrix_, sizeAfterRotation);
    }



    void AffineTransformation::ApplyReverse(MPFImageLocation &imageLocation) const {

        cv::Vec3d topLeft(imageLocation.x_left_upper, imageLocation.y_left_upper, 1);
        cv::Vec2d newTopLeft = reverseTransformationMatrix_ * topLeft;

        imageLocation.x_left_upper = cv::saturate_cast<int>(newTopLeft[0]);
        imageLocation.y_left_upper = cv::saturate_cast<int>(newTopLeft[1]);

        if (!DetectionComponentUtils::RotationAngleEquals(rotationDegrees_, 0)) {
            double existingRotation
                    = DetectionComponentUtils::GetProperty(imageLocation.detection_properties, "ROTATION", 0.0);
            double newRotation = DetectionComponentUtils::NormalizeAngle(existingRotation + rotationDegrees_);
            imageLocation.detection_properties["ROTATION"] = std::to_string(newRotation);
        }
    }


//    cv::Size2d AffineTransformation::GetSizeAfterRotation(double rotationDegrees, const cv::Rect &region,
//                                                          const cv::Size &frameSize) {
//        cv::Point2d center = GetCenter(region, rotationDegrees);
//        // rotationDegrees is the rotation in the counter-clockwise direction, but cv::RotatedRect rotates in the
//        // clockwise direction.
//        cv::Rect2d regionBoundingRect
//                = cv::RotatedRect(center, region.size(), 360 - rotationDegrees).boundingRect2f();
//
//        bool shouldExpand
//                = regionBoundingRect.br().x > frameSize.width || regionBoundingRect.br().y > frameSize.height;
//
//        return shouldExpand
//               ? regionBoundingRect.size()
//               : cv::Size2d(region.size());
//    }


//    cv::Size2d AffineTransformation::GetSizeAfterRotation(double rotationDegrees, const cv::Rect &region,
//                                                          const cv::Size &frameSize) {
//        double radians = rotationDegrees * M_PI / 180.0;
//        double sin_val = std::sin(radians);
//        double cos_val = std::cos(radians);
//
//        double newWidth = region.width * cos_val + region.height * sin_val;
//        double newHeight = region.width * sin_val + region.height * cos_val;
//
//        bool shouldExpand = region.x + newWidth > frameSize.width || region.y + newHeight > frameSize.height;
//        return shouldExpand
//            ? cv::Size2d(newWidth, newHeight)
//            : cv::Size2d(region.size());
//    }

    cv::Size2d AffineTransformation::GetSizeAfterRotation(double rotationDegrees, const cv::Rect &region,
                                                          const cv::Size &frameSize) {
        // We will be rotating frame in opposite direction of orientation.
        double correctedRotation = 360 - rotationDegrees;
        double radians = correctedRotation * M_PI / 180.0;
        double sin_val = std::abs(std::sin(radians));
        double cos_val = std::abs(std::cos(radians));

        double newWidth = std::abs(region.width * cos_val + region.height * sin_val);
        double newHeight = std::abs(region.width * sin_val + region.height * cos_val);

        cv::Matx23d rotationMat = cv::getRotationMatrix2D(GetCenter(region, rotationDegrees), correctedRotation, 1);

        cv::Vec2d result_pt = rotationMat * cv::Vec3d(region.x, region.y, 1);
        cv::Vec2d result_sz = rotationMat * cv::Vec3d(region.width, region.height, 1);

        return region.size();

//        bool shouldExpand = region.x + newWidth > frameSize.width || region.y + newHeight > frameSize.height;
//        return shouldExpand
//               ? cv::Size2d(newWidth, newHeight)
//               : cv::Size2d(region.size());
    }




    cv::Point2d AffineTransformation::GetCenter(const cv::Rect &region, double rotationDegrees) {
        double radians = rotationDegrees * M_PI / 180.0;
        double sin_val = std::sin(radians);
        double cos_val = std::cos(radians);

        double center_x = region.x + region.width / 2.0 * cos_val + region.height / 2.0 * sin_val;
        double center_y = region.y - region.width / 2.0 * sin_val + region.height / 2.0 * cos_val;
        return cv::Point2d(center_x, center_y);
    }


    cv::Matx23d AffineTransformation::GetTransformationMatrix2(const cv::Rect &region, double rotationDegrees, bool flip,
                                                              const cv::Size2d &sizeAfterRotation) {

        cv::Point2d center = GetCenter(region, rotationDegrees);

//        cv::Matx23d rotationMat = cv::getRotationMatrix2D(center, 360 - rotationDegrees, 1);
        cv::Matx33d rotationMat = GetRotationMatrix(center, 360 - rotationDegrees);

//        cv::Vec2d rotationCorrectionDist(-region.width / 2.0, 0);
//        cv::Vec2d rotationCorrectionDist(-80, 0);
        cv::Vec2d rotationCorrectionDist(-82, 80);
        cv::Matx33d rotationShiftCorrection = GetTranslationMatrix(rotationCorrectionDist);

        return (rotationShiftCorrection * rotationMat).get_minor<2, 3>(0, 0);
    }

    cv::Matx23d AffineTransformation::GetTransformationMatrix3(const cv::Rect &region, double rotationDegrees, bool flip,
                                                               const cv::Size2d &sizeAfterRotation) {

//        cv::Vec2d center = cv::Point2d(region.tl()) + cv::Point2d(region.size()) / 2.0;
        cv::Vec2d center = GetCenter(region, rotationDegrees);

        // rotationDegrees indicates the region's rotation in the counter-clockwise direction,
        // so in order correctly orient the image we need to rotate in the opposite direction;
        cv::Matx33d rotationMat = GetRotationMatrix(center, 360 - rotationDegrees);

        // When rotated region fits in frame (region.size() == sizeAfterRotation),
        // this moves the region of interest up and to the left until it is in the top left corner of frame.
        // When the rotated region does not fit in the frame (region.size() < sizeAfterRotation), this moves the
        // region of interest down and to right until the entire region fits in the frame.
        cv::Vec2d rotationCorrectionDist = cv::Vec2d(sizeAfterRotation.width, sizeAfterRotation.height) / 2.0 - center;
        cv::Matx33d rotationShiftCorrection = GetTranslationMatrix(rotationCorrectionDist);

        if (!flip) {
            // Transformations are applied from right to left, so rotation occurs first.
            return (rotationShiftCorrection * rotationMat).get_minor<2, 3>(0, 0);
        }

        /*
                    -x     x=0     +x
           initial: [     ] | [ a b ]
           flipped: [ b a ] | [     ]
           shift:   [     ] | [ b a ]
         */
        cv::Matx33d flipMat = GetHorizontalFlipMatrix();
        cv::Matx33d flipShiftCorrection = GetTranslationMatrix({ sizeAfterRotation.width, 0 });

        // Transformations are applied from right to left, so rotation occurs first.
        return (flipShiftCorrection * flipMat * rotationShiftCorrection * rotationMat).get_minor<2, 3>(0, 0);
    }


    cv::Matx23d AffineTransformation::GetTransformationMatrix(const cv::Rect &region, double rotationDegrees, bool flip,
                                                              const cv::Size2d &sizeAfterRotation) {

//        cv::Vec2d center = cv::Point2d(region.tl()) + cv::Point2d(region.size()) / 2.0;
        cv::Vec2d center = GetCenter(region, rotationDegrees);

        // rotationDegrees indicates the region's rotation in the counter-clockwise direction,
        // so in order correctly orient the image we need to rotate in the opposite direction;
        cv::Matx33d rotationMat = GetRotationMatrix(center, 360 - rotationDegrees);

        // When rotated region fits in frame (region.size() == sizeAfterRotation),
        // this moves the region of interest up and to the left until it is in the top left corner of frame.
        // When the rotated region does not fit in the frame (region.size() < sizeAfterRotation), this moves the
        // region of interest down and to right until the entire region fits in the frame.
        cv::Vec2d rotationCorrectionDist = cv::Vec2d(sizeAfterRotation.width, sizeAfterRotation.height) / 2.0 - center;

        cv::Vec3d newTopLeft = rotationMat * cv::Vec3d(region.x, region.y, 1);

        cv::Matx33d rotationShiftCorrection = GetTranslationMatrix(-1 * cv::Vec2d(newTopLeft[0], newTopLeft[1]));

        if (!flip) {
            // Transformations are applied from right to left, so rotation occurs first.
            return (rotationShiftCorrection * rotationMat).get_minor<2, 3>(0, 0);
        }

        /*
                    -x     x=0     +x
           initial: [     ] | [ a b ]
           flipped: [ b a ] | [     ]
           shift:   [     ] | [ b a ]
         */
        cv::Matx33d flipMat = GetHorizontalFlipMatrix();
        cv::Matx33d flipShiftCorrection = GetTranslationMatrix({ sizeAfterRotation.width, 0 });

        // Transformations are applied from right to left, so rotation occurs first.
        return (flipShiftCorrection * flipMat * rotationShiftCorrection * rotationMat).get_minor<2, 3>(0, 0);
    }




    cv::Matx23d AffineTransformation::GetReverseTransformationMatrix(const cv::Matx23d &transformationMatrix) {
        cv::Matx23d reverseTransformationMatrix;
        cv::invertAffineTransform(transformationMatrix, reverseTransformationMatrix);
        return reverseTransformationMatrix;
    }



    cv::Matx33d AffineTransformation::GetRotationMatrix(const cv::Point2d &center, double rotationDegrees) {
        // The rotation matrix from is a 2x3 matrix, but to combine transformations it must be converted to a
        // 3x3 matrix by adding [0, 0, 1] as the bottom row.
        // cv::getRotationMatrix2D rotates in the counter-clockwise direction.
        cv::Mat ocvRotationMat = cv::getRotationMatrix2D(center, rotationDegrees, 1);
        cv::Matx33d rotationMatrix;
        cv::vconcat(ocvRotationMat,
                    cv::Matx13d(0, 0, 1), rotationMatrix);
        return rotationMatrix;
    }


    cv::Matx33d AffineTransformation::GetHorizontalFlipMatrix() {
        return { -1, 0, 0,
                  0, 1, 0,
                  0, 0, 1 };
    }

    cv::Matx33d AffineTransformation::GetTranslationMatrix(const cv::Vec2d &distance) {
        return { 1, 0, distance[0],
                 0, 1, distance[1],
                 0, 0, 1 };
    }






    AffineFrameTransformer::AffineFrameTransformer(const cv::Rect &region,
                                                   double rotation,
                                                   bool flip,
                                                   IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform))
            , transform_(region, rotation, flip, GetInnerFrameSize(0))
    {

    }

    cv::Size AffineFrameTransformer::GetFrameSize(int frameIndex) {
        return transform_.sizeAfterRotation;
    }


    void AffineFrameTransformer::DoFrameTransform(cv::Mat &frame, int frameIndex) {
        transform_.Apply(frame);
    }


    void AffineFrameTransformer::DoReverseTransform(MPFImageLocation &imageLocation, int frameIndex) {
        transform_.ApplyReverse(imageLocation);
    }




    FeedForwardAffineTransformer::FeedForwardAffineTransformer(
                const std::vector<std::tuple<cv::Rect, double, bool>> &transformInfo,
                IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform))
            , frameTransforms_(GetTransformations(transformInfo))
    {
    }


    cv::Size FeedForwardAffineTransformer::GetFrameSize(int frameIndex) {
        return GetTransform(frameIndex).sizeAfterRotation;
    }


    std::vector<AffineTransformation>
    FeedForwardAffineTransformer::GetTransformations(
            const std::vector<std::tuple<cv::Rect, double, bool>> &transformInfos) {

        std::vector<AffineTransformation> transforms;
        transforms.reserve(transformInfos.size());

        int count = -1;
        for (const auto& transformInfo : transformInfos) {
            count++;
            transforms.emplace_back(std::get<0>(transformInfo), std::get<1>(transformInfo), std::get<2>(transformInfo),
                                    GetInnerFrameSize(count));
        }
        return transforms;
    }

    const AffineTransformation &FeedForwardAffineTransformer::GetTransform(int frameIndex) {
        try {
            return frameTransforms_.at(frameIndex);
        }
        catch (const std::out_of_range &error) {
            std::stringstream ss;
            ss << "Attempted to get transformation for frame: " << frameIndex
               << ", but there are only " << frameTransforms_.size() << " entries in the feed forward track.";
            throw std::out_of_range(ss.str());
        }
    }

    void FeedForwardAffineTransformer::DoFrameTransform(cv::Mat &frame, int frameIndex) {
        GetTransform(frameIndex).Apply(frame);
    }

    void FeedForwardAffineTransformer::DoReverseTransform(MPFImageLocation &imageLocation, int frameIndex) {
        GetTransform(frameIndex).ApplyReverse(imageLocation);
    }

}}