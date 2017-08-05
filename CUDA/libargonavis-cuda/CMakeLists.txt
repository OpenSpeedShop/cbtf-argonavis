################################################################################
# Copyright (c) 2015-2017 Argo Navis Technologies. All Rights Reserved.
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

add_library(argonavis-cuda SHARED
    ArgoNavis/CUDA/CachePreference.hpp
    ArgoNavis/CUDA/CopyKind.hpp
    ArgoNavis/CUDA/CounterDescription.hpp
    ArgoNavis/CUDA/CounterKind.hpp
    ArgoNavis/CUDA/DataTransfer.hpp
    ArgoNavis/CUDA/DataTransferVisitor.hpp
    ArgoNavis/CUDA/Device.hpp
    ArgoNavis/CUDA/KernelExecution.hpp
    ArgoNavis/CUDA/KernelExecutionVisitor.hpp
    ArgoNavis/CUDA/MemoryKind.hpp
    ArgoNavis/CUDA/PerformanceData.hpp PerformanceData.cpp
    ArgoNavis/CUDA/stringify.hpp stringify.cpp
    ArgoNavis/CUDA/Vector.hpp
    BlobGenerator.hpp BlobGenerator.cpp
    DataTable.hpp DataTable.cpp
    EventTable.hpp
    PartialEventTable.hpp
    )

target_include_directories(argonavis-cuda PUBLIC
    ${PROJECT_SOURCE_DIR}/libargonavis-base
    ${PROJECT_BINARY_DIR}/CUDA/messages
    ${Boost_INCLUDE_DIRS}
    ${CBTF_KRELL_MESSAGES_INCLUDE_DIRS}
    )

target_link_libraries(argonavis-cuda
    argonavis-base
    cbtf-messages-cuda
    ${CBTF_KRELL_MESSAGES_BASE_SHARED_LIBRARY}
    )

set_target_properties(argonavis-cuda PROPERTIES VERSION 1.1.0)

install(DIRECTORY ArgoNavis DESTINATION include)

install(TARGETS argonavis-cuda
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    )
