################################################################################
# Copyright (c) 2017 Argo Navis Technologies. All Rights Reserved.
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

set(RUNTIME_PLATFORM "" CACHE
    STRING "Front end runtime platform: none or [bgq,bgp,arm,mic,cray]")

set(CN_RUNTIME_PLATFORM "" CACHE
    STRING "Compute node runtime platform: none or [bgq,bgp,arm,mic,cray]")

message(STATUS "Front end runtime platform: " ${RUNTIME_PLATFORM})
message(STATUS "Compute node runtime platform: " ${CN_RUNTIME_PLATFORM})
  
if(CMAKE_SYSTEM_PROCESSOR MATCHES "ppc64*")
    set(CMAKE_LIBRARY_PATH ${CMAKE_INSTALL_PREFIX}/lib64)
    set(LIB_SUFFIX 64)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "ppc*")
    set(CMAKE_LIBRARY_PATH ${CMAKE_INSTALL_PREFIX}/lib)
elseif(EXISTS /usr/lib64)
    set(LIB_SUFFIX 64)
    set(CMAKE_LIBRARY_PATH ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
endif()

set(PLATFORM_BUILD_COLLECTOR TRUE)

set(PLATFORM_FE_COMPONENT_LOCATION 
    ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/KrellInstitute/Components)

set(PLATFORM_CN_COMPONENT_LOCATION 
    ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/KrellInstitute/Components)

if (CN_RUNTIME_PLATFORM MATCHES "cray")
    set(PLATFORM_BUILD_COLLECTOR FALSE)
endif()
