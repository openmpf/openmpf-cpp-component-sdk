/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2024 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2024 The MITRE Corporation                                       *
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

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "MPFSimpleConfigLoader.h"


namespace MPF { namespace COMPONENT {
    namespace {
        std::string trim(const std::string &input) {
            std::istringstream ss(input);
            std::string result;
            ss >> result;
            return result;
        }

        std::string stripComments(const std::string &input) {
            auto commentPos = input.find_first_of('#');
            return input.substr(0, commentPos);
        }
    }


    std::map<std::string, std::string> LoadConfig(const std::string &config_path) {
        std::map<std::string, std::string> config_map;
        std::string line;
        std::ifstream file(config_path);
        while (std::getline(file, line)) {
            std::istringstream ss(stripComments(line));
            std::string key;
            if (!std::getline(ss, key, ':')) {
                continue;
            };

            auto trimmed_key = trim(key);
            if (trimmed_key.empty()) {
                throw std::runtime_error(
                        "\"" + config_path + "\" contained a line with a blank key.");
            }

            std::string val;
            if (!std::getline(ss, val, ':')) {
                continue;
            }
            config_map.emplace(std::move(trimmed_key), trim(val));
        }
        return config_map;
    }
}}
