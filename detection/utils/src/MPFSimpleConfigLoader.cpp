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


#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QIODevice>
#include <QtCore/QStringList>

#include "MPFSimpleConfigLoader.h"


namespace MPF { namespace COMPONENT {

    int LoadConfig(const std::string &config_path, QHash<QString, QString> &parameters) {
        QFile file(QString::fromStdString(config_path));

        parameters.clear();

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            printf("ERROR: Config file not loaded.\n");
            return (-1);
        }

        QByteArray line = file.readLine();
        while (line.count() > 0) {
            QStringList list = QString(line).left(line.indexOf('#')).split(": ");
            if (list.count() == 2) {
                parameters.insert(list[0].simplified(), list[1].simplified());
            }
            line = file.readLine();
        }
        file.close();
        return (0);
    };


//    namespace {
//        std::string trim(const std::string &input) {
//            std::istringstream ss(input);
//            std::string result;
//            ss >> result;
//            return result;
//        }
//
//        std::string stripComments(const std::string &input) {
//            auto commentPos = input.find_first_of('#');
//            return input.substr(0, commentPos);
//        }
//    }


//    int LoadConfig(const std::string &config_path, std::map<std::string, std::string> &parameters) {
//        std::ifstream file(config_path);
//        parameters.clear();
//
//        if (!file.is_open()) {
//            printf("ERROR: Config file not loaded.\n");
//            return -1;
//
//        }
//
//        std::string line;
//        while (std::getline(file, line)) {
//            line = stripComments(line);
//            std::istringstream ss(line);
//            std::string key;
//            if (!std::getline(ss, key, ':')) {
//                continue;
//            };
//            key = trim(key);
//
//            std::string val;
//            if (!std::getline(ss, val, ':')) {
//                continue;
//            }
//            val = trim(val);
//
//            parameters[key] = val;
//        }
//        return 0;
//    }
}}