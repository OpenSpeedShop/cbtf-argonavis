////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013,2014 Krell Institute. All Rights Reserved.
// Copyright (c) 2015 Argo Navis Technologies. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////

/** @file Definition of the Function class. */

#include <boost/format.hpp>
#include <cxxabi.h>
#include <sstream>

#include <ArgoNavis/Base/Function.hpp>
#include <ArgoNavis/Base/LinkedObject.hpp>
#include <ArgoNavis/Base/Loop.hpp>
#include <ArgoNavis/Base/Statement.hpp>

#include "SymbolTable.hpp"

using namespace ArgoNavis::Base;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Function::Function(const LinkedObject& linked_object, const std::string& name) :
    dm_symbol_table(linked_object.dm_symbol_table),
    dm_unique_identifier(dm_symbol_table->functions().add(
                             Impl::SymbolTable::FunctionFields(name)))
{
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Function::operator std::string() const
{
    std::ostringstream stream;
    stream << *this;
    return stream.str();    
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Function::operator<(const Function& other) const
{
    if (dm_symbol_table < other.dm_symbol_table)
    {
        return true;
    }
    else if (other.dm_symbol_table < dm_symbol_table)
    {
        return false;
    }
    else
    {
        return dm_unique_identifier < other.dm_unique_identifier;
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Function::operator==(const Function& other) const
{
    return (dm_symbol_table == other.dm_symbol_table) &&
        (dm_unique_identifier == other.dm_unique_identifier);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Function Function::clone(LinkedObject& linked_object) const
{
    return Function(
        linked_object.dm_symbol_table,
        linked_object.dm_symbol_table->functions().clone(
            dm_symbol_table->functions(), dm_unique_identifier
            )
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Function::add(const std::set<AddressRange>& ranges)
{
    dm_symbol_table->functions().add(dm_unique_identifier, ranges);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LinkedObject Function::getLinkedObject() const
{
    return LinkedObject(dm_symbol_table);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::string Function::getMangledName() const
{
    return dm_symbol_table->functions().fields(dm_unique_identifier).dm_name;
}



//------------------------------------------------------------------------------
// Get the mangled name of the function and then use the __cxa_demangle() ABI
// function to demangle this name. Return the mangled name if the demangling
// fails for any reason.
//
// http://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.3/a01696.html
//------------------------------------------------------------------------------
std::string Function::getDemangledName() const
{
    std::string mangled = getMangledName();
    
    int status = 0;
    char* raw = abi::__cxa_demangle(mangled.c_str(), NULL, 0, &status);
    
    if (status != 0)
    {
        return mangled;
    }
    
    std::string demangled(raw);
    free(raw);
    
    return demangled;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::set<AddressRange> Function::ranges() const
{
    return dm_symbol_table->functions().addresses(dm_unique_identifier);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Function::visitDefinitions(const StatementVisitor& visitor) const
{
    std::set<AddressRange> ranges =
        dm_symbol_table->functions().addresses(dm_unique_identifier);

    dm_symbol_table->statements().visit<Statement, StatementVisitor>(
        AddressRange(ranges.begin()->begin()), dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Function::visitLoops(const LoopVisitor& visitor) const
{
    dm_symbol_table->loops().visit<Loop, LoopVisitor>(
        dm_symbol_table->functions().addresses(dm_unique_identifier),
        dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Function::visitStatements(const StatementVisitor& visitor) const
{
    dm_symbol_table->statements().visit<Statement, StatementVisitor>(
        dm_symbol_table->functions().addresses(dm_unique_identifier),
        dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::ostream& ArgoNavis::Base::operator<<(std::ostream& stream,
                                          const Function& function)
{
    stream << boost::str(
        boost::format("Function %u in SymbolTable 0x%016X") % 
        function.dm_unique_identifier % function.dm_symbol_table.get()
        );
    
    return stream;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Function::Function(const Impl::SymbolTable::Handle& symbol_table,
                   boost::uint32_t unique_identifier) :
    dm_symbol_table(symbol_table),
    dm_unique_identifier(unique_identifier)
{
}



//------------------------------------------------------------------------------
// The individual comparisons are performed from least to most expensive in
// order to optimize performance.
//------------------------------------------------------------------------------
bool ArgoNavis::Base::equivalent(const Function& first, const Function& second)
{
    if (first.getMangledName() != second.getMangledName())
    {
        return false;
    }

    if (first.ranges() != second.ranges())
    {
        return false;
    }    

    return true;
}
