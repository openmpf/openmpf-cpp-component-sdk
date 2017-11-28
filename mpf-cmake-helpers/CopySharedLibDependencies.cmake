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

include(GetPrerequisites)

# List from http://refspecs.linuxfoundation.org/LSB_5.0.0/LSB-Common/LSB-Common/requirements.html#RLIBRARIES.
set(linux_std_libs
    libc
    libm
    libcrypt
    libdl
    libgcc_s
    libncurses
    libncursesw
    libnspr4
    libnss3
    libpam
    libpthread
    librt
    libssl3
    libstdcxx
    libstdc++
    libutil
    libz
)


function(is_std_linux_lib lib_path result)
    set(${result} FALSE PARENT_SCOPE)
    gp_file_type(${lib_path} ${lib_path} lib_type)
    if(${lib_type} STREQUAL system)
        get_filename_component(lib_file_name ${lib_path} NAME_WE)
        if(${lib_file_name} IN_LIST linux_std_libs)
            set(${result} TRUE PARENT_SCOPE)
        endif()
    endif()
endfunction()


# Can't use the built-in: get_filename_component(<VAR> <FileName> REALPATH)
# because when there is a link to a link to file it only returns the file
# and does not return the intermediate links.
function(resolve_single_link file result)
    execute_process(COMMAND readlink ${file}
        OUTPUT_VARIABLE readlink_output
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    if(IS_ABSOLUTE ${readlink_output})
        set(${result} ${readlink_output} PARENT_SCOPE)
    else()
        # When the link's target is relative, we must use the link's directory
        # to resolve the full path.
        get_filename_component(file_dir ${file} DIRECTORY)
        get_filename_component(target_absolute_path ${readlink_output}
            ABSOLUTE
            BASE_DIR ${file_dir})
        set(${result} ${target_absolute_path} PARENT_SCOPE)
    endif()
endfunction()



function(get_link_chain file result)
    set(link_chain ${file})
    while(IS_SYMLINK ${file})
        resolve_single_link(${file} link_target)
        set(file ${link_target})
        list(APPEND link_chain ${file})
    endwhile()
    set(${result} ${link_chain} PARENT_SCOPE)
endfunction()



separate_arguments(EXTRA_LIB_DIRS)

message(STATUS "Copying dependent libraries to ${DEP_LIBS_INSTALL_LOCATION}")
# get_prerequisites(<target> <prerequisites_var> <exclude_system> <recurse> <exepath> <dirs> [<rpaths>])
get_prerequisites(${TARGET_BINARY_LOCATION} DEPENDENCIES 0 0 "" "${EXTRA_LIB_DIRS}")

foreach(dependency_file_relative ${DEPENDENCIES})
    # gp_resolve_item(<context> <item> <exepath> <dirs> <resolved_item_var> [<rpaths>])
    gp_resolve_item("${TARGET_BINARY_LOCATION}" "${dependency_file_relative}" "" "" dependency_abs_path)

    is_std_linux_lib(${dependency_abs_path} is_std_lib)
    if (NOT ${is_std_lib})
        get_link_chain(${dependency_abs_path} link_chain)
        # Reverse the chain so that the links' target is copied before the link itself.
        list(REVERSE link_chain)
        foreach(lib_file ${link_chain})
            file(COPY ${lib_file} DESTINATION ${DEP_LIBS_INSTALL_LOCATION})
        endforeach()
    endif()
endforeach()


