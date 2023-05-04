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

#include <iostream>
#include <fstream>

#include "ReadDetectionsFromFile.h"


using std::ifstream;
using std::string;
using std::vector;

namespace MPF { namespace COMPONENT { namespace ReadDetectionsFromFile {

    bool ReadVideoTracks(string &csv_filepath, vector<MPFVideoTrack> &tracks) {

        ifstream in_file(csv_filepath.c_str());

        if (!in_file.is_open()) {
            std::cout << "ERROR - failed to open the input file: " << csv_filepath << std::endl;
            return false;
        }

        string delimiter = ",";
        size_t pos = 0;
        string token;

        int next_track_index = 0;
        MPFVideoTrack track;

        string line;
        while (std::getline(in_file, line)) {
            if (!line.empty()) {

                if (line[0] == '#') {
                    // create first track
                    string sub = line.substr(1);
                    int track_index = atoi(sub.c_str());
                    if (next_track_index == track_index) {
                        // push back the track before resetting it
                        if (next_track_index > 0) {
                            tracks.push_back(track);
                        }

                        MPFVideoTrack default_track;
                        // reset the track object
                        track = default_track;
                        next_track_index++;
                    }

                    // get start frame and stop frame
                    getline(in_file, line);
                    track.start_frame = atoi(line.c_str());
                    getline(in_file, line);
                    track.stop_frame = atoi(line.c_str());

                    continue;
                }

                if ((track.start_frame != -1) && (track.stop_frame != -1)) {
                    int delim_index = 0;
                    MPFImageLocation detection;
                    while ((pos = line.find(delimiter)) != string::npos) {
                        token = line.substr(0, pos);

                        switch (delim_index) {
                            case 0:
                                detection.x_left_upper = atoi(token.c_str());
                                break;
                            case 1:
                                detection.y_left_upper = atoi(token.c_str());
                                break;
                            case 2:
                                detection.width = atoi(token.c_str());
                                // ugly hack to keep the loop going
                                line = line + ",";
                                break;
                            case 3:
                                detection.height = atoi(token.c_str());
                                break;
                            default:
                                break;
                        }

                        delim_index++;

                        line.erase(0, pos + delimiter.length());
                    }

                    // at 4 can assume that it is a detection - can't think of any reason to output confidence
                    if (delim_index == 4) {
                        size_t frame_index = track.start_frame + track.frame_locations.size();
                        track.frame_locations[frame_index] = detection;
                    }
                }
            }

        }


        // if finished reading and the track has data then it needs to be pushed back as well
        if ((track.start_frame != -1) &&
            (track.stop_frame != -1) &&
            (!track.frame_locations.empty())) {
            tracks.push_back(track);
        }

        // close the file - if this is called again it will use the same file!!
        if (in_file.is_open()) {
            in_file.close();
        }

        return true;
    }


    bool ReadImageLocations(string &csv_filepath, vector<MPFImageLocation> &detections) {

        ifstream in_file(csv_filepath.c_str());

        if (!in_file.is_open()) {
            std::cout << "ERROR - failed to open the input file: " << csv_filepath << std::endl;
            return false;
        }

        string delimiter = ",";
        size_t pos = 0;
        string token;

        string line;
        while (std::getline(in_file, line)) {
            if (!line.empty()) {

                int delim_index = 0;
                MPFImageLocation detection;
                while ((pos = line.find(delimiter)) != string::npos) {
                    token = line.substr(0, pos);

                    switch (delim_index) {
                        case 0:
                            detection.x_left_upper = atoi(token.c_str());
                            break;
                        case 1:
                            detection.y_left_upper = atoi(token.c_str());
                            break;
                        case 2:
                            detection.width = atoi(token.c_str());
                            line = line + ",";
                            break;
                        case 3:
                            detection.height = atoi(token.c_str());
                            break;
                        default:
                            break;
                    }

                    delim_index++;

                    line.erase(0, pos + delimiter.length());
                }
                detections.push_back(detection);
            }

        }

        // close the file - if this is called again it will use the same file!!
        if (in_file.is_open()) {
            in_file.close();
        }

        return true;
    }
}}}

