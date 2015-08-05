////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014 Krell Institute. All Rights Reserved.
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

/** @file Definition of the Loop class. */

#include <boost/format.hpp>
#include <sstream>

#include <ArgoNavis/SymbolTable/Function.hpp>
#include <ArgoNavis/SymbolTable/Loop.hpp>
#include <ArgoNavis/SymbolTable/LinkedObject.hpp>
#include <ArgoNavis/SymbolTable/Statement.hpp>

#include "SymbolTable.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::SymbolTable;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Loop::Loop(const LinkedObject& linked_object, const Address& head) :
    dm_symbol_table(linked_object.dm_symbol_table),
    dm_unique_identifier(dm_symbol_table->loops().add(
                             Impl::SymbolTable::LoopFields(head)))
{
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Loop::operator std::string() const
{
    std::ostringstream stream;
    stream << *this;
    return stream.str();    
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Loop::operator<(const Loop& other) const
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
bool Loop::operator==(const Loop& other) const
{
    return (dm_symbol_table == other.dm_symbol_table) &&
        (dm_unique_identifier == other.dm_unique_identifier);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Loop Loop::clone(LinkedObject& linked_object) const
{
    return Loop(
        linked_object.dm_symbol_table,
        linked_object.dm_symbol_table->loops().clone(
            dm_symbol_table->loops(), dm_unique_identifier
            )
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Loop::addAddressRanges(const std::set<AddressRange>& ranges)
{
    dm_symbol_table->loops().add(dm_unique_identifier, ranges);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LinkedObject Loop::getLinkedObject() const
{
    return LinkedObject(dm_symbol_table);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Address Loop::getHeadAddress() const
{
    return dm_symbol_table->loops().fields(dm_unique_identifier).dm_head;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::set<AddressRange> Loop::getAddressRanges() const
{
    return dm_symbol_table->loops().addresses(dm_unique_identifier);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Loop::visitDefinitions(const StatementVisitor& visitor) const
{
    dm_symbol_table->statements().visit<Statement, StatementVisitor>(
        AddressRange(
            dm_symbol_table->loops().fields(dm_unique_identifier).dm_head
            ),
        dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Loop::visitFunctions(const FunctionVisitor& visitor) const
{
    dm_symbol_table->functions().visit<Function, FunctionVisitor>(
        dm_symbol_table->loops().addresses(dm_unique_identifier),
        dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Loop::visitStatements(const StatementVisitor& visitor) const
{
    dm_symbol_table->statements().visit<Statement, StatementVisitor>(
        dm_symbol_table->loops().addresses(dm_unique_identifier),
        dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::ostream& ArgoNavis::SymbolTable::operator<<(
    std::ostream& stream, const Loop& loop
    )
{
    stream << boost::str(
        boost::format("Loop %u in SymbolTable 0x%016X") % 
        loop.dm_unique_identifier % loop.dm_symbol_table.get()
        );
    
    return stream;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Loop::Loop(const Impl::SymbolTable::Handle& symbol_table,
           boost::uint32_t unique_identifier) :
    dm_symbol_table(symbol_table),
    dm_unique_identifier(unique_identifier)
{
}



//------------------------------------------------------------------------------
// The individual comparisons are performed from least to most expensive in
// order to optimize performance.
//------------------------------------------------------------------------------
bool ArgoNavis::SymbolTable::equivalent(const Loop& first,
                                        const Loop& second)
{
    if (first.getHeadAddress() != second.getHeadAddress())
    {
        return false;
    }
    
    if (first.getAddressRanges() != second.getAddressRanges())
    {
        return false;
    }    
    
    return true;
}
