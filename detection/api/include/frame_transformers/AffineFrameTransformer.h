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

#ifndef OPENMPF_CPP_COMPONENT_SDK_AFFINEFRAMETRANSFORMER_H
#define OPENMPF_CPP_COMPONENT_SDK_AFFINEFRAMETRANSFORMER_H

#include <map>

#include <opencv2/core.hpp>

#include <MPFDetectionObjects.h>

#include "BaseDecoratedTransformer.h"
#include "IFrameTransformer.h"


namespace MPF { namespace COMPONENT {


    class AffineTransformation {
    public:
        AffineTransformation(const cv::Rect &region, double rotationDegrees, bool flip, const cv::Size &frameSize);

        void Apply(cv::Mat &frame) const;

//        cv::Point Apply(const cv::Point &point) const;

//        void ApplyReverse(MPFImageLocation &imageLocation, const cv::Size &frameSize) const;
        void ApplyReverse(MPFImageLocation &imageLocation) const;

        const cv::Size2d sizeAfterRotation;

    private:
        const double rotationDegrees_;

        const cv::Matx23d transformationMatrix_;

        const cv::Matx23d reverseTransformationMatrix_;

        static cv::Size2d GetSizeAfterRotation(double rotationDegrees, const cv::Rect &region,
                                               const cv::Size &frameSize);

        static cv::Point2d GetCenter(const cv::Rect &region, double rotationDegrees);

        static cv::Matx23d GetTransformationMatrix(const cv::Rect &region, double rotationDegrees, bool flip,
                                                   const cv::Size2d &sizeAfterRotation);
        static cv::Matx23d GetTransformationMatrix2(const cv::Rect &region, double rotationDegrees, bool flip,
                                                   const cv::Size2d &sizeAfterRotation);

        static cv::Matx23d GetTransformationMatrix3(const cv::Rect &region, double rotationDegrees, bool flip,
                                                    const cv::Size2d &sizeAfterRotation);

        static cv::Matx23d GetReverseTransformationMatrix(const cv::Matx23d &transformationMatrix);


        static cv::Matx33d GetRotationMatrix(const cv::Point2d &center, double rotationDegrees);

        static cv::Matx33d GetHorizontalFlipMatrix();

        static cv::Matx33d GetTranslationMatrix(const cv::Vec2d &distance);

    };



    class AffineFrameTransformer : public BaseDecoratedTransformer {

    public:
        AffineFrameTransformer(const cv::Rect &region, double rotation, bool flip,
                               IFrameTransformer::Ptr innerTransform);

        cv::Size GetFrameSize(int frameIndex) override;


    protected:
        void DoFrameTransform(cv::Mat &frame, int frameIndex) override;

        void DoReverseTransform(MPFImageLocation &imageLocation, int frameIndex) override;
        
    private:
        const AffineTransformation transform_;
    };



    class FeedForwardAffineTransformer : public BaseDecoratedTransformer {

    public:
        FeedForwardAffineTransformer(const std::vector<std::tuple<cv::Rect, double, bool>> &transformInfo,
                                      IFrameTransformer::Ptr innerTransform);

        cv::Size GetFrameSize(int frameIndex) override;


    protected:
        void DoFrameTransform(cv::Mat &frame, int frameIndex) override;

        void DoReverseTransform(MPFImageLocation &imageLocation, int frameIndex) override;

    private:
        const std::vector<AffineTransformation> frameTransforms_;

        const AffineTransformation& GetTransform(int frameIndex);

        std::vector<AffineTransformation> GetTransformations(
                const std::vector<std::tuple<cv::Rect, double, bool>> &transformInfo);

    };
}}



#endif //OPENMPF_CPP_COMPONENT_SDK_AFFINEFRAMETRANSFORMER_H
