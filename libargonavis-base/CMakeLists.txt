################################################################################
# Copyright (c) 2013 Krell Institute. All Rights Reserved.
# Copyright (c) 2015 Argo Navis Technologies. All Rights Reserved.
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

add_library(argonavis-base SHARED
    ArgoNavis/Base/Address.hpp
    ArgoNavis/Base/AddressBitmap.hpp AddressBitmap.cpp    
    ArgoNavis/Base/AddressRange.hpp
    ArgoNavis/Base/AddressSet.hpp AddressSet.cpp
    ArgoNavis/Base/FileName.hpp FileName.cpp
    ArgoNavis/Base/Interval.hpp
    ArgoNavis/Base/Raise.hpp
    ArgoNavis/Base/ThreadName.hpp ThreadName.cpp
    ArgoNavis/Base/Time.hpp
    ArgoNavis/Base/TimeInterval.hpp
    )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${CBTF_KRELL_MESSAGES_INCLUDE_DIRS}
    )

target_link_libraries(argonavis-base
    -lrt
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${CBTF_KRELL_MESSAGES_LIBRARIES}
    )

set_target_properties(argonavis-base PROPERTIES VERSION 1.1.0)

add_executable(test-base test.cpp)

target_link_libraries(test-base
    argonavis-base
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    )

install(DIRECTORY ArgoNavis DESTINATION include)

install(TARGETS argonavis-base
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    )
