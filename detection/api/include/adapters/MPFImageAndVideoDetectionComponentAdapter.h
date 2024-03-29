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


#ifndef OPENMPF_CPP_COMPONENT_SDK_MPFIMAGEANDVIDEODETECTIONCOMPONENTADAPTER_H
#define OPENMPF_CPP_COMPONENT_SDK_MPFIMAGEANDVIDEODETECTIONCOMPONENTADAPTER_H

#include <vector>

#include "MPFDetectionComponent.h"
#include "MPFDetectionException.h"

namespace MPF { namespace COMPONENT {

    class MPFImageAndVideoDetectionComponentAdapter : public MPFDetectionComponent {

    public:
        std::vector<MPFAudioTrack> GetDetections(const MPFAudioJob &job) override {
            throw MPFDetectionException(MPFDetectionError::MPF_UNSUPPORTED_DATA_TYPE);
        };

        std::vector<MPFGenericTrack> GetDetections(const MPFGenericJob &job) override {
            throw MPFDetectionException(MPFDetectionError::MPF_UNSUPPORTED_DATA_TYPE);
        }


        bool Supports(MPFDetectionDataType data_type) override {
            return (MPFDetectionDataType::IMAGE == data_type) || (MPFDetectionDataType::VIDEO == data_type);
        };


    protected:
        MPFImageAndVideoDetectionComponentAdapter() = default;
    };
}}


#endif //OPENMPF_CPP_COMPONENT_SDK_MPFIMAGEANDVIDEODETECTIONCOMPONENTADAPTER_H
