/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2017 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2017 The MITRE Corporation                                       *
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


#include <detectionComponentUtils.h>
#include "KeyFrameFilter.h"


namespace MPF { namespace COMPONENT {

    KeyFrameFilter::KeyFrameFilter(const MPFVideoJob &job)
            : FrameListFilter(GetKeyFrames(job)) {

    }

    std::vector<int> KeyFrameFilter::GetKeyFrames(const MPFVideoJob &job) {
        std::string command =
                "ffprobe -loglevel warning -select_streams v -show_entries frame=key_frame -print_format flat=h=0 '"
                + job.data_uri + "'";
        FILE *pipe = popen(command.c_str(), "r");
        if (pipe == nullptr) {
            throw std::runtime_error("Unable to get key frames because ffprobe process failed to start.");
        }

        std::vector<int> keyFrames;
        char lineBuf[128];
        int numKeyFramesSeen = 0;
        int frameInterval = std::max(1, DetectionComponentUtils::GetProperty(job.job_properties, "FRAME_INTERVAL", 1));

        while (fgets(lineBuf, 128, pipe) != nullptr) {
            // Expected line format for key frame: frame.209.key_frame=1
            // Expected line format for non-key frame: frame.210.key_frame=0
            std::string lineStr = lineBuf;

            size_t firstDot = lineStr.find('.');
            if (firstDot == std::string::npos) {
                continue;
            }

            int frameNumber = std::stoi(lineStr.substr(firstDot + 1));
            if (frameNumber < job.start_frame) {
                continue;
            }
            if (frameNumber > job.stop_frame) {
                break;
            }

            if (lineStr.find("key_frame=1") == std::string::npos) {
                continue;
            }

            if (numKeyFramesSeen % frameInterval == 0) {
                keyFrames.push_back(frameNumber);
            }
            numKeyFramesSeen++;
        }

        int returnCode = pclose(pipe);
        if (returnCode != 0) {
            int ffprobeExitStatus = (returnCode >> 8) & 255; // bits 15-8 contain the subprocess's exit code.
            throw std::runtime_error("Unable to get key frames because the ffprobe process exited with exit code: "
                                     + std::to_string(ffprobeExitStatus) + '.');
        }

        keyFrames.shrink_to_fit();
        return keyFrames;
    }
}}