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

#include "detectionComponentUtils.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>
#include <filesystem>

#include <opencv2/core.hpp>

#include "MPFInvalidPropertyException.h"


using std::exception;
using std::string;

using MPF::COMPONENT::MPFDetectionDataType;
using MPF::COMPONENT::MPFDetectionError;
using MPF::COMPONENT::MPFInvalidPropertyException;
using MPF::COMPONENT::Properties;
using MPF::COMPONENT::MPFDetectionException;


namespace DetectionComponentUtils {

    namespace {
        string DetectionDataTypeToString(MPFDetectionDataType dataType) {
            switch (dataType) {
                case MPFDetectionDataType::VIDEO:
                    return "video";
                case MPFDetectionDataType::IMAGE:
                    return "image";
                case MPFDetectionDataType::AUDIO:
                    return "audio";
                default:
                    return "unknown data type";
            }
        }
    }

    namespace detail {
        bool ToBool(const std::string& str) {
            if (str == "1") {
                return true;
            }
            static const string trueString = "TRUE";
            return std::equal(trueString.begin(), trueString.end(), str.begin(),
                              [](char trueChar, char propChar) {
                                  return trueChar == std::toupper(propChar);
                              });
        }
    }

    bool GetProperty(const Properties &props,
                     const string &key,
                     bool defaultValue) {
        return GetProperty<bool>(props, key).value_or(defaultValue);
    }

    std::string GetProperty(const MPF::COMPONENT::Properties &props,
                            const std::string &key,
                            const char* defaultValue) {
        return GetProperty<std::string>(props, key, defaultValue);
    }


    [[noreturn]] void ReThrowAsMpfDetectionException(MPFDetectionDataType dataType) {
        const string &dataTypeName = DetectionDataTypeToString(dataType);

        try {
            throw;
        }
        catch (const MPFDetectionException &ex) {
            throw MPFDetectionException(ex.error_code,
                    "An exception occurred while trying to get detections from " + dataTypeName
                    + ": " + ex.what());
        }
        catch (const cv::Exception &ex) {
            throw MPFDetectionException(MPFDetectionError::MPF_DETECTION_FAILED,
                    "OpenCV raised an exception while trying to get detections from " + dataTypeName
                    + ": " + ex.what());
        }
        catch (const exception &ex) {
            throw MPFDetectionException(MPFDetectionError::MPF_OTHER_DETECTION_ERROR_TYPE,
                    "An exception occurred while trying to get detections from " + dataTypeName
                    + ": " + ex.what());
        }
        catch (...) {
            throw MPFDetectionException(MPFDetectionError::MPF_OTHER_DETECTION_ERROR_TYPE,
                    "An unknown error occurred while trying to get detections from " + dataTypeName + ".");
        }
    }


    double NormalizeAngle(double angle) {
        if (0 <= angle && angle < 360) {
            return angle;
        }
        angle = std::fmod(angle, 360);
        if (angle >= 0) {
            return angle;
        }
        return 360 + angle;
    }


    bool RotationAnglesEqual(double a1, double a2, double epsilon) {
        a1 = NormalizeAngle(a1);
        a2 = NormalizeAngle(a2);
        if (std::abs(a1 - a2) < epsilon) {
            return true;
        }
        else {
            double a1_dist = std::min(a1, std::abs(360 - a1));
            double a2_dist = std::min(a2, std::abs(360 - a2));
            return (a1_dist + a2_dist) < epsilon;
        }
    }

    std::string GetAppDir(const char * const argv0) {
        namespace fs = std::filesystem;
        try {
            return fs::canonical("/proc/self/exe").parent_path();
        }
        catch (const fs::filesystem_error&) {
        }

        try {
            if (fs::path argv0Path(argv0); argv0Path.has_parent_path()) {
                return argv0Path.parent_path();
            }
        }
        catch (const fs::filesystem_error&) {
        }

        try {
            return fs::current_path();
        }
        catch (const fs::filesystem_error&) {
        }

        return ".";
    }
}
 
