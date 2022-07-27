/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2022 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2022 The MITRE Corporation                                       *
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



#ifndef OPENMPF_CPP_COMPONENT_SDK_MPFINVALIDPROPERTYEXCEPTION_H
#define OPENMPF_CPP_COMPONENT_SDK_MPFINVALIDPROPERTYEXCEPTION_H

#include <string>
#include <stdexcept>

#include "MPFDetectionComponent.h"
#include "MPFDetectionException.h"

namespace MPF { namespace COMPONENT {

    class MPFInvalidPropertyException : public MPFDetectionException {

    public:
        explicit MPFInvalidPropertyException(const std::string &propertyName,
                                             const std::string &reason = "",
                                             MPFDetectionError detectionError = MPF_INVALID_PROPERTY)
                : MPFDetectionException(
                    detectionError,
                    "The " + propertyName + " job property contained an invalid value. " + reason) {

        }
    };

}}

#endif //OPENMPF_CPP_COMPONENT_SDK_MPFINVALIDPROPERTYEXCEPTION_H
