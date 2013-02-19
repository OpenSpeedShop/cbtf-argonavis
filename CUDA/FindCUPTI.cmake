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

find_library(CUPTI_LIBRARY NAMES libcupti.so 
    HINTS $ENV{CUDA_INSTALL_PATH}/extras/CUPTI $ENV{CUPTI_ROOT}
    PATH_SUFFIXES lib lib64
    )

find_path(CUDA_INCLUDE_DIR cuda.h
    HINTS $ENV{CUDA_INSTALL_PATH} $ENV{CUDA_ROOT}
    PATH_SUFFIXES include
    )

find_path(CUPTI_INCLUDE_DIR cupti.h
    HINTS $ENV{CUDA_INSTALL_PATH}/extras/CUPTI $ENV{CUPTI_ROOT}
    PATH_SUFFIXES include
    )

find_package_handle_standard_args(
    CUPTI DEFAULT_MSG CUPTI_LIBRARY CUDA_INCLUDE_DIR CUPTI_INCLUDE_DIR
    )

set(CUPTI_LIBRARIES ${CUPTI_LIBRARY})
set(CUPTI_INCLUDE_DIRS ${CUDA_INCLUDE_DIR} ${CUPTI_INCLUDE_DIR})

mark_as_advanced(CUPTI_LIBRARY CUDA_INCLUDE_DIR CUPTI_INCLUDE_DIR)

if(CUPTI_FOUND AND DEFINED CUPTI_INCLUDE_DIR)
  
    file(READ
        ${CUPTI_INCLUDE_DIR}/cupti_version.h
        CUPTI_VERSION_FILE
        )
  
    string(REGEX REPLACE
        ".*#define CUPTI_API_VERSION[ \t]+([0-9]+)\n.*" "\\1"
        CUPTI_API_VERSION ${CUPTI_VERSION_FILE}
        )
      
    message(STATUS "CUPTI API version: " ${CUPTI_API_VERSION})

    if(DEFINED CUPTI_FIND_VERSION)
        if(${CUPTI_API_VERSION} VERSION_LESS ${CUPTI_FIND_VERSION})

            set(CUPTI_FOUND FALSE)

            if(DEFINED CUPTI_FIND_REQUIRED)
                message(FATAL_ERROR
                    "Could NOT find CUPTI  (version < "
                    ${CUPTI_FIND_VERSION} ")"
                    )
            else()
                message(STATUS
                    "Could NOT find CUPTI  (version < " 
                    ${CUPTI_FIND_VERSION} ")"
                    )
            endif()
 
        endif()
    endif()
  
endif()
