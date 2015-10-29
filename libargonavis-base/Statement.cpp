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

/** @file Definition of the Statement class. */

#include <boost/format.hpp>
#include <sstream>

#include <ArgoNavis/Base/Function.hpp>
#include <ArgoNavis/Base/LinkedObject.hpp>
#include <ArgoNavis/Base/Loop.hpp>
#include <ArgoNavis/Base/Statement.hpp>

#include "SymbolTable.hpp"

using namespace ArgoNavis::Base;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Statement::Statement(const LinkedObject& linked_object,
                     const FileName& file,
                     unsigned int line,
                     unsigned int column) :
    dm_symbol_table(linked_object.dm_symbol_table),
    dm_unique_identifier(dm_symbol_table->statements().add(
                             Impl::SymbolTable::StatementFields(
                                 file, line, column
                                 )
                             ))
{
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Statement::operator std::string() const
{
    std::ostringstream stream;
    stream << *this;
    return stream.str();    
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Statement::operator<(const Statement& other) const
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
bool Statement::operator==(const Statement& other) const
{
    return (dm_symbol_table == other.dm_symbol_table) &&
        (dm_unique_identifier == other.dm_unique_identifier);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Statement Statement::clone(LinkedObject& linked_object) const
{
    return Statement(
        linked_object.dm_symbol_table,
        linked_object.dm_symbol_table->statements().clone(
            dm_symbol_table->statements(), dm_unique_identifier
            )
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Statement::add(const std::set<AddressRange>& ranges)
{
    dm_symbol_table->statements().add(dm_unique_identifier, ranges);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LinkedObject Statement::getLinkedObject() const
{
    return LinkedObject(dm_symbol_table);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
FileName Statement::getFile() const
{
    return dm_symbol_table->statements().fields(dm_unique_identifier).dm_file;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
unsigned int Statement::getLine() const
{
    return dm_symbol_table->statements().fields(dm_unique_identifier).dm_line;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
unsigned int Statement::getColumn() const
{
    return dm_symbol_table->statements().fields(dm_unique_identifier).dm_column;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::set<AddressRange> Statement::ranges() const
{
    return dm_symbol_table->statements().addresses(dm_unique_identifier);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Statement::visitFunctions(const FunctionVisitor& visitor) const
{
    dm_symbol_table->functions().visit<Function, FunctionVisitor>(
        dm_symbol_table->statements().addresses(dm_unique_identifier),
        dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Statement::visitLoops(const LoopVisitor& visitor) const
{
    dm_symbol_table->loops().visit<Loop, LoopVisitor>(
        dm_symbol_table->statements().addresses(dm_unique_identifier),
        dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::ostream& ArgoNavis::Base::operator<<(std::ostream& stream,
                                          const Statement& statement)
{
    stream << boost::str(
        boost::format("Statement %u in SymbolTable 0x%016X") % 
        statement.dm_unique_identifier % statement.dm_symbol_table.get()
        );
    
    return stream;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Statement::Statement(const Impl::SymbolTable::Handle& symbol_table,
                     boost::uint32_t unique_identifier) :
    dm_symbol_table(symbol_table),
    dm_unique_identifier(unique_identifier)
{
}



//------------------------------------------------------------------------------
// The individual comparisons are performed from least to most expensive in
// order to optimize performance.
//------------------------------------------------------------------------------
bool ArgoNavis::Base::equivalent(const Statement& first,
                                 const Statement& second)
{
    if (first.getLine() != second.getLine())
    {
        return false;
    }

    if (first.getColumn() != second.getColumn())
    {
        return false;
    }

    if (first.getFile() != second.getFile())
    {
        return false;
    }

    if (first.ranges() != second.ranges())
    {
        return false;
    }

    return true;
}
