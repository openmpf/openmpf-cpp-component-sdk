/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2018 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2018 The MITRE Corporation                                       *
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


#ifndef OPENMPF_CPP_COMPONENT_SDK_MODELSINIPARSER_H
#define OPENMPF_CPP_COMPONENT_SDK_MODELSINIPARSER_H


#include <string>
#include <vector>
#include <fstream>
#include <utility>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>


namespace MPF { namespace COMPONENT {

    class ModelsIniException : public std::runtime_error {
        using std::runtime_error::runtime_error;
    };


    template <typename TModelInfo>
    class ModelsIniParser {

    public:
        typedef std::string (TModelInfo::*class_field_t);


        ModelsIniParser& Init(const std::string &plugin_models_dir) {
            plugin_models_dir_ = plugin_models_dir;
            return *this;
        }

        ModelsIniParser& RegisterField(const std::string &key_name, class_field_t field) {
            if (key_name.empty()) {
                throw ModelsIniException("\"key_name\" must not be empty.");
            }
            fields_.emplace_back(key_name, field);
            return *this;
        }


        TModelInfo ParseIni(const std::string &model_name, const std::string &common_models_dir) const {
            using namespace boost::property_tree;
            iptree all_models_ini;
            std::string models_ini_path = GetFullPath("models.ini", common_models_dir);
            try {
                ini_parser::read_ini(models_ini_path, all_models_ini);
            }
            catch (const ini_parser::ini_parser_error &ex) {
                throw ModelsIniException("Failed to open \"" + models_ini_path + "\" due to: " + ex.what());
            }

            auto model_iter = all_models_ini.find(model_name);
            if (model_iter == all_models_ini.not_found()) {
                throw ModelsIniException("Unable to load model \"" + model_name
                                         + "\" because the models.ini file does not contain a section with the model name.");
            }

            TModelInfo model_info;
            for (const auto& field_info : fields_) {
                try {
                    std::string file_name = model_iter->second.get<std::string>(field_info.first);
                    model_info.*field_info.second = GetFullPath(file_name, common_models_dir);
                }
                catch (const ptree_bad_path &ex) {
                    throw ModelsIniException("Unable load the \"" + model_name + "\" model because the \""
                                             + field_info.first + "\" key was not present.");
                }
            }
            return model_info;
        }



    private:
        std::string plugin_models_dir_;

        std::vector<std::pair<std::string, class_field_t>> fields_;


        std::string GetFullPath(const std::string &file_name, const std::string &common_models_dir) const {

            std::vector<std::string> possible_locations;
            if (file_name.front() == '/') {
                possible_locations.push_back(file_name);
            }
            else {
                possible_locations.push_back(common_models_dir + '/' + file_name);
                possible_locations.push_back(plugin_models_dir_ + '/' + file_name);
            }


            for (const auto& possible_location : possible_locations) {
                if (std::ifstream(possible_location).good()) {
                    return possible_location;
                }
            }

            std::string error_msg = "Failed to load model because a required file was not present. ";
            if (file_name.front() == '/') {
                error_msg += "Expected a file at \"" + file_name + "\" to exist.";
            }
            else {
                error_msg += "Expected a file to exist at either \"" + possible_locations.at(0)
                             + "\" or \"" + possible_locations.at(1) + "\".";
            }

            throw ModelsIniException(error_msg);
        }
    };
}}



#endif //OPENMPF_CPP_COMPONENT_SDK_MODELSINIPARSER_H
