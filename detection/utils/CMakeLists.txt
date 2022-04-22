#############################################################################
# NOTICE                                                                    #
#                                                                           #
# This software (or technical data) was produced for the U.S. Government    #
# under contract, and is subject to the Rights in Data-General Clause       #
# 52.227-14, Alt. IV (DEC 2007).                                            #
#                                                                           #
# Copyright 2021 The MITRE Corporation. All Rights Reserved.                #
#############################################################################

#############################################################################
# Copyright 2021 The MITRE Corporation                                      #
#                                                                           #
# Licensed under the Apache License, Version 2.0 (the "License");           #
# you may not use this file except in compliance with the License.          #
# You may obtain a copy of the License at                                   #
#                                                                           #
#    http://www.apache.org/licenses/LICENSE-2.0                             #
#                                                                           #
# Unless required by applicable law or agreed to in writing, software       #
# distributed under the License is distributed on an "AS IS" BASIS,         #
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  #
# See the License for the specific language governing permissions and       #
# limitations under the License.                                            #
#############################################################################

cmake_minimum_required(VERSION 3.6)
project(openmpf-cpp-component-utils)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV 4.5.5 EXACT REQUIRED PATHS /opt/opencv-4.5.5
    COMPONENTS opencv_core opencv_imgcodecs opencv_imgproc opencv_videoio)

find_package(PkgConfig)
pkg_search_module(LOG4CXX REQUIRED liblog4cxx)

find_package(Qt4 REQUIRED)


include_directories(include)

set(SOURCE_FILES
        include/Utils.h
        src/Utils.cpp

        include/MPFSimpleConfigLoader.h
        src/MPFSimpleConfigLoader.cpp

        include/ModelsIniParser.h
        src/ModelsIniParser.cpp

        include/DlClassLoader.h
    )

add_library(mpfComponentUtils SHARED ${SOURCE_FILES})
target_link_libraries(mpfComponentUtils ${OpenCV_LIBS} ${LOG4CXX_LIBRARIES} Qt4::QtCore dl)


export_mpf_lib(mpfComponentUtils)

add_subdirectory(test)
