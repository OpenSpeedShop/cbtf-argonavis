################################################################################
# Copyright (c) 2012 Argo Navis Technologies. All Rights Reserved.
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

find_library(PAPI_LIBRARY NAMES libpapi.so
    HINTS $ENV{PAPI_ROOT}
    PATH_SUFFIXES lib lib64
    )

find_path(PAPI_INCLUDE_DIR papi.h
    HINTS $ENV{PAPI_ROOT}
    PATH_SUFFIXES include
    )

find_package_handle_standard_args(
    PAPI DEFAULT_MSG PAPI_LIBRARY PAPI_INCLUDE_DIR
    )

set(PAPI_LIBRARIES ${PAPI_LIBRARY})
set(PAPI_INCLUDE_DIRS ${PAPI_INCLUDE_DIR})

mark_as_advanced(PAPI_LIBRARY PAPI_INCLUDE_DIR)

if(PAPI_FOUND AND DEFINED PAPI_INCLUDE_DIR)
  
    file(READ
        ${PAPI_INCLUDE_DIR}/papi.h
        PAPI_VERSION_FILE
        )

    string(REGEX REPLACE
        ".*#define PAPI_VERSION[ \t]+PAPI_VERSION_NUMBER[(]([0-9]+),.*"
        "\\1" PAPI_VERSION_MAJOR ${PAPI_VERSION_FILE}
        )

    string(REGEX REPLACE
        ".*#define PAPI_VERSION[ \t]+PAPI_VERSION_NUMBER[(][0-9]+,([0-9]+),.*"
        "\\1" PAPI_VERSION_MINOR ${PAPI_VERSION_FILE}
        )

    string(REGEX REPLACE
   ".*#define PAPI_VERSION[ \t]+PAPI_VERSION_NUMBER[(][0-9]+,[0-9]+,([0-9]+),.*"
        "\\1" PAPI_VERSION_PATCH ${PAPI_VERSION_FILE}
        )

    set(PAPI_VERSION_STRING
        ${PAPI_VERSION_MAJOR}.${PAPI_VERSION_MINOR}.${PAPI_VERSION_PATCH}
        )
     
    message(STATUS "PAPI version: " ${PAPI_VERSION_STRING})

    if(DEFINED PAPI_FIND_VERSION)
        if(${PAPI_VERSION_STRING} VERSION_LESS ${PAPI_FIND_VERSION})

            set(PAPI_FOUND FALSE)

            if(DEFINED PAPI_FIND_REQUIRED)
                message(FATAL_ERROR
                    "Could NOT find PAPI  (version < "
                    ${PAPI_FIND_VERSION} ")"
                    )
            else()
                message(STATUS
                    "Could NOT find PAPI  (version < " 
                    ${PAPI_FIND_VERSION} ")"
                    )
            endif()
 
        endif()
    endif()
  
endif()
