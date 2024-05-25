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

#ifndef OPENMPF_CPP_COMPONENT_SDK_AFFINEFRAMETRANSFORMER_H
#define OPENMPF_CPP_COMPONENT_SDK_AFFINEFRAMETRANSFORMER_H

#include <tuple>
#include <vector>

#include <opencv2/core.hpp>

#include "BaseDecoratedTransformer.h"
#include "IFrameTransformer.h"
#include "MPFDetectionObjects.h"
#include "SearchRegion.h"
#include "MPFRotatedRect.h"


namespace MPF { namespace COMPONENT {


    class AffineTransformation {
    public:
        AffineTransformation(
                const std::vector<MPFRotatedRect> &preTransformRegions,
                double frameRotationDegrees, bool flip,
                const cv::Scalar &fillColor,
                const SearchRegion &postTransformSearchRegion = {});

        void Apply(cv::Mat &frame) const;

        void ApplyReverse(MPFImageLocation &imageLocation) const;

        cv::Size2d GetRegionSize() const;

    private:
        double rotationDegrees_;

        bool flip_;

        cv::Size2d regionSize_;

        cv::Scalar fillColor_;

        // OpenCV does the mapping in the reverse order (from destination to the source) to avoid sampling artifacts.
        cv::Matx23d reverseTransformationMatrix_;
    };



    class AffineFrameTransformer : public BaseDecoratedTransformer {

    public:
        // Search region frame cropping on rotated frame constructor.
        AffineFrameTransformer(double rotation, bool flip, const cv::Scalar &fillColor,
                               const SearchRegion &searchRegion,
                               IFrameTransformer::Ptr innerTransform);

        // Rotate full frame constructor.
        AffineFrameTransformer(double rotation, bool flip, const cv::Scalar &fillColor,
                               IFrameTransformer::Ptr innerTransform);

        // Feed forward superset region constructor
        AffineFrameTransformer(const std::vector<MPFRotatedRect> &regions,
                               double frameRotation, bool frameFlip, const cv::Scalar &fillColor,
                               IFrameTransformer::Ptr innerTransform);

        cv::Size GetFrameSize(int frameIndex) const override;


    protected:
        void DoFrameTransform(cv::Mat &frame, int frameIndex) const override;

        void DoReverseTransform(MPFImageLocation &imageLocation, int frameIndex) const override;
        
    private:
        const AffineTransformation transform_;
    };



    class FeedForwardExactRegionAffineTransformer : public BaseDecoratedTransformer {

    public:
        // Feed forward exact region constructor
        FeedForwardExactRegionAffineTransformer(const std::vector<MPFRotatedRect> &regions,
                                                const cv::Scalar &fillColor,
                                                IFrameTransformer::Ptr innerTransform);

        cv::Size GetFrameSize(int frameIndex) const override;


    protected:
        void DoFrameTransform(cv::Mat &frame, int frameIndex) const override;

        void DoReverseTransform(MPFImageLocation &imageLocation, int frameIndex) const override;

    private:
        const std::vector<AffineTransformation> frameTransforms_;

        const AffineTransformation& GetTransform(int frameIndex) const;

        static std::vector<AffineTransformation> CreateTransformations(
                const std::vector<MPFRotatedRect> &regions, const cv::Scalar &fillColor);

    };
}}



#endif //OPENMPF_CPP_COMPONENT_SDK_AFFINEFRAMETRANSFORMER_H
