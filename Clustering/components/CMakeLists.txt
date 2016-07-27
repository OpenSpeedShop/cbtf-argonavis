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

add_library(Clustering SHARED
    ArgoNavis/Clustering/Feature.hpp
    ArgoNavis/Clustering/FeatureGenerator.hpp FeatureGenerator.cpp
    ArgoNavis/Clustering/FeatureName.hpp
    ArgoNavis/Clustering/FeatureVector.hpp
    BLAS.hpp BLAS.cpp
    ClusteringFilter.cpp
    ClusteringLeaf.cpp
    ClusteringManager.cpp
    CompareFeatureNames.hpp
    Messages.h Messages.c
    State.hpp State.cpp
    ThreadTable.hpp ThreadTable.cpp
    ThreadUID.hpp
    XDR.cpp
    )

add_custom_command(
    COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/Messages.h
    COMMAND rm -f ${CMAKE_CURRENT_BINARY_DIR}/Messages.c
    COMMAND rpcgen -h -o ${CMAKE_CURRENT_BINARY_DIR}/Messages.h Messages.x
    COMMAND rpcgen -c -o ${CMAKE_CURRENT_BINARY_DIR}/Messages.c Messages.x
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Messages.x
    OUTPUT Messages.h Messages.c
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

target_include_directories(Clustering PUBLIC
    ${PROJECT_SOURCE_DIR}/libargonavis-base
    ${PROJECT_BINARY_DIR}/Clustering/messages
    ${Boost_INCLUDE_DIRS}
    ${CBTF_INCLUDE_DIRS}
    ${CBTF_KRELL_CORE_INCLUDE_DIRS}
    ${CBTF_KRELL_MESSAGES_INCLUDE_DIRS}
    ${MRNet_INCLUDE_DIRS}
    )

target_link_libraries(Clustering
    argonavis-base
    cbtf-messages-clustering
    ${CBTF_LIBRARIES}
    ${CBTF_KRELL_CORE_LIBRARIES}
    ${CBTF_KRELL_MESSAGES_LIBRARIES}
    ${MRNet_LIBRARIES}
    )

set_target_properties(Clustering PROPERTIES PREFIX "")

install(DIRECTORY ArgoNavis DESTINATION include)

install(TARGETS Clustering
    RUNTIME DESTINATION bin/KrellInstitute/Components
    LIBRARY DESTINATION lib${LIB_SUFFIX}/KrellInstitute/Components
    ARCHIVE DESTINATION lib${LIB_SUFFIX}/KrellInstitute/Components
    )
