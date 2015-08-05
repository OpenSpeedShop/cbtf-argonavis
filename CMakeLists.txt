################################################################################
# Copyright (c) 2012-2015 Argo Navis Technologies. All Rights Reserved.
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

cmake_minimum_required(VERSION 2.8.11)

project(CBTF-ArgoNavis)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

find_package(Boost 1.39.0 REQUIRED
    COMPONENTS filesystem regex system unit_test_framework
    )
find_package(CBTF REQUIRED)
find_package(CBTF-Krell REQUIRED)
#find_package(CUDA 5.0 REQUIRED)
find_package(CUPTI 2)
find_package(MRNet)
find_package(PAPI 5.4.0)
find_package(Threads REQUIRED)

if(DEFINED CUDA_TOOLKIT_ROOT_DIR)
    set(CUPTI_INCLUDE_DIR ${CUDA_TOOLKIT_ROOT_DIR}/extras/CUPTI/include)
    set(CUPTI_LIBRARY_DIR ${CUDA_cupti_LIBRARY})
    set(CUPTI_LIBRARIES ${CUPTI_LIBRARY_DIR})
    set(CUPTI_INCLUDE_DIRS ${CUDA_INCLUDE_DIRS} ${CUPTI_INCLUDE_DIR})
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64*")
    set(CMAKE_LIBRARY_PATH ${CMAKE_INSTALL_PREFIX}/lib64)
    set(LIB_SUFFIX 64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ppc*")
    set(CMAKE_LIBRARY_PATH ${CMAKE_INSTALL_PREFIX}/lib)
elseif(EXISTS /usr/lib64)
    set(LIB_SUFFIX 64)
    set(CMAKE_LIBRARY_PATH ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
endif()

add_subdirectory(libargonavis-base)
add_subdirectory(libargonavis-symtab)

if(CUPTI_FOUND AND MRNET_FOUND)
    add_subdirectory(CUDA)
endif()
