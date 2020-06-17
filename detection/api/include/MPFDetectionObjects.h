/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2020 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2020 The MITRE Corporation                                       *
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



#ifndef OPENMPF_CPP_COMPONENT_SDK_DETECTION_OBJECTS_H
#define OPENMPF_CPP_COMPONENT_SDK_DETECTION_OBJECTS_H

#include <map>
#include <string>

namespace MPF { namespace COMPONENT {
    typedef std::map<std::string, std::string> Properties;

    enum MPFDetectionDataType {
        UNKNOWN, VIDEO, IMAGE, AUDIO, INVALID_TYPE
    };

    enum MPFDetectionError {
        MPF_DETECTION_SUCCESS = 0,
        MPF_OTHER_DETECTION_ERROR_TYPE,
        MPF_DETECTION_NOT_INITIALIZED,
        MPF_UNRECOGNIZED_DATA_TYPE,
        MPF_UNSUPPORTED_DATA_TYPE,
        MPF_INVALID_DATAFILE_URI,
        MPF_COULD_NOT_OPEN_DATAFILE,
        MPF_COULD_NOT_READ_DATAFILE,
        MPF_FILE_WRITE_ERROR,
        MPF_IMAGE_READ_ERROR,
        MPF_BAD_FRAME_SIZE,
        MPF_BOUNDING_BOX_SIZE_ERROR,
        MPF_INVALID_FRAME_INTERVAL,
        MPF_INVALID_START_FRAME,
        MPF_INVALID_STOP_FRAME,
        MPF_DETECTION_FAILED,
        MPF_DETECTION_TRACKING_FAILED,
        MPF_INVALID_PROPERTY,
        MPF_MISSING_PROPERTY,
        MPF_PROPERTY_IS_NOT_INT,
        MPF_PROPERTY_IS_NOT_FLOAT,
        MPF_INVALID_ROTATION,
        MPF_MEMORY_ALLOCATION_FAILED,
        MPF_GPU_ERROR
    };

    struct MPFImageLocation {
        int x_left_upper;
        int y_left_upper;
        int width;
        int height;
        float confidence;  // optional
        Properties detection_properties;


        MPFImageLocation()
                : x_left_upper(-1)
                , y_left_upper(-1)
                , width(-1)
                , height(-1)
                , confidence(-1) {
        }

        MPFImageLocation(int x_left_upper,
                         int y_left_upper,
                         int width,
                         int height,
                         float confidence = -1,
                         const Properties &detection_properties = {})
                : x_left_upper(x_left_upper)
                , y_left_upper(y_left_upper)
                , width(width)
                , height(height)
                , confidence(confidence)
                , detection_properties(detection_properties) {
        }
    };


    struct MPFVideoTrack {
        int start_frame;
        int stop_frame;
        float confidence;  // optional
        std::map<int, MPFImageLocation> frame_locations;
        Properties detection_properties;

        MPFVideoTrack()
                : start_frame(-1)
                , stop_frame(-1)
                , confidence(-1) {
        }


        MPFVideoTrack(int start, int stop, float confidence = -1, const Properties &detection_properties = {})
                : start_frame(start)
                , stop_frame(stop)
                , confidence(confidence)
                , detection_properties(detection_properties) {
        }
    };


    struct MPFAudioTrack {
        int start_time;
        int stop_time;
        float confidence;  // optional
        Properties detection_properties;

        MPFAudioTrack()
                : start_time(-1)
                , stop_time(-1)
                , confidence(-1) {
        }

        MPFAudioTrack(int start, int stop, float confidence = -1, const Properties &detection_properties = {})
                : start_time(start)
                , stop_time(stop)
                , confidence(confidence)
                , detection_properties(detection_properties) { }
    };


    struct MPFGenericTrack {
        float confidence;  // optional
        Properties detection_properties;

        explicit MPFGenericTrack(float confidence = -1, const Properties &detection_properties = {})
                : confidence(confidence)
                , detection_properties(detection_properties) { }
    };

}}


#endif //OPENMPF_CPP_COMPONENT_SDK_DETECTION_OBJECTS_H
