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


#ifndef OPENMPF_CPP_COMPONENT_SDK_DETECTIONCOMPONENTUTILS_H
#define OPENMPF_CPP_COMPONENT_SDK_DETECTIONCOMPONENTUTILS_H


#include <string>
#include <utility>

#include <boost/lexical_cast.hpp>

#include "MPFDetectionComponent.h"


namespace DetectionComponentUtils {


    // Allow string literals to be passed to GetProperty without wrapping them in an std::string.
    // For example, instead of GetProperty(props, "KEY", std::string("default")) you can just use
    // GetProperty(props, "KEY", "default")
    template <typename T>
    using t_unless_char_ptr_then_string
        = typename std::conditional<
                std::is_same<T, const char*>::value,
                std::string,
                T
          >::type;

    template<typename T>
    t_unless_char_ptr_then_string<T> GetProperty(const MPF::COMPONENT::Properties &props,
                                                 const std::string &key,
                                                 const T defaultValue) {
        auto iter = props.find(key);
        if (iter == props.end()) {
            return defaultValue;
        }

        try {
            return boost::lexical_cast<t_unless_char_ptr_then_string<T>>(iter->second);
        }
        catch (const boost::bad_lexical_cast &e) {
            return defaultValue;
        }
    }

    /**
     * Specialization of GetProperty to support converting the string "true" to a boolean.
     * This specialization is necessary because boost::lexical_cast only considers the string "1" to be true.
     * @param props
     * @param key
     * @param defaultValue
     * @return true if the property value is "true" (case-insensitive) or "1"; false otherwise
     */
    template<>
    bool GetProperty(const MPF::COMPONENT::Properties &props,
                     const std::string &key,
                     const bool defaultValue);


    /**
     * Exception dispatcher pattern from https://isocpp.org/wiki/faq/exceptions#throw-without-an-object
     * Converts the current exception to MPF::COMPONENT::MPFDetectionException, adds context to the error message,
     * and re-throws it.
     * @param dataType Type of the input data that caused the exception
     * @throws MPF::COMPONENT::MPFDetectionException
     */
    [[noreturn]] void ReThrowAsMpfDetectionException(MPF::COMPONENT::MPFDetectionDataType dataType);


    double NormalizeAngle(double angle);

    bool RotationAnglesEqual(double a1, double a2, double epsilon = 0.1);
}


#endif //OPENMPF_CPP_COMPONENT_SDK_DETECTIONCOMPONENTUTILS_H
