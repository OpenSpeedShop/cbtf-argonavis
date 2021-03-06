################################################################################
# Copyright (c) 2012-2013,2015 Argo Navis Technologies. All Rights Reserved.
# Copyright (c) 2018-2019 Krell Institute.  All Rights Reserved.
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

add_library(cbtf-messages-cuda SHARED CUDA_data.h CUDA_data.c)

add_custom_command(
    COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/CUDA_data.h
    COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/CUDA_data.c
    COMMAND rpcgen -h -o ${CMAKE_CURRENT_BINARY_DIR}/CUDA_data.h CUDA_data.x
    COMMAND rpcgen -c -o ${CMAKE_CURRENT_BINARY_DIR}/CUDA_data.c CUDA_data.x
    COMMAND mkdir -p ${CMAKE_CURRENT_BINARY_DIR}/KrellInstitute/Messages
    COMMAND ln -s -f ${CMAKE_CURRENT_BINARY_DIR}/CUDA_data.h ${CMAKE_CURRENT_BINARY_DIR}/KrellInstitute/Messages/CUDA_data.h
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/CUDA_data.x
    OUTPUT CUDA_data.h CUDA_data.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

target_include_directories(cbtf-messages-cuda PUBLIC
    ${Libtirpc_INCLUDE_DIRS}
    ${CBTF_INCLUDE_DIRS}
    ${CBTF_KRELL_MESSAGES_INCLUDE_DIRS}
    )

target_link_libraries(cbtf-messages-cuda
    ${CBTF_KRELL_MESSAGES_BASE_SHARED_LIBRARY}
    )

set_target_properties(cbtf-messages-cuda PROPERTIES VERSION 1.1.0)

install(
    FILES ${PROJECT_BINARY_DIR}/CUDA/messages/CUDA_data.h
    DESTINATION include/KrellInstitute/Messages
    )

install(TARGETS cbtf-messages-cuda
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    )
