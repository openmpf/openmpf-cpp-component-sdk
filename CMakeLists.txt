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
project(openmpf-cpp-component-sdk)

set(CMAKE_CXX_STANDARD 11)

# Configure install location for SDK libs
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT) # prefix path not provided on command line
    if(DEFINED ENV{MPF_SDK_INSTALL_PATH})
        set(CMAKE_INSTALL_PREFIX "$ENV{MPF_SDK_INSTALL_PATH}" CACHE PATH "..." FORCE)
    else()
        set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/mpf-sdk-install" CACHE PATH "..." FORCE)
    endif()
endif()


function(export_mpf_lib targetName)
    install(TARGETS ${targetName} EXPORT ${targetName}Config
            ARCHIVE  DESTINATION lib
            LIBRARY  DESTINATION lib
            RUNTIME  DESTINATION bin
            INCLUDES DESTINATION include
            )

    install(DIRECTORY include/ DESTINATION include)

    # Generate CMake config file that will install ${targetName}
    export(TARGETS ${targetName} FILE ${targetName}Config.cmake EXPORT_LINK_INTERFACE_LIBRARIES)

    # Make it so ${targetName} can be found with find_package
    export(PACKAGE ${targetName})

    # Copy CMake config file to install location so dependencies can find it.
    install(EXPORT ${targetName}Config DESTINATION lib/cmake/${targetName} EXPORT_LINK_INTERFACE_LIBRARIES)
endfunction()

add_subdirectory(interface)
include_directories(interface/include)

add_subdirectory(detection/api)
include_directories(detection/api/include)

add_subdirectory(detection/utils)
include_directories(detection/utils/include)

add_subdirectory(detection/testUtils)
include_directories(detection/testUtils/include)

add_subdirectory(mpf-cmake-helpers)