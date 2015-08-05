################################################################################
# Copyright (c) 2013-2014 Krell Institute. All Rights Reserved.
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

add_library(argonavis-symtab SHARED
    ArgoNavis/SymbolTable/AddressSpaces.hpp AddressSpaces.cpp
    ArgoNavis/SymbolTable/Function.hpp Function.cpp
    ArgoNavis/SymbolTable/FunctionVisitor.hpp
    ArgoNavis/SymbolTable/LinkedObject.hpp LinkedObject.cpp
    ArgoNavis/SymbolTable/LinkedObjectVisitor.hpp
    ArgoNavis/SymbolTable/Loop.hpp Loop.cpp
    ArgoNavis/SymbolTable/LoopVisitor.hpp
    ArgoNavis/SymbolTable/MappingVisitor.hpp
    ArgoNavis/SymbolTable/Statement.hpp Statement.cpp
    ArgoNavis/SymbolTable/StatementVisitor.hpp
    ArgoNavis/SymbolTable/ThreadVisitor.hpp
    AddressRangeIndex.hpp
    EntityTable.hpp
    EntityUID.hpp
    SymbolTable.hpp SymbolTable.cpp
    )

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/libargonavis-base
    ${Boost_INCLUDE_DIRS}
    ${CBTF_KRELL_MESSAGES_INCLUDE_DIRS}
    )

target_link_libraries(argonavis-symtab
    argonavis-base
    ${Boost_FILESYSTEM_LIBRARY}
    ${Boost_SYSTEM_LIBRARY}
    ${CBTF_KRELL_MESSAGES_LIBRARIES}
    )

set_target_properties(argonavis-symtab PROPERTIES VERSION 1.1.0)

add_executable(test-symtab test.cpp)

target_link_libraries(test-symtab
    argonavis-symtab
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
    )

install(DIRECTORY ArgoNavis DESTINATION include)

install(TARGETS argonavis-symtab
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    )
