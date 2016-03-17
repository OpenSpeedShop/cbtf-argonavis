################################################################################
# Copyright (c) 2012-2016 Argo Navis Technologies. All Rights Reserved.
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

add_library(CUDA SHARED
    DataAggregatorForCUDA.cpp
    StateManagementForCUDA.cpp
    XDR.cpp
    )

target_include_directories(CUDA PUBLIC
    ${PROJECT_SOURCE_DIR}/libargonavis-base
    ${PROJECT_SOURCE_DIR}/CUDA/libargonavis-cuda
    ${PROJECT_BINARY_DIR}/CUDA/messages
    ${Boost_INCLUDE_DIRS}
    ${CBTF_INCLUDE_DIRS}
    ${CBTF_KRELL_CORE_INCLUDE_DIRS}
    ${CBTF_KRELL_MESSAGES_INCLUDE_DIRS}
    ${MRNet_INCLUDE_DIRS}
    )

target_link_libraries(CUDA
    argonavis-base
    argonavis-cuda
    cbtf-messages-cuda
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_REGEX_LIBRARY}
    ${CBTF_LIBRARIES}
    ${CBTF_KRELL_CORE_LIBRARIES}
    ${CBTF_KRELL_MESSAGES_LIBRARIES}
    ${MRNet_LIBRARIES}
    )

set_target_properties(CUDA PROPERTIES PREFIX "")

install(TARGETS CUDA
    RUNTIME DESTINATION bin/KrellInstitute/Components
    LIBRARY DESTINATION lib${LIB_SUFFIX}/KrellInstitute/Components
    ARCHIVE DESTINATION lib${LIB_SUFFIX}/KrellInstitute/Components
    )
