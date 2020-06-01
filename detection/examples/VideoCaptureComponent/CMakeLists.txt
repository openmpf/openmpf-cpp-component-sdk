#############################################################################
# NOTICE                                                                    #
#                                                                           #
# This software (or technical data) was produced for the U.S. Government    #
# under contract, and is subject to the Rights in Data-General Clause       #
# 52.227-14, Alt. IV (DEC 2007).                                            #
#                                                                           #
# Copyright 2019 The MITRE Corporation. All Rights Reserved.                #
#############################################################################

#############################################################################
# Copyright 2019 The MITRE Corporation                                      #
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
project(video-capture-component)

set(CMAKE_CXX_STANDARD 11)

include(../ComponentSetup.cmake)

find_package(mpfComponentInterface REQUIRED)
find_package(mpfDetectionComponentApi REQUIRED)
find_package(OpenCV $ENV{OPENCV_VERSION} EXACT REQUIRED PATHS /opt/opencv-$ENV{OPENCV_VERSION} COMPONENTS opencv_core opencv_highgui)


set(VIDEO_CAPTURE_COMPONENT_SOURCES VideoCaptureComponent.h VideoCaptureComponent.cpp)

add_library(mpfVideoCapture SHARED ${VIDEO_CAPTURE_COMPONENT_SOURCES})
target_link_libraries(mpfVideoCapture mpfDetectionComponentApi ${OpenCV_LIBS})

configure_mpf_component(VideoCaptureComponent TARGETS mpfVideoCapture)


add_executable(sample_video_capture_component sample_video_capture_component.cpp)
target_link_libraries(sample_video_capture_component mpfVideoCapture)

