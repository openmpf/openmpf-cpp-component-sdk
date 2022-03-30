/******************************************************************************
 * NOTICE                                                                     *
 *                                                                            *
 * This software (or technical data) was produced for the U.S. Government     *
 * under contract, and is subject to the Rights in Data-General Clause        *
 * 52.227-14, Alt. IV (DEC 2007).                                             *
 *                                                                            *
 * Copyright 2021 The MITRE Corporation. All Rights Reserved.                 *
 ******************************************************************************/

/******************************************************************************
 * Copyright 2021 The MITRE Corporation                                       *
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

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/lexical_cast.hpp>

#include <detectionComponentUtils.h>
#include "MPFDetectionException.h"
#include "Utils.h"


namespace MPF::COMPONENT {


    namespace ModelsIniHelpers {
        // In order to avoid including the boost headers in this header,
        // everything that uses boost is in this class's implementation.
        // This makes it so the boost related code gets compiled as part of the utils library instead of as part of
        // the library that includes this.
        // Something about the boost property tree headers, probably their complexity,
        // prevents clang-tidy from working correctly. When the boost property tree headers were included
        // in this header, the clang-tidy issue spread to all files that included this file.
        class IniHelper {
        public:
            IniHelper(const std::string &file_path, const std::string &model_name);
            std::string GetValue(const std::string &key) const;
            std::optional<std::string> GetOptionalValue(const std::string &key) const;

        private:
            std::string model_name_;
            std::unordered_map<std::string, std::string> model_ini_fields_;
        };


        std::string GetFullPath(const std::string &file_name, const std::string& plugin_models_dir,
                                const std::string &common_models_dir);


        // Need to use a base class that doesn't have a template argument for the field type so that
        // the FieldInfo instances can all be added to the same vector.
        template <typename TModelInfo>
        class BaseFieldInfo {
        public:
            virtual ~BaseFieldInfo() = default;

            virtual void SetField(const IniHelper &helper, TModelInfo& model_info,
                                  const std::string &plugin_dir, const std::string &common_models_dir) = 0;
        };


        template <typename TModelInfo, typename TField>
        class FieldInfo : public BaseFieldInfo<TModelInfo> {
            using class_field_t = TField (TModelInfo::*);

        public:
            FieldInfo(std::string key_name, class_field_t field_reference, bool is_optional,
                      TField default_value)
                    : key_name_(std::move(key_name))
                    , field_reference_(field_reference)
                    , is_optional_(is_optional)
                    , default_value_(std::move(default_value)) {
                if (key_name_.empty()) {
                    throw std::invalid_argument("\"key_name\" must not be empty.");
                }
            }

            void SetField(
                    const IniHelper &helper,
                    TModelInfo& model_info,
                    const std::string &plugin_dir,
                    const std::string &common_models_dir) override {

                if (is_optional_) {
                    const auto ini_value = helper.GetOptionalValue(key_name_);
                    if (ini_value) {
                        model_info.*field_reference_
                            = ConvertValue(*ini_value, plugin_dir, common_models_dir);
                        return;
                    }
                    // This field was optional and it was not found in the ini file.
                    model_info.*field_reference_ = ConvertDefaultValue(
                            default_value_, plugin_dir, common_models_dir);
                    return;
                }
                const std::string string_value = helper.GetValue(key_name_);
                model_info.*field_reference_ = ConvertValue(string_value, plugin_dir, common_models_dir);
            }

        private:
            std::string key_name_;
            class_field_t field_reference_;
            bool is_optional_;
            TField default_value_;


            virtual TField ConvertValue(const std::string &string_value, const std::string &plugin_dir,
                                        const std::string &common_models_dir) {
                try {
                    return boost::lexical_cast<TField>(string_value);
                }
                catch (const boost::bad_lexical_cast &e) {
                    throw MPFDetectionException(
                            MPF_INVALID_PROPERTY,
                            "Failed to convert the \"" + key_name_ + "\" field to the target type due to: " + e.what());
                }
            };

            virtual TField ConvertDefaultValue(const TField &value, const std::string &plugin_dir,
                                               const std::string &common_models_dir) {
                return value;
            };
        };


        template <typename TModelInfo>
        class PathFieldInfo : public FieldInfo<TModelInfo, std::string> {
        public:
            using FieldInfo<TModelInfo, std::string>::FieldInfo;

        private:
            std::string ConvertValue(const std::string &string_value, const std::string &plugin_models_dir,
                                     const std::string &common_models_dir) override {
                return GetFullPath(string_value, plugin_models_dir, common_models_dir);
            }

            std::string ConvertDefaultValue(const std::string &value, const std::string &plugin_models_dir,
                                            const std::string &common_models_dir) {
                return value.empty() ? value : GetFullPath(value, plugin_models_dir, common_models_dir);
            };
        };
    }

    template <typename TModelInfo>
    class ModelsIniParser {
        using base_field_ptr_t = std::unique_ptr<ModelsIniHelpers::BaseFieldInfo<TModelInfo>>;

    public:
        // Pointer to member field
        template <typename TField>
        using class_field_t = TField (TModelInfo::*);


        ModelsIniParser& Init(const std::string &plugin_models_dir) {
            plugin_models_dir_ = plugin_models_dir;
            return *this;
        }


        template<typename T, typename... Args>
        ModelsIniParser& RegisterFieldCustomType(Args&&... args) {
            fields_.emplace_back(base_field_ptr_t(new T(std::forward<Args>(args)...)));
            return *this;
        }


        template <typename TField>
        ModelsIniParser& RegisterField(const std::string &key_name, class_field_t<TField> field) {
            return RegisterFieldCustomType<ModelsIniHelpers::FieldInfo<TModelInfo, TField>>(
                    key_name, field, false, TField{});
        }


        template <typename TField>
        ModelsIniParser& RegisterOptionalField(
                const std::string &key_name, class_field_t<TField> field,
                const TField &default_value = {}) {
            return RegisterFieldCustomType<ModelsIniHelpers::FieldInfo<TModelInfo, TField>>(
                    key_name, field, true, default_value);
        }

        // Handle cases, such as a string literal, where TDefault and TField are different types, but
        // TDefault can be converted to TField.
        template <typename TField, typename TDefault>
        ModelsIniParser& RegisterOptionalField(
                const std::string &key_name, class_field_t<TField> field,
                const TDefault &default_value = {}) {
            return RegisterFieldCustomType<ModelsIniHelpers::FieldInfo<TModelInfo, TField>>(
                    key_name, field, true, TField(default_value));
        }



        ModelsIniParser& RegisterPathField(const std::string &key_name, class_field_t<std::string> field) {
            return RegisterFieldCustomType<ModelsIniHelpers::PathFieldInfo<TModelInfo>>(
                    key_name, field, false, "");
        }


        ModelsIniParser& RegisterOptionalPathField(
                const std::string &key_name, class_field_t<std::string> field,  const std::string &default_value = "") {
            return RegisterFieldCustomType<ModelsIniHelpers::PathFieldInfo<TModelInfo>>(
                    key_name, field, true, default_value);
        }



        TModelInfo ParseIni(const std::string &model_name, const std::string &common_models_dir) const {
            std::string expanded_common_models_dir;
            std::string exp_error = Utils::expandFileName(common_models_dir, expanded_common_models_dir);
            if (!exp_error.empty()) {
                throw MPFDetectionException(
                        MPF_COULD_NOT_OPEN_DATAFILE,
                        "Failed to expand path to models directory which was \""
                            + common_models_dir + "\" due to: " + exp_error);
            }

            std::string models_ini_path = ModelsIniHelpers::GetFullPath(
                    "models.ini", plugin_models_dir_, expanded_common_models_dir);
            ModelsIniHelpers::IniHelper helper(models_ini_path, model_name);

            TModelInfo model_info;

            for (const auto& field_info : fields_) {
                field_info->SetField(helper, model_info, plugin_models_dir_, expanded_common_models_dir);
            }

            return model_info;
        }

    private:
        std::string plugin_models_dir_;

        std::vector<base_field_ptr_t> fields_;
    };
}



#endif //OPENMPF_CPP_COMPONENT_SDK_MODELSINIPARSER_H
