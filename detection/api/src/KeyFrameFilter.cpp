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

#include <thread>
#include <cstdio>

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
        static const std::string linePrefix = "frame.";
        char lineBuf[128];
        int numKeyFramesSeen = 0;
        int frameInterval = std::max(1, DetectionComponentUtils::GetProperty(job.job_properties, "FRAME_INTERVAL", 1));

        while (fgets(lineBuf, 128, pipe) != nullptr) {
            // Expected line format for key frame: frame.209.key_frame=1
            // Expected line format for non-key frame: frame.210.key_frame=0
            std::string lineStr = lineBuf;
            if (!std::equal(linePrefix.begin(), linePrefix.end(), lineStr.begin())) {
                std::cerr << "Expected each line of output from ffprobe to start with \"" << linePrefix
                          << "\", but the following line was found: \"" << lineStr << "\".";
                continue;
            }

            size_t endOfFrameNumber = 0;
            int frameNumber = std::stoi(lineStr.substr(linePrefix.size()), &endOfFrameNumber);
            if (frameNumber < job.start_frame) {
                continue;
            }

            if (frameNumber > job.stop_frame) {
                // pclose waits for the subprocess to exit before returning. If we have already seen all the frames
                // in the current job's segment there is no need to wait until ffprobe exits.
                // Running pclose in a separate thread allows us to ensure pclose gets called without having to wait
                // for it to complete.
                std::thread(pclose, pipe).detach();
                keyFrames.shrink_to_fit();
                return keyFrames;
            }

            if (lineStr.find("key_frame=1", linePrefix.size() + endOfFrameNumber + 1) == std::string::npos) {
                continue;
            }

            if (numKeyFramesSeen % frameInterval == 0) {
                keyFrames.push_back(frameNumber);
            }
            numKeyFramesSeen++;
        }


        int returnCode = pclose(pipe);
        if (returnCode != 0) {
            std::stringstream errorMsg;
            errorMsg << "Unable to get key frames because the ffprobe process ";

            if (WIFEXITED(returnCode)) {
                int ffprobeExitStatus = WEXITSTATUS(returnCode);
                errorMsg << "exited with exit code: " << ffprobeExitStatus;
            }
            else {
                errorMsg << "did not exit normally";
            }

            if (WIFSIGNALED(returnCode)) {
                errorMsg << " due to signal number: " << WTERMSIG(returnCode);
            }
            errorMsg << '.';

            throw std::runtime_error(errorMsg.str());
        }

        keyFrames.shrink_to_fit();
        return keyFrames;
    }
}}