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


    AffineTransformation::AffineTransformation(const cv::Rect &region, double rotationDegrees, bool flip)
            : regionSize(region.size())
            , rotationDegrees_(rotationDegrees)
            , flip_(flip)
            , reverseTransformationMatrix_(GetReverseTransformationMatrix(
                    regionSize, GetRotatedRegionCenter(region, rotationDegrees), rotationDegrees, flip))
    {
    }


    AffineTransformation::AffineTransformation(const cv::Size &frameSize, double rotationDegrees, bool flip)
            : regionSize(GetRotatedFrameSize(frameSize, rotationDegrees))
            , rotationDegrees_(rotationDegrees)
            , flip_(flip)
            , reverseTransformationMatrix_(GetReverseTransformationMatrix(
                    regionSize, cv::Point2d(frameSize) / 2.0, rotationDegrees, flip))
    {
    }



    void AffineTransformation::Apply(cv::Mat &frame) const {
        // From cv::warpAffine docs:
        // The function warpAffine transforms the source image using the specified matrix when the flag
        // WARP_INVERSE_MAP is set. Otherwise, the transformation is first inverted with cv::invertAffineTransform
        // From OpenCV's Geometric Image Transformations module documentation:
        // To avoid sampling artifacts, the mapping is done in the reverse order, from destination to the source.
        cv::warpAffine(frame, frame, reverseTransformationMatrix_, regionSize,
                       cv::InterpolationFlags::WARP_INVERSE_MAP | cv::InterpolationFlags::INTER_CUBIC);

    }



    void AffineTransformation::ApplyReverse(MPFImageLocation &imageLocation) const {
        cv::Vec3d topLeft(imageLocation.x_left_upper, imageLocation.y_left_upper, 1);
        if (flip_) {
            topLeft[0] += imageLocation.width;
        }

        cv::Vec2d newTopLeft = reverseTransformationMatrix_ * topLeft;

        imageLocation.x_left_upper = cv::saturate_cast<int>(newTopLeft[0]);
        imageLocation.y_left_upper = cv::saturate_cast<int>(newTopLeft[1]);

        if (!DetectionComponentUtils::RotationAngleEquals(rotationDegrees_, 0)) {
            double existingRotation
                    = DetectionComponentUtils::GetProperty(imageLocation.detection_properties, "ROTATION", 0.0);
            double newRotation = DetectionComponentUtils::NormalizeAngle(existingRotation + rotationDegrees_);
            imageLocation.detection_properties["ROTATION"] = std::to_string(newRotation);
        }

        if (flip_) {
            bool existingFlip = DetectionComponentUtils::GetProperty(
                    imageLocation.detection_properties, "HORIZONTAL_FLIP", false);
            if (existingFlip) {
                imageLocation.detection_properties.erase("HORIZONTAL_FLIP");
            }
            else {
                imageLocation.detection_properties.emplace("HORIZONTAL_FLIP", "true");
            }
        }
    }



    cv::Matx23d AffineTransformation::GetReverseTransformationMatrix(
            const cv::Size2d &regionSize, const cv::Point2d &center, double rotationDegrees, bool flip) {

        // rotationDegrees indicates the region's rotation in the counter-clockwise direction,
        // so in order correctly orient the image we need to rotate in the opposite direction;
        cv::Matx33d rotationMat = GetRotationMatrix(center, 360 - rotationDegrees);

        cv::Matx33d moveRoiToOrigin = GetTranslationMatrix(regionSize.width / 2.0 - center.x,
                                                           regionSize.height / 2.0 - center.y);

        cv::Matx33d combinedTransform;
        if (flip) {
            /*
                        -x     x=0     +x
               initial: [     ] | [ a b ]
               flipped: [ b a ] | [     ]
               shift:   [     ] | [ b a ]
             */
            cv::Matx33d flipMat = GetHorizontalFlipMatrix();
            cv::Matx33d flipShiftCorrection = GetTranslationMatrix(regionSize.width, 0);
            // Transformations are applied from right to left, so rotation occurs first.
            combinedTransform = flipShiftCorrection * flipMat * moveRoiToOrigin * rotationMat;
        }
        else {
            // Transformations are applied from right to left, so rotation occurs first.
            combinedTransform = moveRoiToOrigin * rotationMat;
        }


        // When combining transformations the 3d version must be used,
        // but when mapping 2d points the last row of the matrix can be dropped.
        cv::Matx23d combined2dTransform = combinedTransform.get_minor<2, 3>(0, 0);
        cv::Matx23d reverseTransform;
        cv::invertAffineTransform(combined2dTransform, reverseTransform);
        return reverseTransform;
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

    cv::Matx33d AffineTransformation::GetTranslationMatrix(double xDistance, double yDistance) {
        return { 1, 0, xDistance,
                 0, 1, yDistance,
                 0, 0, 1 };
    }


    cv::Point2d AffineTransformation::GetRotatedRegionCenter(const cv::Rect &region, double rotationDegrees) {
        double radians = rotationDegrees * M_PI / 180.0;
        double sinVal = std::sin(radians);
        double cosVal = std::cos(radians);

        double centerX = region.x + region.width / 2.0 * cosVal + region.height / 2.0 * sinVal;
        double centerY = region.y - region.width / 2.0 * sinVal + region.height / 2.0 * cosVal;
        return cv::Point2d(centerX, centerY);
    }


    cv::Size2d AffineTransformation::GetRotatedFrameSize(const cv::Size &frameSize, double rotationDegrees) {
        double correctionDegrees = 360 - rotationDegrees;
        double correctionRadians = correctionDegrees * M_PI / 180.0;
        double sinVal = std::abs(std::sin(correctionRadians));
        double cosVal = std::abs(std::cos(correctionRadians));

        double newWidth = frameSize.height * sinVal + frameSize.width * cosVal;
        double newHeight = frameSize.height * cosVal + frameSize.width * sinVal;
        return cv::Size2d(newWidth, newHeight);
    }



    AffineFrameTransformer::AffineFrameTransformer(const cv::Rect &region,
                                                   double rotation,
                                                   bool flip,
                                                   IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform))
            , transform_(region, rotation, flip)
    {
    }

    AffineFrameTransformer::AffineFrameTransformer(double rotation,
                                                   bool flip,
                                                   IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform))
            , transform_(GetInnerFrameSize(0), rotation, flip)
    {
    }



    cv::Size AffineFrameTransformer::GetFrameSize(int frameIndex) {
        return transform_.regionSize;
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
        return GetTransform(frameIndex).regionSize;
    }


    std::vector<AffineTransformation>
    FeedForwardAffineTransformer::GetTransformations(
            const std::vector<std::tuple<cv::Rect, double, bool>> &transformInfos) {

        std::vector<AffineTransformation> transforms;
        transforms.reserve(transformInfos.size());

        int count = -1;
        for (const auto& transformInfo : transformInfos) {
            count++;
            transforms.emplace_back(std::get<0>(transformInfo), std::get<1>(transformInfo), std::get<2>(transformInfo));
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