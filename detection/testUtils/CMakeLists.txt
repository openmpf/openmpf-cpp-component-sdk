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
project(openmpf-cpp-component-test-utils)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenCV 4.9.0 EXACT REQUIRED PATHS /opt/opencv-4.9.0
    COMPONENTS opencv_core opencv_imgcodecs opencv_imgproc opencv_videoio opencv_highgui)

include_directories(include)

set(SOURCE_FILES
        include/DetectionComparison.h
        src/DetectionComparison.cpp

        include/ImageGeneration.h
        src/ImageGeneration.cpp

        include/ReadDetectionsFromFile.h
        src/ReadDetectionsFromFile.cpp

        include/WriteDetectionsToFile.h
        src/WriteDetectionsToFile.cpp

        include/VideoGeneration.h
        src/VideoGeneration.cpp)

add_library(mpfComponentTestUtils SHARED ${SOURCE_FILES})
target_link_libraries(mpfComponentTestUtils ${OpenCV_LIBS})

export_mpf_lib(mpfComponentTestUtils)
