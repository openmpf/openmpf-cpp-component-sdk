/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2021 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2021 The MITRE Corporation                                       *
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
#include "MPFRotatedRect.h"


namespace MPF { namespace COMPONENT {

    namespace {
        cv::Mat_<double> GetAllCorners(const std::vector<MPFRotatedRect> &regions) {
            // Matrix containing each region's 4 corners. First row is x coordinate and second row is y coordinate.
            cv::Mat_<double> corners(2, 4 * regions.size());
            int cornerIdx = 0;
            for (const auto& region : regions) {
                for (const auto& corner : region.GetCorners()) {
                    corners(0, cornerIdx) = corner.x;
                    corners(1, cornerIdx) = corner.y;
                    cornerIdx++;
                }
            }
            return corners;
        }

        cv::Rect2d GetMappedBoundingRect(
                const std::vector<MPFRotatedRect> &regions,
                const cv::Matx33d &frameRotMat) {

            // Since we are working with 2d points and we aren't doing any translation here,
            // we can drop the last row and column to save some work.
            cv::Mat_<double> simpleRotation = cv::Mat_<double>(frameRotMat, false)({0, 0, 2, 2});
            cv::Mat_<double> mappedCorners = simpleRotation * GetAllCorners(regions);

            double minX, maxX, minY, maxY;
            cv::minMaxLoc(mappedCorners.row(0), &minX, &maxX);
            cv::minMaxLoc(mappedCorners.row(1), &minY, &maxY);

            return cv::Rect2d(cv::Point2d(minX, minY), cv::Point2d(maxX + 1, maxY + 1));
        }


        std::vector<MPFRotatedRect> fullFrame(const cv::Size &frameSize) {
            return { MPFRotatedRect(0, 0, frameSize.width, frameSize.height, 0, false) };
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



    AffineTransformation::AffineTransformation(
                const std::vector<MPFRotatedRect> &preTransformRegions,
                double frameRotationDegrees,
                bool flip,
                const cv::Scalar &fillColor,
                const SearchRegion &postTransformSearchRegion)
            : rotationDegrees_(frameRotationDegrees)
            , flip_(flip)
            , fillColor_(fillColor)
    {
        if (preTransformRegions.empty()) {
            throw std::length_error(
                    "The \"preTransformRegions\" parameter must contain at least one element, but it was empty.");
        }

        // Rotating an image around the origin will move some or all of the pixels out of the frame.
        cv::Matx33d rotationMat = IndividualXForms::Rotation(360 - frameRotationDegrees);
        // Use the rotation matrix to figure out where the pixels we are looking for ended up.
        cv::Rect mappedBoundingRect = GetMappedBoundingRect(preTransformRegions, rotationMat);
        // Shift the pixels we are looking for back in to the frame.
        cv::Matx33d moveRoiToOrigin = IndividualXForms::Translation(-mappedBoundingRect.x, -mappedBoundingRect.y);

        // The search region is generally specified by a human, so for convenience the coordinates are relative to
        // the correctly oriented image.
        // searchRegionRect will either be the same as mappedBoundingRect or be contained within mappedBoundingRect.
        cv::Rect searchRegionRect = postTransformSearchRegion.GetRect(mappedBoundingRect.size());
        regionSize_ = searchRegionRect.size();
        // When searchRegionRect is smaller than mappedBoundingRect, we need to move the searchRegionRect
        // to the origin. This slides the pixels outside of the search region off of the frame.
        cv::Matx33d moveSearchRegionToOrigin = IndividualXForms::Translation(-searchRegionRect.x, -searchRegionRect.y);

        cv::Matx33d combinedTransform;
        if (flip) {
            /*
                       -x     x=0     +x
              initial: [     ] | [ a b ]
              flipped: [ b a ] | [     ]
              shift:   [     ] | [ b a ]
            */
            cv::Matx33d flipMat = IndividualXForms::HorizontalFlip();
            cv::Matx33d flipShiftCorrection = IndividualXForms::Translation(mappedBoundingRect.width - 1, 0);
            // Transformations are applied from right to left, so rotation occurs first.
            combinedTransform = moveSearchRegionToOrigin * flipShiftCorrection * flipMat * moveRoiToOrigin * rotationMat;
        }
        else {
            // Transformations are applied from right to left, so rotation occurs first.
            combinedTransform = moveSearchRegionToOrigin * moveRoiToOrigin * rotationMat;
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
                       cv::InterpolationFlags::WARP_INVERSE_MAP | cv::InterpolationFlags::INTER_CUBIC,
                       cv::BORDER_CONSTANT, fillColor_);
    }



    void AffineTransformation::ApplyReverse(MPFImageLocation &imageLocation) const {
        cv::Vec3d topLeft(imageLocation.x_left_upper, imageLocation.y_left_upper, 1);
        cv::Vec2d newTopLeft = reverseTransformationMatrix_ * topLeft;

        imageLocation.x_left_upper = cv::saturate_cast<int>(newTopLeft[0]);
        imageLocation.y_left_upper = cv::saturate_cast<int>(newTopLeft[1]);

        if (!DetectionComponentUtils::RotationAnglesEqual(rotationDegrees_, 0)) {
            double existingRotation
                    = DetectionComponentUtils::GetProperty(imageLocation.detection_properties, "ROTATION", 0.0);

            double rotationAdjustAmount = flip_ ? 360 - rotationDegrees_ : rotationDegrees_;
            double newRotation = DetectionComponentUtils::NormalizeAngle(existingRotation + rotationAdjustAmount);

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




    // Search region frame cropping on rotated frame constructor.
    AffineFrameTransformer::AffineFrameTransformer(double rotation,
                                                   bool flip,
                                                   const cv::Scalar &fillColor,
                                                   const SearchRegion &searchRegion,
                                                   IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform))
            , transform_(fullFrame(GetInnerFrameSize(0)), rotation, flip, fillColor, searchRegion)
    {
    }


    // Rotate full frame constructor.
    AffineFrameTransformer::AffineFrameTransformer(double rotation,
                                                   bool flip,
                                                   const cv::Scalar &fillColor,
                                                   IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform))
            , transform_(fullFrame(GetInnerFrameSize(0)), rotation, flip, fillColor)
    {
    }


    // Feed forward superset region constructor
    AffineFrameTransformer::AffineFrameTransformer(const std::vector<MPFRotatedRect> &regions,
                                                   double frameRotation, bool frameFlip,
                                                   const cv::Scalar &fillColor,
                                                   IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform))
            , transform_(regions, frameRotation, frameFlip, fillColor)
    {
    }


    cv::Size AffineFrameTransformer::GetFrameSize(int frameIndex) const {
        return transform_.GetRegionSize();
    }


    void AffineFrameTransformer::DoFrameTransform(cv::Mat &frame, int frameIndex) const {
        transform_.Apply(frame);
    }


    void AffineFrameTransformer::DoReverseTransform(MPFImageLocation &imageLocation,
                                                    int frameIndex) const {
        transform_.ApplyReverse(imageLocation);
    }




    FeedForwardExactRegionAffineTransformer::FeedForwardExactRegionAffineTransformer(
                const std::vector<MPFRotatedRect> &regions,
                const cv::Scalar &fillColor,
                IFrameTransformer::Ptr innerTransform)
            : BaseDecoratedTransformer(std::move(innerTransform))
            , frameTransforms_(CreateTransformations(regions, fillColor))
    {
    }


    cv::Size FeedForwardExactRegionAffineTransformer::GetFrameSize(int frameIndex) const {
        return GetTransform(frameIndex).GetRegionSize();
    }


    std::vector<AffineTransformation> FeedForwardExactRegionAffineTransformer::CreateTransformations(
            const std::vector<MPFRotatedRect> &regions, const cv::Scalar &fillColor) {

        std::vector<AffineTransformation> transforms;
        transforms.reserve(regions.size());

        for (const auto& region : regions) {
            double frameRotation = region.flip ? 360 - region.rotation : region.rotation;
            transforms.emplace_back(std::vector<MPFRotatedRect>{ region }, frameRotation,
                                    region.flip, fillColor);
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

    void FeedForwardExactRegionAffineTransformer::DoFrameTransform(cv::Mat &frame,
                                                                   int frameIndex) const {
        GetTransform(frameIndex).Apply(frame);
    }

    void FeedForwardExactRegionAffineTransformer::DoReverseTransform(
            MPFImageLocation &imageLocation, int frameIndex) const {
        GetTransform(frameIndex).ApplyReverse(imageLocation);
    }
}}
