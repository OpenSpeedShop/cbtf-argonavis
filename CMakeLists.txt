################################################################################
# Copyright (c) 2012-2017 Argo Navis Technologies. All Rights Reserved.
# Copyright (c) 2018 Krell Institute.  All Rights Reserved.
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

cmake_minimum_required(VERSION 2.8)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif()

project(CBTF-ArgoNavis)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)

include(${PROJECT_SOURCE_DIR}/cmake/CheckPlatform.cmake)

# The following inclusion and associated HAS_CXX11_AUTO condition can be
# removed once we are able to advance our required CMake version to 3.1.0
# (or above). It introduces new C++11 feature checking that obviates the
# need for this explicit checking.

include(${PROJECT_SOURCE_DIR}/cmake/CheckCXX11Features.cmake)

if(HAS_CXX11_AUTO)
    message(STATUS "C++11 support found: TRUE")
    message(STATUS "C++11 flags: " ${CXX11_COMPILER_FLAGS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_COMPILER_FLAGS}")
else()
    message(STATUS "C++11 support found: FALSE")
endif()

find_package(Boost 1.39.0 REQUIRED
    COMPONENTS filesystem program_options regex system unit_test_framework
    )
find_package(CBTF REQUIRED)
find_package(CBTF-Krell REQUIRED)
find_package(CUDA)
find_package(CUPTI 2)
find_package(MRNet)
find_package(PAPI 5.4.0)
find_package(LibMonitor)

find_package(Threads REQUIRED)

set(TLS_MODEL "implicit" CACHE STRING "cbtf-krell tls model: [implicit,explicit]")
#
#--------------------------------------------------------------------------------
# Handle the TLS options for building the runtimes
#    - explicit means the tool allocates and manages the thread local storage
#    - implicit means the tool uses the systems thread local storage
#--------------------------------------------------------------------------------
#
if (TLS_MODEL MATCHES "explicit")
    # Builder requsted explicit tls to be used in the runtimes and collectors
    set(TLS_DEFINES "USE_EXPLICIT_TLS" )
    message(STATUS "Build runtimes using explicit TLS model.")
elseif (TLS_MODEL MATCHES "implicit")
    # Builder requsted implicit tls to be used in the runtimes and collectors
    set(TLS_DEFINES "USE_IMPLICIT_TLS" )
    message(STATUS "Build runtimes using implicit TLS model.")
else()
    # Default to implicit tls in the runtimes and collectors
    set(TLS_DEFINES "USE_IMPLICIT_TLS" )
    message(STATUS "Build runtimes using implicit TLS model.")
endif()



add_subdirectory(libargonavis-base)
add_subdirectory(CUDA)
