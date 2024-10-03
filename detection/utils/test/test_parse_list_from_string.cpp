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
 * Please see the accompanying LICENSE file.                                  *
 ******************************************************************************/

#include <gtest/gtest.h>
#include <log4cxx/basicconfigurator.h>

#include <Utils.h>


using namespace std;
using namespace MPF;
using namespace COMPONENT;

bool init_logging() {
    log4cxx::BasicConfigurator::configure();
    return true;
}
bool logging_initialized = init_logging();

TEST(ParseListFromString, ParseEmptyList) {
    string test_string;
    vector<string> result = Utils::ParseListFromString(test_string);
    ASSERT_TRUE(result.empty());

}

TEST(ParseListFromString, ParseListWithSingleString) {
    string test_string("Hello");
    vector<string> result = Utils::ParseListFromString(test_string);
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0] == test_string);

}

TEST(ParseListFromString, ParseDelimitedList) {
    string test_string("Hey;Hello;World");
    vector<string> result = Utils::ParseListFromString(test_string);
    ASSERT_EQ(result.size(), 3);
    ASSERT_TRUE(result[0] == "Hey");
    ASSERT_TRUE(result[1] == "Hello");
    ASSERT_TRUE(result[2] == "World");


}

TEST(ParseListFromString, ParseListWithEscapedDelimiter) {
    string test_string("Hey;Hello\\;World");
    vector<string> result = Utils::ParseListFromString(test_string);
    ASSERT_EQ(result.size(), 2);
    ASSERT_TRUE(result[0] == "Hey");
    ASSERT_TRUE(result[1] == "Hello;World");

}

TEST(ParseListFromString, ParseListWithSingleBackslash) {
    string test_string("Hey;Hello\;World");
    vector<string> result = Utils::ParseListFromString(test_string);
    ASSERT_EQ(result.size(), 3);
    ASSERT_TRUE(result[0] == "Hey");
    ASSERT_TRUE(result[1] == "Hello");
    ASSERT_TRUE(result[2] == "World");

}

TEST(ParseListFromString, ParseListWithExtraBackslash) {
    string test_string("Hello\\\;World");
    vector<string> result = Utils::ParseListFromString(test_string);
    ASSERT_EQ(result.size(), 1);
    ASSERT_TRUE(result[0] == "Hello;World");

}

TEST(ParseListFromString, ParseListWithExtraDelimiter) {
    string test_string("Hello;;World");
    vector<string> result = Utils::ParseListFromString(test_string);
    ASSERT_EQ(result.size(), 2);
    ASSERT_TRUE(result[0] == "Hello");
    ASSERT_TRUE(result[1] == "World");

}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
