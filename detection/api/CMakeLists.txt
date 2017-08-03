#############################################################################
# NOTICE                                                                    #
#                                                                           #
# This software (or technical data) was produced for the U.S. Government    #
# under contract, and is subject to the Rights in Data-General Clause       #
# 52.227-14, Alt. IV (DEC 2007).                                            #
#                                                                           #
# Copyright 2017 The MITRE Corporation. All Rights Reserved.                #
#############################################################################

#############################################################################
# Copyright 2017 The MITRE Corporation                                      #
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
project(openmpf-cpp-component-api)

set(CMAKE_CXX_STANDARD 11)

find_package(OpenCV 3.2 EXACT REQUIRED PATHS /opt/opencv-3.2.0
    COMPONENTS opencv_core opencv_imgcodecs opencv_imgproc opencv_videoio)


include_directories(include)
set(SOURCE_FILES
        include/MPFDetectionComponent.h

        include/detectionComponentUtils.h
        src/detectionComponentUtils.cpp

        include/MPFInvalidPropertyException.h
        src/MPFInvalidPropertyException.cpp

        include/MPFImageReader.h
        src/MPFImageReader.cpp

        include/MPFVideoCapture.h
        src/MPFVideoCapture.cpp

        include/VideoSegmentToFramesConverter.h
        src/VideoSegmentToFramesConverter.cpp


        include/frame_transformers/IFrameTransformer.h

        include/frame_transformers/BaseDecoratedTransformer.h
        src/frame_transformers/BaseDecoratedTransformer.cpp

        include/frame_transformers/FrameCropper.h
        src/frame_transformers/FrameCropper.cpp

        include/frame_transformers/FrameFlipper.h
        src/frame_transformers/FrameFlipper.cpp

        include/frame_transformers/FrameRotator.h
        src/frame_transformers/FrameRotator.cpp

        include/frame_transformers/NoOpFrameTransformer.h
        src/frame_transformers/NoOpFrameTransformer.cpp

        include/frame_transformers/FrameTransformerFactory.h
        src/frame_transformers/FrameTransformerFactory.cpp

        include/adapters/MPFAudioAndVideoDetectionComponentAdapter.h
        include/adapters/MPFAudioDetectionComponentAdapter.h
        include/adapters/MPFImageAndVideoDetectionComponentAdapter.h
        include/adapters/MPFImageDetectionComponentAdapter.h
        include/adapters/MPFVideoDetectionComponentAdapter.h
        include/adapters/MPFVideoFrameDataDetectionAdapter.h)

add_library(mpfDetectionComponentApi SHARED ${SOURCE_FILES})
target_link_libraries(mpfDetectionComponentApi mpfComponentInterface ${OpenCV_LIBS})



export_mpf_lib(mpfDetectionComponentApi)
