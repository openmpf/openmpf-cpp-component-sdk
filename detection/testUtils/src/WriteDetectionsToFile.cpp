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

#include <ctime>
#include <fstream>
#include <iostream>
#include <map>

#include "WriteDetectionsToFile.h"

using std::ifstream;
using std::map;
using std::ofstream;
using std::string;
using std::vector;

namespace MPF { namespace COMPONENT { namespace WriteDetectionsToFile {

    namespace {
        bool FileExists(const string &filepath) {
            return ifstream(filepath).good();
        }
    }

    void WriteVideoTracks(const string &csv_filepath, const vector<MPFVideoTrack> &tracks) {
        string path = string(csv_filepath);
        if (FileExists(csv_filepath.c_str())) {
            time_t t = time(0);
            struct tm *now = localtime(&t);

            string timeString = "";
            timeString = std::to_string((now->tm_year + 1900));
            timeString = timeString + "_" + std::to_string((now->tm_mon + 1));
            timeString = timeString + "_" + std::to_string((now->tm_mday));

            size_t beginExt = csv_filepath.find_last_of(".");
            string noExtStr = csv_filepath.substr(0, beginExt);

            string new_path = noExtStr + "_" + timeString + ".tracks";

            path = new_path;

            ifstream src(csv_filepath.c_str());
            ofstream dst(new_path.c_str());

            dst << src.rdbuf();

            dst.close();
        }

        if (!tracks.empty()) {
            ofstream my_file;

            my_file.open(path.c_str());
            if (!my_file.good()) {
                std::cout << std::endl << "Failure to open output file" << std::endl;
                return;
            }

            my_file << path << std::endl;

            for (unsigned int i = 0; i < tracks.size(); i++) {
                my_file << "#" << i << std::endl;

                my_file << tracks[i].start_frame << std::endl;
                my_file << tracks[i].stop_frame << std::endl;

                for (map<int, MPFImageLocation>::const_iterator it = tracks[i].frame_locations.begin();
                     it != tracks[i].frame_locations.end(); it++) {
                    my_file << it->second.x_left_upper << ","
                            << it->second.y_left_upper << ","
                            << it->second.width << ","
                            << it->second.height << std::endl;
                }
            }

            if (my_file.is_open()) {
                my_file.close();
            }
        }
        else {
            std::cout << std::endl << "--No tracks found--" << std::endl;
        }

    }


    void WriteVideoTracks(const string &csv_filepath,
                          const vector<MPFImageLocation> &locations) {
        string path = string(csv_filepath);

        if (FileExists(csv_filepath.c_str())) {
            time_t t = time(0);
            struct tm *now = localtime(&t);

            string timeString = "";
            timeString = std::to_string((now->tm_year + 1900));
            timeString = timeString + "_" + std::to_string((now->tm_mon + 1));
            timeString = timeString + "_" + std::to_string((now->tm_mday));

            size_t beginExt = csv_filepath.find_last_of(".");
            string noExtStr = csv_filepath.substr(0, beginExt);

            string new_path = noExtStr + "_" + timeString + ".tracks";

            path = new_path;

            std::ifstream src(csv_filepath.c_str());
            std::ofstream dst(new_path.c_str());

            dst << src.rdbuf();

            dst.close();
        }

        if (!locations.empty()) {
            ofstream my_file;

            my_file.open(path.c_str());
            if (!my_file.good()) {
                std::cout << std::endl << "Failure to open output file" << std::endl;
                return;
            }

            my_file << path << std::endl;

            for (unsigned int i = 0; i < locations.size(); i++) {
                my_file << locations[i].x_left_upper << ","
                        << locations[i].y_left_upper << ","
                        << locations[i].width << ","
                        << locations[i].height << std::endl;
            }

            if (my_file.is_open()) {
                my_file.close();
            }
        }
        else {
            std::cout << std::endl << "--No tracks found--" << std::endl;
        }

    }

}}}