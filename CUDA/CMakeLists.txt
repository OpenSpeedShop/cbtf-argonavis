################################################################################
# Copyright (c) 2012-2017 Argo Navis Technologies. All Rights Reserved.
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

add_subdirectory(libargonavis-cuda)
add_subdirectory(messages)

if(MRNET_FOUND AND PLATFORM_BUILD_COLLECTOR)

    if(BUILD_CUDA_REPLAY_COLLECTOR)
        message(STATUS "Building the CBTF/MRNet CUDA replay collector")
        add_subdirectory(replay)
    else()
        add_subdirectory(collector)
    endif()

    add_subdirectory(components)
  
    configure_file(cuda.xml.in cuda.xml)
        
    install(
        FILES
            ${PROJECT_BINARY_DIR}/CUDA/cuda.xml
        DESTINATION
            share/KrellInstitute/xml
        )
  
endif()

if(CUDA_FOUND AND CUPTI_FOUND AND (${CUPTI_API_VERSION} GREATER 3))
    add_subdirectory(cupti_avail)
endif()
