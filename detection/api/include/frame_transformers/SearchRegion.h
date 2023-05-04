/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2023 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2023 The MITRE Corporation                                       *
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

#ifndef OPENMPF_CPP_COMPONENT_SDK_SEARCHREGION_H
#define OPENMPF_CPP_COMPONENT_SDK_SEARCHREGION_H

#include <functional>

#include <opencv2/core.hpp>


namespace MPF { namespace COMPONENT {

    /**
     * Holds the information about a search region as described by job properties. When a frame is going to be
     * rotated the frame size is not known at the same time the job properties are parsed. This class can be
     * constructed when parsing the job properties. Then, when the frame size is known, GetRect can be called.
     */
    class SearchRegion {
    public:
        using resolve_region_edge_t = std::function<int(int, int)>;

        /**
         * Creates a search region where the region is the entire frame.
         */
        SearchRegion();

        SearchRegion(resolve_region_edge_t left,
                     resolve_region_edge_t top,
                     resolve_region_edge_t right,
                     resolve_region_edge_t bottom);

        cv::Rect GetRect(const cv::Size &frameSize) const;

    private:
        resolve_region_edge_t left_;
        resolve_region_edge_t top_;
        resolve_region_edge_t right_;
        resolve_region_edge_t bottom_;
    };


    namespace RegionEdge {
        using resolve_region_edge_t = std::function<int(int, int)>;

        // Used when a value was not provided for an edge.
        resolve_region_edge_t Default();

        // Use when the value for an edge is specified as pixel coordinate.
        resolve_region_edge_t Absolute(int value);

        // Used when the value for an edge is specified as a percentage.
        resolve_region_edge_t Percentage(double percentage);
    }
}}




#endif //OPENMPF_CPP_COMPONENT_SDK_SEARCHREGION_H
