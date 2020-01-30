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


function(configure_mpf_component pluginName)
    cmake_parse_arguments(kw_args "" "" "TARGETS;EXTRA_LIB_PATHS" ${ARGN})
    if (NOT DEFINED kw_args_TARGETS)
        message(FATAL_ERROR "One or more targets must follow the \"TARGETS\" keyword")
    endif()

    if (DEFINED kw_args_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unexpected arguments: ${kw_args_UNPARSED_ARGUMENTS}")
    endif()

    get_plugin_build_location(${pluginName} pluginLocation)
    foreach (targetName ${kw_args_TARGETS})
        copy_shared_libs(${targetName} ${pluginLocation} "${kw_args_EXTRA_LIB_PATHS}")
        configure_mpf_component_install(${targetName} ${pluginLocation})
    endforeach()

    tar_mpf_component(${pluginName} ${pluginLocation})
endfunction()




function(get_plugin_build_location pluginName result)
    set(${result} ${CMAKE_CURRENT_BINARY_DIR}/plugin/${pluginName} PARENT_SCOPE)
endfunction()


# Add MPF SDK install location to CMAKE_PREFIX_PATH so find_library can find the MPF SDK libs
macro(configure_mpf_sdk_install_location)
    if(DEFINED ENV{MPF_SDK_INSTALL_PATH})
        list(APPEND CMAKE_PREFIX_PATH $ENV{MPF_SDK_INSTALL_PATH}/lib/cmake)
    endif()

    list(APPEND CMAKE_PREFIX_PATH $ENV{HOME}/mpf-sdk-install/lib/cmake)
endmacro()


# Add post-build command that copies all referenced libraries to the plugin's lib directory
function(copy_shared_libs targetName pluginLocation)
    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        if (targetName STREQUAL darknet_wrapper)
            message("!!! enabling debug for copy_shared_libs")
            add_custom_command(TARGET ${targetName} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -D TARGET_BINARY_LOCATION="$<TARGET_FILE:${targetName}>"
                -D DEP_LIBS_INSTALL_LOCATION="${pluginLocation}/lib"
                -D EXTRA_LIB_DIRS="${ARGV2}"
                -D MPF_DEBUG_ENABLED=true
                -P ${CopySharedLibDependencies_LOCATION})
        else()
            add_custom_command(TARGET ${targetName} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -D TARGET_BINARY_LOCATION="$<TARGET_FILE:${targetName}>"
                -D DEP_LIBS_INSTALL_LOCATION="${pluginLocation}/lib"
                -D EXTRA_LIB_DIRS="${ARGV2}"
                -P ${CopySharedLibDependencies_LOCATION})
        endif()
    endif()
endfunction()


# Setup install rules that moves project files in to the plugin's directory
function(configure_mpf_component_install targetName pluginLocation)
    copy_plugin_files_dir(${pluginLocation})

    # Copy the libraries created by the current project in to the plugin's lib directory
    install(TARGETS ${targetName}
        ARCHIVE  DESTINATION ${pluginLocation}/lib
        LIBRARY  DESTINATION ${pluginLocation}/lib)
endfunction()


function(copy_plugin_files_dir pluginLocation)
    file(COPY plugin-files/ DESTINATION ${pluginLocation})
endfunction()

# Setup install rule that creates a tar.gz file containing the content of the plugin's directory.
function(tar_mpf_component pluginName pluginLocation)
    install(
        CODE "file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/plugin-packages)"
        CODE "message(STATUS \"Creating ${pluginName} plugin package\")"
        CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E tar cfz \"${CMAKE_BINARY_DIR}/plugin-packages/${pluginName}.tar.gz\" \"${pluginLocation}\" WORKING_DIRECTORY \"${CMAKE_CURRENT_BINARY_DIR}/plugin\")"
        CODE "message(STATUS \"${pluginName} plugin package created at ${CMAKE_BINARY_DIR}/plugin-packages/${pluginName}.tar.gz\")"
    )
endfunction()
