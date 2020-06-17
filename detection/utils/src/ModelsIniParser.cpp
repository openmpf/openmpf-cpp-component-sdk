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

#include "ModelsIniParser.h"

#include <fstream>

#include <boost/algorithm/string/join.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>


namespace MPF { namespace COMPONENT { namespace ModelsIniHelpers {

    std::string GetFullPath(const std::string &file_name, const std::string& plugin_models_dir,
                            const std::string &common_models_dir) {
        if (file_name.empty()) {
            throw MPFDetectionException(
                    MPF_COULD_NOT_READ_DATAFILE,
                    "Failed to load model because one of the fields in the models ini file was empty.");
        }

        std::string expanded_file_name;
        std::string exp_error = Utils::expandFileName(file_name, expanded_file_name);
        if (!exp_error.empty()) {
            throw MPFDetectionException(
                    MPF_COULD_NOT_READ_DATAFILE,
                    "Failed to load model because the expansion of \"" + file_name
                    + "\" failed due to: " + exp_error);
        }
        if (expanded_file_name.empty()) {
            throw MPFDetectionException(
                    MPF_COULD_NOT_READ_DATAFILE,
                    "Failed to load model because \"" + file_name +  "\" expanded to the empty string.");
        }

        std::vector<std::string> possible_locations;
        if (expanded_file_name.front() == '/') {
            possible_locations.push_back(expanded_file_name);
        }
        else {
            possible_locations.push_back(common_models_dir + '/' + expanded_file_name);
            possible_locations.push_back(plugin_models_dir + '/' + expanded_file_name);
        }


        for (const auto& possible_location : possible_locations) {
            if (std::ifstream(possible_location).good()) {
                return possible_location;
            }
        }

        std::string error_msg = "Failed to load model because a required file was not present. ";
        if (expanded_file_name.front() == '/') {
            error_msg += "Expected a file at \"" + expanded_file_name + "\" to exist.";
        }
        else {
            error_msg += "Expected a file to exist at either \"" + possible_locations.at(0)
                         + "\" or \"" + possible_locations.at(1) + "\".";
        }

        throw MPFDetectionException(MPF_COULD_NOT_OPEN_DATAFILE, error_msg);
    }


    IniHelper::IniHelper(const std::string &file_path, const std::string &model_name)
        : model_name_(model_name)
    {
        using namespace boost::property_tree;
        iptree all_models_ini;
        try {
            ini_parser::read_ini(file_path, all_models_ini);
        }
        catch (const ini_parser::ini_parser_error &ex) {
            throw MPFDetectionException(MPF_COULD_NOT_READ_DATAFILE,
                                        "Failed to open \"" + file_path + "\" due to: " + ex.what());
        }


        const auto &model_iter = all_models_ini.find(model_name);
        if (model_iter == all_models_ini.not_found() || model_iter->second.empty()) {
            std::vector<std::string> model_names_vec;
            for (auto it = all_models_ini.ordered_begin(); it != all_models_ini.not_found(); ++it) {
                model_names_vec.emplace_back("[" + it->first + "]");
            }
            std::string joined_model_names = boost::algorithm::join(model_names_vec, ", ");

            throw MPFDetectionException(
                    MPF_COULD_NOT_READ_DATAFILE,
                    "Failed to load model \"" + model_name
                    + "\" because the models.ini file did not contain a non-empty section named ["
                    + model_name + "], but it did contain the following models: " + joined_model_names);
        }

        for (const auto &sub_tree : model_iter->second) {
            model_ini_fields_.emplace(sub_tree.first, sub_tree.second.data());
        }
    }

    std::string IniHelper::GetValue(const std::string &key) const {
        try {
            return model_ini_fields_.at(key);
        }
        catch (std::out_of_range &ex) {
            throw MPFDetectionException(
                    MPF_COULD_NOT_READ_DATAFILE,
                    "Unable to load the \"" + model_name_ + "\" model because the \"" + key
                    + "\" key was not present in the [" + model_name_ + "] section.");
        }
    }

    std::pair<bool, std::string> IniHelper::GetOptionalValue(const std::string &key) const {
        auto iter = model_ini_fields_.find(key);
        if (iter == model_ini_fields_.end()) {
            return {false, ""};
        }
        else {
            return {true, iter->second};
        }
    }
}}}

