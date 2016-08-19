################################################################################
# Copyright (c) 2016 Argo Navis Technologies. All Rights Reserved.
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

find_library(CUDA_LIBRARY NAMES libcuda.so
    HINTS $ENV{CUDA_INSTALL_PATH} $ENV{CUDA_DIR}
    HINTS ${CUDA_INSTALL_PATH} ${CUDA_DIR}
    PATH_SUFFIXES lib lib64 lib64/stubs
    )

find_path(CUDA_INCLUDE_DIR cuda.h
    HINTS $ENV{CUDA_INSTALL_PATH} $ENV{CUDA_DIR}
    HINTS ${CUDA_INSTALL_PATH} ${CUDA_DIR}
    PATH_SUFFIXES include
    )

find_package_handle_standard_args(
    CUDA DEFAULT_MSG CUDA_LIBRARY CUDA_INCLUDE_DIR
    )

set(CUDA_LIBRARIES ${CUDA_LIBRARY})
set(CUDA_INCLUDE_DIRS ${CUDA_INCLUDE_DIR})

mark_as_advanced(CUDA_LIBRARY CUDA_INCLUDE_DIR)

if(CUDA_FOUND AND DEFINED CUDA_INCLUDE_DIR)
  
    file(READ ${CUDA_INCLUDE_DIR}/cuda.h CUDA_VERSION_FILE)
  
    string(REGEX REPLACE
        ".*#define CUDA_VERSION[ \t]+([0-9]+)\n.*" "\\1"
        CUDA_VERSION ${CUDA_VERSION_FILE}
        )
      
    message(STATUS "CUDA version: " ${CUDA_VERSION})

    if(DEFINED CUDA_FIND_VERSION)
        if(${CUDA_VERSION} VERSION_LESS ${CUDA_FIND_VERSION})

            set(CUDA_FOUND FALSE)

            if(DEFINED CUDA_FIND_REQUIRED)
                message(FATAL_ERROR
                    "Could NOT find CUDA  (version < "
                    ${CUDA_FIND_VERSION} ")"
                    )
            else()
                message(STATUS
                    "Could NOT find DA  (version < " 
                    ${CUDA_FIND_VERSION} ")"
                    )
            endif()
 
        endif()
    endif()
  
endif()
