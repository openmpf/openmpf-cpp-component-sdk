#############################################################################
# NOTICE                                                                    #
#                                                                           #
# This software (or technical data) was produced for the U.S. Government    #
# under contract, and is subject to the Rights in Data-General Clause       #
# 52.227-14, Alt. IV (DEC 2007).                                            #
#                                                                           #
# Copyright 2023 The MITRE Corporation. All Rights Reserved.                #
#############################################################################

#############################################################################
# Copyright 2023 The MITRE Corporation                                      #
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
project(hello-world-component)

set(CMAKE_CXX_STANDARD 17)

include(../ComponentSetup.cmake)

find_package(mpfComponentInterface REQUIRED)
find_package(mpfDetectionComponentApi REQUIRED)
find_package(OpenCV 4.9.0 EXACT REQUIRED PATHS /opt/opencv-4.9.0 COMPONENTS opencv_core)
find_package(PkgConfig)
pkg_search_module(LOG4CXX REQUIRED liblog4cxx)


set(HELLO_WORLD_COMPONENT_SOURCES HelloWorld.cpp HelloWorld.h)

add_library(mpfHelloWorld SHARED ${HELLO_WORLD_COMPONENT_SOURCES})
target_link_libraries(mpfHelloWorld mpfDetectionComponentApi ${LOG4CXX_LIBRARIES})

add_library(mpfStreamingHelloWorld SHARED StreamingHelloWorld.cpp StreamingHelloWorld.h)
target_link_libraries(mpfStreamingHelloWorld mpfDetectionComponentApi ${LOG4CXX_LIBRARIES})

configure_mpf_component(HelloWorldComponent TARGETS mpfHelloWorld mpfStreamingHelloWorld)


add_executable(sample_hello_world_detector sample_hello_world_detector.cpp)
target_link_libraries(sample_hello_world_detector mpfHelloWorld)

