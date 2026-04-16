#############################################################################
# NOTICE                                                                    #
#                                                                           #
# This software (or technical data) was produced for the U.S. Government    #
# under contract, and is subject to the Rights in Data-General Clause       #
# 52.227-14, Alt. IV (DEC 2007).                                            #
#                                                                           #
# Copyright 2026 The MITRE Corporation. All Rights Reserved.                #
#############################################################################

#############################################################################
# Copyright 2026 The MITRE Corporation                                      #
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
project(openmpf-internal)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

set(SOURCE_FILES
        include/MPFBreaker.h
        src/MPFBreaker.cpp)

add_library(mpfInternalObjLib OBJECT ${SOURCE_FILES})

# Position-independent code is required because MPFBreaker uses a global variable that should be
# shared with any library linking to this one.
set_target_properties(mpfInternalObjLib PROPERTIES
        INTERFACE_POSITION_INDEPENDENT_CODE ON
        POSITION_INDEPENDENT_CODE ON)

add_library(mpfInternal SHARED $<TARGET_OBJECTS:mpfInternalObjLib>)

export_mpf_lib(mpfInternal)
