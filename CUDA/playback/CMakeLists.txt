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

set(SOURCES collector.c)

#
# The following linker options were useful when compiling and linking
# (not executing) the collector against a variety of CUPTI API versions.
#
#set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--unresolved-symbols=report-all -Wl,--warn-unresolved-symbols -Wl,--warn-once")

add_library(cuda-collector-monitor-fileio SHARED ${SOURCES})

target_include_directories(cuda-collector-monitor-fileio PUBLIC
    ${LibMonitor_INCLUDE_DIRS}
    ${CBTF_INCLUDE_DIRS}
    ${CBTF_KRELL_MESSAGES_INCLUDE_DIRS}
    ${CBTF_KRELL_SERVICES_INCLUDE_DIRS}
    )

target_link_libraries(cuda-collector-monitor-fileio
    ${CBTF_KRELL_MESSAGES_BASE_SHARED_LIBRARY}
    ${CBTF_KRELL_MESSAGES_EVENTS_SHARED_LIBRARY}
    ${CBTF_KRELL_SERVICES_COLLECTOR_MONITOR_FILEIO_SHARED_LIBRARY}
    ${CBTF_KRELL_SERVICES_MONITOR_SHARED_LIBRARY}
    ${CBTF_KRELL_SERVICES_OFFLINE_SHARED_LIBRARY}
    ${CBTF_KRELL_SERVICES_UNWIND_SHARED_LIBRARY}
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LibMonitor_SHARED_LIBRARIES}
    )

add_library(cuda-collector-monitor-mrnet SHARED ${SOURCES})

target_include_directories(cuda-collector-monitor-mrnet PUBLIC
    ${LibMonitor_INCLUDE_DIRS}
    ${CBTF_INCLUDE_DIRS}
    ${CBTF_KRELL_MESSAGES_INCLUDE_DIRS}
    ${CBTF_KRELL_SERVICES_INCLUDE_DIRS}
    )

target_link_libraries(cuda-collector-monitor-mrnet
    ${CBTF_KRELL_MESSAGES_BASE_SHARED_LIBRARY}
    ${CBTF_KRELL_MESSAGES_EVENTS_SHARED_LIBRARY}
    ${CBTF_KRELL_SERVICES_COLLECTOR_MONITOR_MRNET_SHARED_LIBRARY}
    ${CBTF_KRELL_SERVICES_MONITOR_SHARED_LIBRARY}
    ${CBTF_KRELL_SERVICES_OFFLINE_SHARED_LIBRARY}
    ${CBTF_KRELL_SERVICES_UNWIND_SHARED_LIBRARY}
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LibMonitor_SHARED_LIBRARIES}
    )

add_library(cuda-collector-monitor-mrnet-mpi SHARED ${SOURCES})

target_include_directories(cuda-collector-monitor-mrnet-mpi PUBLIC
    ${LibMonitor_INCLUDE_DIRS}
    ${CBTF_INCLUDE_DIRS}
    ${CBTF_KRELL_MESSAGES_INCLUDE_DIRS}
    ${CBTF_KRELL_SERVICES_INCLUDE_DIRS}
    )

target_link_libraries(cuda-collector-monitor-mrnet-mpi
    ${CBTF_KRELL_MESSAGES_BASE_SHARED_LIBRARY}
    ${CBTF_KRELL_MESSAGES_EVENTS_SHARED_LIBRARY}
    ${CBTF_KRELL_SERVICES_COLLECTOR_MONITOR_MRNET_MPI_SHARED_LIBRARY}
    ${CBTF_KRELL_SERVICES_MONITOR_SHARED_LIBRARY}
    ${CBTF_KRELL_SERVICES_OFFLINE_SHARED_LIBRARY}
    ${CBTF_KRELL_SERVICES_UNWIND_SHARED_LIBRARY}
    ${CMAKE_DL_LIBS}
    ${CMAKE_THREAD_LIBS_INIT}
    ${LibMonitor_SHARED_LIBRARIES}
    )

set_target_properties(
    cuda-collector-monitor-fileio
    cuda-collector-monitor-mrnet
    cuda-collector-monitor-mrnet-mpi
    PROPERTIES PREFIX ""
    )

install(
    TARGETS
        cuda-collector-monitor-fileio
        cuda-collector-monitor-mrnet
        cuda-collector-monitor-mrnet-mpi
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}/KrellInstitute/Collectors
    ARCHIVE DESTINATION lib${LIB_SUFFIX}/KrellInstitute/Collectors/static
    )
