################################################################################
# Copyright (c) 2017 Krell Institute. All Rights Reserved.
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 59 Temple
# Place, Suite 330, Boston, MA  02111-1307  USA
################################################################################

include(FindPackageHandleStandardArgs)

find_library(Dyninst_LIBRARY NAMES libdyninstAPI.so
    HINTS $ENV{DYNINST_DIR}
    HINTS ${DYNINST_DIR}
    PATH_SUFFIXES lib lib64
    )

find_path(Dyninst_INCLUDE_DIR BPatch.h
    HINTS $ENV{DYNINST_DIR}
    HINTS ${DYNINST_DIR}
    PATH_SUFFIXES include include/dyninst
    )

find_package_handle_standard_args(
    Dyninst DEFAULT_MSG Dyninst_LIBRARY Dyninst_INCLUDE_DIR
    )

set(Dyninst_LIBRARIES ${Dyninst_LIBRARY})
set(Dyninst_INCLUDE_DIRS ${Dyninst_INCLUDE_DIR})

if(Dyninst_FOUND AND DEFINED Dyninst_INCLUDE_DIR)

    file(READ ${Dyninst_INCLUDE_DIR}/version.h Dyninst_VERSION_FILE)

    string(REGEX REPLACE
        ".*#[ ]*define DYNINST_MAJOR_VERSION[ ]+([0-9]+)\n.*" "\\1"
        Dyninst_VERSION_MAJOR ${Dyninst_VERSION_FILE}
        )

    string(REGEX REPLACE
        ".*#[ ]*define DYNINST_MINOR_VERSION[ ]+([0-9]+)\n.*" "\\1"
        Dyninst_VERSION_MINOR ${Dyninst_VERSION_FILE}
        )

    string(REGEX REPLACE
        ".*#[ ]*define DYNINST_PATCH_VERSION[ ]+([0-9]+)\n.*" "\\1"
        Dyninst_VERSION_PATCH ${Dyninst_VERSION_FILE}
        )

    set(Dyninst_VERSION
${Dyninst_VERSION_MAJOR}.${Dyninst_VERSION_MINOR}.${Dyninst_VERSION_PATCH}
        )
  
    message(STATUS "Dyninst version: " ${Dyninst_VERSION})

    if(DEFINED Dyninst_FIND_VERSION)
        if(${Dyninst_VERSION} VERSION_LESS ${Dyninst_FIND_VERSION})

            set(Dyninst_FOUND FALSE)

            if(DEFINED Dyninst_FIND_REQUIRED)
                message(FATAL_ERROR
                    "Could NOT find Dyninst  (version < "
                    ${Dyninst_FIND_VERSION} ")"
                    )
            else()
                message(STATUS
                    "Could NOT find Dyninst  (version < " 
                    ${Dyninst_FIND_VERSION} ")"
                    )
            endif()
 
        endif()
    endif()

endif()
