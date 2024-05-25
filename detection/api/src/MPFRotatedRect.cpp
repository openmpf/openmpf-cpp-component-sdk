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

#include "MPFRotatedRect.h"

#include <cassert>

#include <algorithm>

#include <opencv2/imgproc.hpp>

#include "detectionComponentUtils.h"


namespace MPF { namespace COMPONENT {


    MPFRotatedRect::MPFRotatedRect(double x, double y, double width, double height, double rotation, bool flip)
        : x(x)
        , y(y)
        , width(width)
        , height(height)
        , rotation(DetectionComponentUtils::NormalizeAngle(rotation))
        , flip(flip)
    {
    }


    std::array<cv::Point2d, 4> MPFRotatedRect::GetCorners() const {
        bool hasRotation = !DetectionComponentUtils::RotationAnglesEqual(rotation, 0);
        if (!hasRotation && !flip) {
            return {
                    cv::Point2d(x, y),
                    { x + width - 1, y },
                    { x + width - 1, y + height - 1 },
                    { x, y + height - 1 }
            };
        }

        cv::Matx23d xformMat = GetTransformMat();

        double trX = x + width - 1;
        double brY = y + height - 1;
        // The top left corner is not needed below because it doesn't move. If it were included, it still wouldn't
        // move after multiplying by the transformation matrix.
        cv::Matx33d cornerMat {
                trX, trX, x,
                  y, brY, brY,
                  1,   1, 1
        };
        cv::Matx23d xformedCorners = xformMat * cornerMat;

        return std::array<cv::Point2d, 4> {
                cv::Point2d(x, y),
                { xformedCorners(0, 0), xformedCorners(1, 0) },
                { xformedCorners(0, 1), xformedCorners(1, 1) },
                { xformedCorners(0, 2), xformedCorners(1, 2) },
        };
    }


    cv::Rect2d MPFRotatedRect::GetBoundingRect() const {
        std::array<cv::Point2d, 4> corners = GetCorners();

        const auto* cornerIter = corners.begin();
        double minX = cornerIter->x;
        double maxX = cornerIter->x;
        double minY = cornerIter->y;
        double maxY = cornerIter->y;
        ++cornerIter;

        for (; cornerIter != corners.end(); ++cornerIter) {
            minX = std::min(minX, cornerIter->x);
            maxX = std::max(maxX, cornerIter->x);
            minY = std::min(minY, cornerIter->y);
            maxY = std::max(maxY, cornerIter->y);
        }

        return cv::Rect2d(cv::Point2d(minX, minY),
                          cv::Point2d(maxX + 1, maxY + 1));
    }


    cv::Matx23d MPFRotatedRect::GetTransformMat() const {
        bool hasRotation = !DetectionComponentUtils::RotationAnglesEqual(rotation, 0);
        assert(hasRotation || flip);

        if (hasRotation && !flip) {
            // This case is handled separately because it is the most common case and we can avoid some work.
            return cv::getRotationMatrix2D(cv::Point2d(x, y), rotation, 1);
        }

        // At this point, we know we need to flip since we already checked !hasRotation && !flip before calling this
        // function.

        // The last column of flipMat makes it so the flip is around the top left coordinate, rather than the y-axis.
        // 2*x is used because the point is initially x units away from the y-axis. After flipping around the y-axis,
        // the mirrored point is also x units away from the y-axis, except in the opposite direction. This means there
        // is a total distance of 2*x between a point and its mirrored point.
        cv::Matx23d flipMat {
                -1, 0, 2 * x,
                 0, 1, 0
        };
        if (!hasRotation) {
            return flipMat;
        }

        cv::Matx33d rotationMat;
        cv::Mat rotationMat2D = cv::getRotationMatrix2D(cv::Point2d(x, y), rotation, 1);
        // Add a row to the rotation matrix so it has the size required to multiply with the flip matrix.
        cv::vconcat(rotationMat2D, cv::Matx13d(0, 0, 1), rotationMat);

        // Transform are applied from right to left, so rotation will occur before flipping.
        return flipMat * rotationMat;
    }


}}
