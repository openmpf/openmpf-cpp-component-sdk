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

//    cv::Point AffineTransformation::Apply(const cv::Point &point) const {
//        cv::Vec3d pointVec(point.x, point.y, 1);
//        cv::Point2d result = transformationMatrix_ * pointVec;
//        return result;
//    }


//    void AffineTransformation::ApplyReverse(MPFImageLocation &imageLocation, const cv::Size &frameSize) const {
//        cv::Vec2d distToCenter(imageLocation.width / 2.0, imageLocation.height / 2.0);
//        cv::Vec2d originalCenter
//                = cv::Vec2d(imageLocation.x_left_upper, imageLocation.y_left_upper) + distToCenter;
//
//        cv::Vec2d transformedCenter = reverseTransformationMatrix_ * cv::Vec3d(originalCenter[0], originalCenter[1], 1);
//        cv::Vec2d newTopLeft = transformedCenter - distToCenter;
//
//        if (DetectionComponentUtils::RotationAngleEquals(rotationDegrees_, 90)) {
//            int fixed_x = imageLocation.y_left_upper;
//            int fixed_y = frameSize.height - (imageLocation.x_left_upper + imageLocation.width);
//            imageLocation.x_left_upper = fixed_x;
//            imageLocation.y_left_upper = fixed_y;
//            std::swap(imageLocation.width, imageLocation.height);
//        }
//        else if (DetectionComponentUtils::RotationAngleEquals(rotationDegrees_, 270)) {
//            int fixed_x = frameSize.width - (imageLocation.y_left_upper + imageLocation.height);
//            int fixed_y = imageLocation.x_left_upper;
//            imageLocation.x_left_upper = fixed_x;
//            imageLocation.y_left_upper = fixed_y;
//            std::swap(imageLocation.width, imageLocation.height);
//        }
//        else {
//            imageLocation.x_left_upper = cv::saturate_cast<int>(newTopLeft[0]);
//            imageLocation.y_left_upper = cv::saturate_cast<int>(newTopLeft[1]);
//            if (!DetectionComponentUtils::RotationAngleEquals(rotationDegrees_, 0)) {
//                double existingRotation
//                        = DetectionComponentUtils::GetProperty(imageLocation.detection_properties, "ROTATION", 0.0);
//                double newRotation = DetectionComponentUtils::NormalizeAngle(existingRotation + rotationDegrees_);
//                imageLocation.detection_properties["ROTATION"] = std::to_string(newRotation);
//            }
//        }
//    }



    void AffineTransformation::ApplyReverse(MPFImageLocation &imageLocation) const {
        cv::Vec2d distToCenter(imageLocation.width / 2.0, imageLocation.height / 2.0);
        cv::Vec2d originalCenter
                = cv::Vec2d(imageLocation.x_left_upper, imageLocation.y_left_upper) + distToCenter;

        cv::Vec2d transformedCenter = reverseTransformationMatrix_ * cv::Vec3d(originalCenter[0], originalCenter[1], 1);
        cv::Vec2d newTopLeft = transformedCenter - distToCenter;

        imageLocation.x_left_upper = cv::saturate_cast<int>(newTopLeft[0]);
        imageLocation.y_left_upper = cv::saturate_cast<int>(newTopLeft[1]);

        if (!DetectionComponentUtils::RotationAngleEquals(rotationDegrees_, 0)) {
            double existingRotation
                    = DetectionComponentUtils::GetProperty(imageLocation.detection_properties, "ROTATION", 0.0);
            double newRotation = DetectionComponentUtils::NormalizeAngle(existingRotation + rotationDegrees_);
            imageLocation.detection_properties["ROTATION"] = std::to_string(newRotation);
        }
    }


    cv::Size2d AffineTransformation::GetSizeAfterRotation(double rotationDegrees, const cv::Rect &region,
                                                          const cv::Size &frameSize) {
        cv::Point2d center = cv::Point2d(region.tl()) + cv::Point2d(region.size()) / 2.0;
        cv::Rect2d regionBoundingRect
                = cv::RotatedRect(center, region.size(), 360 - rotationDegrees).boundingRect2f();

        bool shouldExpand
                = regionBoundingRect.br().x > frameSize.width || regionBoundingRect.br().y > frameSize.height;

        return shouldExpand
               ? regionBoundingRect.size()
               : cv::Size2d(region.size());
    }


    cv::Matx23d AffineTransformation::GetTransformationMatrix(const cv::Rect &region, double rotationDegrees, bool flip,
                                                              const cv::Size2d &sizeAfterRotation) {

        cv::Point2d center = cv::Point2d(region.tl()) + cv::Point2d(region.size()) / 2.0;
        cv::Matx23d rotationMatrix = cv::getRotationMatrix2D(center, 360 - rotationDegrees, 1);

        // Adjust the rotation matrix to take into account translation.
        // Push region to top left corner.
        rotationMatrix(0, 2) += sizeAfterRotation.width / 2.0 - center.x;
        rotationMatrix(1, 2) += sizeAfterRotation.height / 2.0 - center.y;

        if (!flip) {
            return rotationMatrix;
        }

        cv::Matx33d augmentedRotationMatrix;
        cv::vconcat(rotationMatrix, cv::Matx13d(0, 0, 1), augmentedRotationMatrix);

        /*
                    -x     x=0     +x
           initial: [     ] | [ a b ]
           flipped: [ b a ] | [     ]
           shift:   [     ] | [ b a ]
         */
        cv::Matx33d augmentedFlipMatrix(-1, 0, sizeAfterRotation.width,
                                        0, 1, 0,
                                        0, 0, 1);

        cv::Matx33d augmentedTransformationMatrix = augmentedFlipMatrix * augmentedRotationMatrix;

        return augmentedTransformationMatrix.get_minor<2, 3>(0, 0);
    }


//    cv::Matx23d AffineTransformation::GetTransformationMatrix(const cv::Rect &region, double rotationDegrees, bool flip,
//                                                              const cv::Size2d &sizeAfterRotation) {
//
//        cv::Point2d center = cv::Point2d(region.tl()) + cv::Point2d(region.size()) / 2.0;
//
//        cv::Matx33d rotationMatrix;
//        cv::vconcat(cv::getRotationMatrix2D(center, 360 - rotationDegrees, 1),
//                    cv::Matx13d(0, 0, 1), rotationMatrix);
//
//        cv::Matx33d rotationCorrectionShift(1, 0, sizeAfterRotation.width / 2.0 - center.x,
//                                            0, 1, sizeAfterRotation.height / 2.0 - center.y,
//                                            0, 0, 1);
//
//
//
//        if (!flip) {
//            return (rotationCorrectionShift * rotationMatrix).get_minor<2, 3>(0, 0);
//        }
//
//
//        cv::Matx33d flipMatrix(-1, 0, 0,
//                               0, 1, 0,
//                               0, 0, 1);
//        cv::Matx33d flipCorrectionShift(1, 0, sizeAfterRotation.width,
//                                        0, 1, 0,
//                                        0, 0, 1);
//
//        cv::Matx33d transformationMatrix
//                = flipCorrectionShift * flipMatrix * rotationCorrectionShift * rotationMatrix;
//
//        return transformationMatrix.get_minor<2, 3>(0, 0);
//    }




    cv::Matx23d AffineTransformation::GetReverseTransformationMatrix(const cv::Matx23d &transformationMatrix) {
        cv::Matx23d reverseTransformationMatrix;
        cv::invertAffineTransform(transformationMatrix, reverseTransformationMatrix);
        return reverseTransformationMatrix;
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