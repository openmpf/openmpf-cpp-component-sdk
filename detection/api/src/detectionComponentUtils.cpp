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

#include <algorithm>
#include <cctype>
#include <cmath>
#include <exception>

#include <opencv2/core.hpp>

#include "MPFInvalidPropertyException.h"

#include "detectionComponentUtils.h"


using std::exception;
using std::pair;
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

    template<>
    bool GetProperty<bool>(const Properties &props,
                           const string &key,
                           const bool defaultValue) {
        const string &propValue = GetProperty<string>(props, key, "");
        if (propValue.empty()) {
            return defaultValue;
        }
        if (propValue == "1") {
            return true;
        }

        static const string trueString = "TRUE";
        return std::equal(trueString.begin(), trueString.end(), propValue.begin(), [](char trueChar, char propChar) {
            return trueChar == std::toupper(propChar);
        });
    }


    pair<MPFDetectionError, string> HandleDetectionException(MPFDetectionDataType dataType) {
        const string &dataTypeName = DetectionDataTypeToString(dataType);

        try {
            throw;
        }
        catch (const MPFDetectionException &ex) {
            return {
                ex.error_code,
                "An exception occurred while trying to get detections from " + dataTypeName + ": " + ex.what()
            };
        }
        catch (const cv::Exception &ex) {
            return {
                MPFDetectionError::MPF_DETECTION_FAILED,
                "OpenCV raised an exception while trying to get detections from " + dataTypeName + ": " + ex.what()
            };
        }
        catch (const exception &ex) {
            return {
                MPFDetectionError::MPF_OTHER_DETECTION_ERROR_TYPE,
                "An exception occurred while trying to get detections from " + dataTypeName + ": " + ex.what()
            };
        }
        catch (...) {
            return {
                MPFDetectionError::MPF_OTHER_DETECTION_ERROR_TYPE,
                "An unknown error occurred while trying to get detections from " + dataTypeName + "."
            };
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
        return std::abs(NormalizeAngle(a1) - NormalizeAngle(a2)) < epsilon;
    }
}
 
