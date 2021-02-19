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

#ifndef OPENMPF_CPP_COMPONENT_SDK_MPFROTATEDRECT_H
#define OPENMPF_CPP_COMPONENT_SDK_MPFROTATEDRECT_H

#include <array>

#include <opencv2/core.hpp>

namespace MPF { namespace COMPONENT {

    /**
     * Represents a rectangle that may or may not be axis aligned.
     * The algorithm to "draw" the rectangle, is as follows:
     * 1. Draw the rectangle ignoring rotation and flip.
     * 2. Stick a pin in the top left corner of the rectangle because the top left doesn't move,
     *    but the rest of the rectangle may be moving.
     * 3. Rotate the rectangle counter-clockwise the given number of degrees around its top left corner.
     * 4. If the rectangle is flipped, flip horizontally around the top left corner.
     */
    class MPFRotatedRect {

    public:
        const double x;
        const double y;
        const double width;
        const double height;
        const double rotation;
        const bool flip;


        MPFRotatedRect(double x, double y, double width, double height, double rotation = 0, bool flip = false);

        std::array<cv::Point2d, 4> GetCorners() const;

        cv::Rect2d GetBoundingRect() const;

    private:
        cv::Matx23d GetTransformMat() const;

    };

}}



#endif //OPENMPF_CPP_COMPONENT_SDK_MPFROTATEDRECT_H
