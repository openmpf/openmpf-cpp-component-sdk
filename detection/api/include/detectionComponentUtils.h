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


#ifndef OPENMPF_CPP_COMPONENT_SDK_DETECTIONCOMPONENTUTILS_H
#define OPENMPF_CPP_COMPONENT_SDK_DETECTIONCOMPONENTUTILS_H


#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <boost/lexical_cast.hpp>

#include "MPFDetectionComponent.h"


namespace DetectionComponentUtils {

    namespace detail {
        bool ToBool(const std::string& str);
    }

    template<typename T>
    std::optional<T> GetProperty(
            const MPF::COMPONENT::Properties &props,
            const std::string &key) {
        auto iter = props.find(key);
        if (iter == props.end()) {
            return {};
        }

        if constexpr (std::is_same_v<T, bool>) {
            return detail::ToBool(iter->second);
        }

        try {
            return boost::lexical_cast<T>(iter->second);
        }
        catch (const boost::bad_lexical_cast &e) {
            return {};
        }
    }


    template<typename T>
    T GetProperty(const MPF::COMPONENT::Properties &props,
                  const std::string &key,
                  T defaultValue) {
        return GetProperty<T>(props, key)
                        .value_or(std::move(defaultValue));
    }


    /**
     * Overload of GetProperty to support converting the string "true" to a boolean.
     * This overload is necessary because boost::lexical_cast only considers the string "1" to be true.
     * @param props
     * @param key
     * @param defaultValue
     * @return true if the property value is "true" (case-insensitive) or "1"; false otherwise
     */
    bool GetProperty(const MPF::COMPONENT::Properties &props,
                     const std::string &key,
                     bool defaultValue);

    // Overload so that a std::string gets returned when a user passes in a string literal as the
    // default value.
    std::string GetProperty(const MPF::COMPONENT::Properties &props,
                            const std::string &key,
                            const char* defaultValue);


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

    std::string GetAppDir(const char * argv0);
}


#endif //OPENMPF_CPP_COMPONENT_SDK_DETECTIONCOMPONENTUTILS_H
