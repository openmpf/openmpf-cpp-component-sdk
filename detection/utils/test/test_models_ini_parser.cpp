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

#include <cstdlib>
#include <fstream>
#include <vector>

#include <gtest/gtest.h>

#include "ModelsIniParser.h"

using namespace MPF::COMPONENT;

struct ModelSettings {
    std::string string_field;
    std::string path_field;
    int int_field;
};

const char * ini_content = R"(
[test model]
string_field=hello world
int_field = 567
path_field=test_file.txt

[other model]
string_field=other
int_field = 765
path_field=other_file.txt

[empty path model]
string_field=hello world
int_field=5
path_fieldMPFDetectionException=

[missing fields model]
string_field=hello world
)";


void write_to_file(const std::string &file_path, const std::string& content) {
    std::ofstream fp(file_path);
    fp << content;
    if (!fp.good()) {
        throw std::runtime_error("Failed to write content to file at: " + file_path);
    }
}

class ModelsIniParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        char test_dir[] = "/tmp/mpf_testXXXXXX";
        test_dir_ = mkdtemp(test_dir);
        mkdir((test_dir_ + "/plugin").c_str(), 0777);

        plugins_model_dir_ = test_dir_ + "/plugin/models";
        mkdir(plugins_model_dir_.c_str(), 0777);
        write_to_file(plugins_model_dir_ + "/models.ini", ini_content);

        mkdir((test_dir_ + "/common").c_str(), 0777);
        common_models_dir_ = test_dir_ + "/common/test_component";
        mkdir(common_models_dir_.c_str(), 0777);

        models_parser_.Init(plugins_model_dir_)
                .RegisterField("string_field", &ModelSettings::string_field)
                .RegisterField("int_field", &ModelSettings::int_field)
                .RegisterPathField("path_field", &ModelSettings::path_field);

    }

    void TearDown() override {
        system(("rm -rf " + test_dir_).c_str());
    }


    std::string test_dir_;
    std::string plugins_model_dir_;
    std::string common_models_dir_;

    ModelsIniParser<ModelSettings> models_parser_;
};


TEST_F(ModelsIniParserTest, CanLoadFromPluginDir) {
    auto test_file_path = plugins_model_dir_ + "/test_file.txt";
    write_to_file(test_file_path, "test");

    auto model_settings = models_parser_.ParseIni("test model", common_models_dir_);
    ASSERT_EQ("hello world", model_settings.string_field);
    ASSERT_EQ(567, model_settings.int_field);
    ASSERT_EQ(test_file_path, model_settings.path_field);
}


TEST_F(ModelsIniParserTest, CanLoadFromCommonDir) {
    auto test_file_path = common_models_dir_ + "/test_file.txt";
    write_to_file(test_file_path, "test");

    auto model_settings = models_parser_.ParseIni("test model", common_models_dir_);
    ASSERT_EQ("hello world", model_settings.string_field);
    ASSERT_EQ(567, model_settings.int_field);
    ASSERT_EQ(test_file_path, model_settings.path_field);
}


TEST_F(ModelsIniParserTest, CanLoadDifferentModelsWithSameParser) {
    auto test_file_path = plugins_model_dir_ + "/test_file.txt";
    write_to_file(test_file_path, "test");

    auto other_file_path = plugins_model_dir_ + "/other_file.txt";
    write_to_file(other_file_path, "test");

    auto test_model = models_parser_.ParseIni("test model", common_models_dir_);
    ASSERT_EQ("hello world", test_model.string_field);
    ASSERT_EQ(567, test_model.int_field);
    ASSERT_EQ(test_file_path, test_model.path_field);

    auto other_model = models_parser_.ParseIni("other model", common_models_dir_);
    ASSERT_EQ("other", other_model.string_field);
    ASSERT_EQ(765, other_model.int_field);
    ASSERT_EQ(other_file_path, other_model.path_field);
}


TEST_F(ModelsIniParserTest, PrefersCommonModelsDir) {
    const char * common_model_ini_content = R"(
[test model]
string_field=string content
int_field = 123
path_field=other_file.txt
)";
    write_to_file(common_models_dir_ + "/models.ini", common_model_ini_content);
    auto test_file_plugin_path = plugins_model_dir_ + "/other_file.txt";
    write_to_file(test_file_plugin_path, "test");

    auto test_file_common_path = common_models_dir_ + "/other_file.txt";
    write_to_file(test_file_common_path, "test");

    auto model_settings = models_parser_.ParseIni("test model", common_models_dir_);
    ASSERT_EQ("string content", model_settings.string_field);
    ASSERT_EQ(123, model_settings.int_field);
    ASSERT_EQ(test_file_common_path, model_settings.path_field);
}

TEST_F(ModelsIniParserTest, ThrowsWhenFileMissing) {
    ASSERT_THROW(models_parser_.ParseIni("test model", common_models_dir_), MPFDetectionException);
}


TEST_F(ModelsIniParserTest, ThrowsOnUnknownModel) {
    ASSERT_THROW(models_parser_.ParseIni("not a model", common_models_dir_), MPFDetectionException);
}


TEST_F(ModelsIniParserTest, ThrowsWhenEmptyPath) {
    ASSERT_THROW(models_parser_.ParseIni("empty path model", common_models_dir_), MPFDetectionException);
}

TEST_F(ModelsIniParserTest, ThrowsWhenMissingRequiredFields) {
    ASSERT_THROW(models_parser_.ParseIni("missing fields model", common_models_dir_), MPFDetectionException);
}


TEST_F(ModelsIniParserTest, UsesValuesFromIniFileWhenFieldIsOptionalAndInIniFile) {
    auto test_file_path = plugins_model_dir_ + "/test_file.txt";
    write_to_file(test_file_path, "test");

    auto model_settings = ModelsIniParser<ModelSettings>().Init(plugins_model_dir_)
            .RegisterOptionalField("string_field", &ModelSettings::string_field)
            .RegisterOptionalField("int_field", &ModelSettings::int_field, 100)
            .RegisterOptionalPathField("path_field", &ModelSettings::path_field)
            .ParseIni("test model", common_models_dir_);

    ASSERT_EQ("hello world", model_settings.string_field);
    ASSERT_EQ(567, model_settings.int_field);
    ASSERT_EQ(test_file_path, model_settings.path_field);
}


TEST_F(ModelsIniParserTest, CanHandleMissingFieldsWhenOptional) {
    auto model_settings = ModelsIniParser<ModelSettings>().Init(plugins_model_dir_)
        .RegisterOptionalField("string_field", &ModelSettings::string_field)
        .RegisterOptionalField("int_field", &ModelSettings::int_field, 100)
        .RegisterOptionalPathField("path_field", &ModelSettings::path_field)
        .ParseIni("missing fields model", common_models_dir_);

    ASSERT_EQ("hello world", model_settings.string_field);
    ASSERT_EQ(100, model_settings.int_field);
    ASSERT_EQ("", model_settings.path_field);
}



TEST_F(ModelsIniParserTest, ValidatesDefaultValueForOptionalPath) {
    ModelsIniParser<ModelSettings> parser;
    parser.Init(plugins_model_dir_)
            .RegisterOptionalField("string_field", &ModelSettings::string_field, "asdf")
            .RegisterOptionalField("int_field", &ModelSettings::int_field, 100)
            .RegisterOptionalPathField("path_field", &ModelSettings::path_field, "test_file.txt");

    ASSERT_THROW(parser.ParseIni("missing fields model", common_models_dir_), MPFDetectionException);


    auto test_file_path = plugins_model_dir_ + "/test_file.txt";
    write_to_file(test_file_path, "test");

    auto model_settings = parser.ParseIni("missing fields model", common_models_dir_);
    ASSERT_EQ("hello world", model_settings.string_field);
    ASSERT_EQ(100, model_settings.int_field);
    ASSERT_EQ(test_file_path, model_settings.path_field);
}


TEST_F(ModelsIniParserTest, ThrowsWhenTypeConversionFails) {
    ModelsIniParser<ModelSettings> parser;
    parser.Init(plugins_model_dir_)
            .RegisterField("string_field", &ModelSettings::int_field);

    ASSERT_THROW(parser.ParseIni("test model", common_models_dir_), MPFDetectionException);
}
