################################################################################
# Copyright (c) 2012 Argo Navis Technologies. All Rights Reserved.
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

add_library(cbtf-symtab SHARED
    KrellInstitute/SymbolTable/Address.hpp
    AddressBitmap.hpp AddressBitmap.cpp
    KrellInstitute/SymbolTable/AddressRange.hpp
    KrellInstitute/SymbolTable/Interval.hpp
    KrellInstitute/SymbolTable/Time.hpp
    KrellInstitute/SymbolTable/TimeInterval.hpp
    )

#    KrellInstitute/SymbolTable/Extent.hpp
#    KrellInstitute/SymbolTable/Function.hpp Function.cpp
#    FunctionImpl.hpp FunctionImpl.cpp
#    KrellInstitute/SymbolTable/LinkedObject.hpp LinkedObject.cpp
#    LinkedObjectImpl.hpp LinkedObjectImpl.cpp
#    KrellInstitute/SymbolTable/Statement.hpp Statement.cpp
#    StatementImpl.hpp StatementImpl.cpp

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${CBTF_MESSAGES_INCLUDE_DIRS}
    )

target_link_libraries(cbtf-symtab
    ${Boost_FILESYSTEM_LIBRARY}
    ${CBTF_MESSAGE_LIBRARIES}
    )

set_target_properties(cbtf-symtab PROPERTIES VERSION 0.0.0)

install(DIRECTORY KrellInstitute DESTINATION include)

install(TARGETS cbtf-symtab
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib${LIB_SUFFIX}
    ARCHIVE DESTINATION lib${LIB_SUFFIX}
    )
