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

#include "frame_transformers/AffineFrameTransformer.h"

#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include <opencv2/imgproc.hpp>

#include "detectionComponentUtils.h"


namespace MPF { namespace COMPONENT {

    namespace {
        cv::Mat_<double> GetAllCorners(const std::vector<std::tuple<cv::Rect, double, bool>> &regions) {
            // Matrix containing each regions 4 corners. First row is x coordinate and second row is y coordinate.
            cv::Mat_<double> corners(2, 4 * regions.size());
            auto x_iter = corners.begin();
            auto y_iter = corners.row(1).begin();

            for (const auto &region : regions) {
                const cv::Rect& rect = std::get<0>(region);
                double rotation = std::get<1>(region);

                *(x_iter++) = rect.x;
                *(y_iter++) = rect.y;

                double radians = rotation * M_PI / 180.0;
                double sinVal = std::sin(radians);
                double cosVal = std::cos(radians);

                // Need to subtract one because if you have a Rect(x=0, y=0, width=10, height=10),
                // the bottom right corner is (9, 9) not (10, 10)
                double width = rect.width - 1;
                double height = rect.height - 1;

                double corner2X = rect.x + width * cosVal;
                double corner2Y = rect.y - width * sinVal;
                *(x_iter++) = corner2X;
                *(y_iter++) = corner2Y;

                *(x_iter++) = corner2X + height * sinVal;
                *(y_iter++) = corner2Y + height * cosVal;

                *(x_iter++) = rect.x + height * sinVal;
                *(y_iter++) = rect.y + height * cosVal;
            }
            return corners;
        }


        cv::Rect2d GetMappedBoundingRect(
                const std::vector<std::tuple<cv::Rect, double, bool>> &regions,
                const cv::Matx33d &frameRotMat) {

            // Since we are working with 2d points and we aren't doing any translation here,
            // we can drop the last row and column to save some work.
            cv::Mat_<double> simpleRotation = cv::Mat_<double>(frameRotMat, false)({0, 0, 2, 2});
            cv::Mat_<double> mappedCorners = simpleRotation * GetAllCorners(regions);

            double minX, maxX, minY, maxY;
            cv::minMaxLoc(mappedCorners.row(0), &minX, &maxX);
            cv::minMaxLoc(mappedCorners.row(1), &minY, &maxY);

            double width = maxX - minX + 1;
            double height = maxY - minY + 1;

            return cv::Rect2d(cv::Point2d(minX, minY), cv::Size2d(width, height));
        }


        std::vector<std::tuple<cv::Rect, double, bool>> singleRegion(const cv::Rect &region, double rotation,
                                                                     bool flip) {
            return { std::make_tuple(region, rotation, flip) };
        }


        namespace IndividualXForms {
            // All transformation matrices are from
            // https://en.wikipedia.org/wiki/Affine_transformation#Image_transformation

            // Returns a matrix that will rotate points the given number of degrees in the counter-clockwise direction.
            cv::Matx33d Rotation(double rotationDegrees) {
                if (DetectionComponentUtils::RotationAnglesEqual(0, rotationDegrees)) {
                    // When rotation angle is 0 some matrix elements that should
                    // have been 0 were actually 1e-16 due to rounding issues.
                    return cv::Matx33d::eye();
                }

                double radians = rotationDegrees * M_PI / 180.0;
                double sinVal = std::sin(radians);
                double cosVal = std::cos(radians);
                return {
                    cosVal,  sinVal, 0,
                    -sinVal, cosVal, 0,
                    0,       0,      1
                };
            }

            cv::Matx33d HorizontalFlip() {
                return {
                    -1, 0, 0,
                     0, 1, 0,
                     0, 0, 1
                };
            }

            cv::Matx33d Translation(double xDistance, double yDistance) {
                return {
                    1, 0, xDistance,
                    0, 1, yDistance,
                    0, 0, 1
                };
            }
        } // End IndividualXForms
    } // End anonymous namespace



    AffineTransformation::AffineTransformation(const std::vector<std::tuple<cv::Rect, double, bool>> &regions,
                                               double frameRotationDegrees, bool flip)
            : rotationDegrees_(frameRotationDegrees)
            , flip_(flip)
    {
        if (regions.empty()) {
            throw std::length_error("The \"regions\" parameter must contain at least one element, but it was empty.");
        }

        cv::Matx33d rotationMat = IndividualXForms::Rotation(360 - frameRotationDegrees);
        cv::Rect mappedBoundingRect = GetMappedBoundingRect(regions, rotationMat);
        regionSize_ = mappedBoundingRect.size();

        cv::Matx33d moveRoiToOrigin = IndividualXForms::Translation(-mappedBoundingRect.x, -mappedBoundingRect.y);

        cv::Matx33d combinedTransform;
        if (flip) {
            /*
                        -x     x=0     +x
              initial: [     ] | [ a b ]
              flipped: [ b a ] | [     ]
              shift:   [     ] | [ b a ]
            */
            cv::Matx33d flipMat = IndividualXForms::HorizontalFlip();
            cv::Matx33d flipShiftCorrection = IndividualXForms::Translation(regionSize_.width, 0);
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

        cv::invertAffineTransform(combined2dTransform, reverseTransformationMatrix_);
    }



    void AffineTransformation::Apply(cv::Mat &frame) const {
        // From cv::warpAffine docs:
        // The function warpAffine transforms the source image using the specified matrix when the flag
        // WARP_INVERSE_MAP is set. Otherwise, the transformation is first inverted with cv::invertAffineTransform.
        // From OpenCV's Geometric Image Transformations module documentation:
        // To avoid sampling artifacts, the mapping is done in the reverse order, from destination to the source.

        // Using INTER_CUBIC, because according to
        // https://en.wikipedia.org/wiki/Affine_transformation#Image_transformation
        // "This transform relocates pixels requiring intensity interpolation to approximate the value of moved pixels,
        // bicubic interpolation is the standard for image transformations in image processing applications."
        cv::warpAffine(frame, frame, reverseTransformationMatrix_, regionSize_,
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

        if (!DetectionComponentUtils::RotationAnglesEqual(rotationDegrees_, 0)) {
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


    cv::Size2d AffineTransformation::GetRegionSize() const {
        return regionSize_;
    }




    AffineFrameTransformer::AffineFrameTransformer(const cv::Rect &region,
                                                   double rotation,
                                                   bool flip,
                                                   IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform))
            , transform_(
                // Pass in rotation here since the bounding box itself is rotated.
                singleRegion(region, rotation, flip),
                // Pass in rotation here so that the frame will rotated so that the bounding box ends up being upright.
                rotation, flip)
    {
    }

    AffineFrameTransformer::AffineFrameTransformer(double rotation,
                                                   bool flip,
                                                   IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform))
            , transform_(singleRegion(cv::Rect(cv::Point(0, 0), GetInnerFrameSize(0)), 0, false), rotation, flip)
    {
    }


    AffineFrameTransformer::AffineFrameTransformer(const std::vector<std::tuple<cv::Rect, double, bool>> &regions,
                                                   double frameRotation, bool frameFlip,
                                                   IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform))
            , transform_(regions, frameRotation, frameFlip)
    {
    }


    cv::Size AffineFrameTransformer::GetFrameSize(int frameIndex) {
        return transform_.GetRegionSize();
    }


    void AffineFrameTransformer::DoFrameTransform(cv::Mat &frame, int frameIndex) {
        transform_.Apply(frame);
    }


    void AffineFrameTransformer::DoReverseTransform(MPFImageLocation &imageLocation, int frameIndex) {
        transform_.ApplyReverse(imageLocation);
    }




    FeedForwardExactRegionAffineTransformer::FeedForwardExactRegionAffineTransformer(
                const std::vector<std::tuple<cv::Rect, double, bool>> &regions,
                IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform))
            , frameTransforms_(GetTransformations(regions))
    {
    }


    cv::Size FeedForwardExactRegionAffineTransformer::GetFrameSize(int frameIndex) {
        return GetTransform(frameIndex).GetRegionSize();
    }


    std::vector<AffineTransformation> FeedForwardExactRegionAffineTransformer::GetTransformations(
            const std::vector<std::tuple<cv::Rect, double, bool>> &regions) {

        std::vector<AffineTransformation> transforms;
        transforms.reserve(regions.size());

        for (const auto& region : regions) {
            transforms.emplace_back(std::vector<std::tuple<cv::Rect, double, bool>>{ region },
                                    std::get<1>(region), std::get<2>(region));
        }
        return transforms;
    }


    const AffineTransformation &FeedForwardExactRegionAffineTransformer::GetTransform(int frameIndex) const {
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

    void FeedForwardExactRegionAffineTransformer::DoFrameTransform(cv::Mat &frame, int frameIndex) {
        GetTransform(frameIndex).Apply(frame);
    }

    void FeedForwardExactRegionAffineTransformer::DoReverseTransform(MPFImageLocation &imageLocation, int frameIndex) {
        GetTransform(frameIndex).ApplyReverse(imageLocation);
    }
}}