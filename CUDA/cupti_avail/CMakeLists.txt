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

add_executable(cupti_avail cupti_avail.cpp)

target_include_directories(cupti_avail PUBLIC
    ${Boost_INCLUDE_DIRS}
    ${CUDA_INCLUDE_DIRS}
    ${CUPTI_INCLUDE_DIRS}
    )

target_link_libraries(cupti_avail
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
    ${CUDA_LIBRARIES}
    ${CUPTI_LIBRARIES}
    )

install(TARGETS cupti_avail RUNTIME DESTINATION bin)
