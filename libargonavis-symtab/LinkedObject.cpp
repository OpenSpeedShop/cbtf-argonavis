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

/** @file Definition of the LinkedObject class. */

#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/ref.hpp>
#include <sstream>

#include <ArgoNavis/SymbolTable/Function.hpp>
#include <ArgoNavis/SymbolTable/LinkedObject.hpp>
#include <ArgoNavis/SymbolTable/Loop.hpp>
#include <ArgoNavis/SymbolTable/Statement.hpp>

#include "SymbolTable.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::SymbolTable;



/** Anonymous namespace hiding implementation details. */
namespace {

    /**
     * Visitor used to determine if an arbitrary set of visited entities
     * contains an entity equivalent to the given entity. The visitation
     * is terminated as soon as such an entity is found.
     */
    template <typename T>
    bool containsEquivalent(const T& entity, const T& x, bool& contains)
    {
        contains |= equivalent(x, entity);
        return !contains;
    }
    
    /**
     * Visitor for determining if the given linked object contains a function
     * equivalent to all functions in an arbitrary set of visited functions.
     * The visitation is terminated as soon as a function is encountered that
     * doesn't have an equivalent in the given linked object.
     */
    bool containsAllFunctions(const LinkedObject& linked_object,
                              const Function& x, bool& contains)
    {
        bool contains_this = false;

        linked_object.visitFunctions(
            boost::bind(
                containsEquivalent<Function>, x, _1, boost::ref(contains_this)
                )
            );
        
        contains &= contains_this;
        return contains;
    }
    
    /**
     * Visitor for determining if the given linked object contains a loop
     * equivalent to all loops in an arbitrary set of visited loops. The
     * visitation is terminated as soon as a loop is encountered that
     * doesn't have an equivalent in the given linked object.
     */
    bool containsAllLoops(const LinkedObject& linked_object,
                          const Loop& x, bool& contains)
    {
        bool contains_this = false;

        linked_object.visitLoops(
            boost::bind(
                containsEquivalent<Loop>, x, _1, boost::ref(contains_this)
                )
            );
        
        contains &= contains_this;
        return contains;
    }
    
    /**
     * Visitor for determining if the given linked object contains a statement
     * equivalent to all statements in an arbitrary set of visited statements.
     * The visitation is terminated as soon as a statement is encountered that
     * doesn't have an equivalent in the given linked object.
     */
    bool containsAllStatements(const LinkedObject& linked_object,
                               const Statement& x, bool& contains)
    {
        bool contains_this = false;
        
        linked_object.visitStatements(
            boost::bind(
                containsEquivalent<Statement>, x, _1, boost::ref(contains_this)
                )
            );
        
        contains &= contains_this;
        return contains;
    }
    
} // namespace <anonymous>



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LinkedObject::LinkedObject(const FileName& file) :
    dm_symbol_table(new Impl::SymbolTable(file))
{
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LinkedObject::LinkedObject(const CBTF_Protocol_SymbolTable& message) :
    dm_symbol_table(new Impl::SymbolTable(message))
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LinkedObject::operator CBTF_Protocol_SymbolTable() const
{
    return *dm_symbol_table;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LinkedObject::operator std::string() const
{
    std::ostringstream stream;
    stream << *this;
    return stream.str();    
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool LinkedObject::operator<(const LinkedObject& other) const
{
    return dm_symbol_table < other.dm_symbol_table;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool LinkedObject::operator==(const LinkedObject& other) const
{
    return dm_symbol_table == other.dm_symbol_table;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LinkedObject LinkedObject::clone() const
{
    return LinkedObject(
        Impl::SymbolTable::Handle(new Impl::SymbolTable(*dm_symbol_table))
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
FileName LinkedObject::getFile() const
{
    return dm_symbol_table->getFile();
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void LinkedObject::visitFunctions(const FunctionVisitor& visitor) const
{
    dm_symbol_table->functions().visit<Function, FunctionVisitor>(
        dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void LinkedObject::visitFunctions(const AddressRange& range,
                                  const FunctionVisitor& visitor) const
{
    dm_symbol_table->functions().visit<Function, FunctionVisitor>(
        range, dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void LinkedObject::visitLoops(const LoopVisitor& visitor) const
{
    dm_symbol_table->loops().visit<Loop, LoopVisitor>(
        dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void LinkedObject::visitLoops(const AddressRange& range,
                              const LoopVisitor& visitor) const
{
    dm_symbol_table->loops().visit<Loop, LoopVisitor>(
        range, dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void LinkedObject::visitStatements(const StatementVisitor& visitor) const
{
    dm_symbol_table->statements().visit<Statement, StatementVisitor>(
        dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void LinkedObject::visitStatements(const AddressRange& range,
                                   const StatementVisitor& visitor) const
{
    dm_symbol_table->statements().visit<Statement, StatementVisitor>(
        range, dm_symbol_table, visitor
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::ostream& ArgoNavis::SymbolTable::operator<<(
    std::ostream& stream,
    const LinkedObject& linked_object
    )
{
    stream << boost::str(boost::format("SymbolTable 0x%016X") % 
                         linked_object.dm_symbol_table.get());
    
    return stream;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
LinkedObject::LinkedObject(const Impl::SymbolTable::Handle& symbol_table) :
    dm_symbol_table(symbol_table)
{
}



//------------------------------------------------------------------------------
// The individual comparisons are performed from least to most expensive in
// order to optimize performance. The implementation of the comparisons would
// be signficantly simplified if lambda expresions could be used here...
//------------------------------------------------------------------------------
bool ArgoNavis::SymbolTable::equivalent(const LinkedObject& first,
                                        const LinkedObject& second)
{
    // Are the two linked object's files different?
    if (first.getFile() != second.getFile())
    {
        return false;
    }

    // Is "second" missing any of the functions from "first"?
    
    bool contains = true;
    
    first.visitFunctions(
        boost::bind(containsAllFunctions, second, _1, boost::ref(contains))
        );
    
    if (!contains)
    {
        return false;
    }

    // Is "first" missing any of the functions from "second"?

    contains = true;
    
    second.visitFunctions(
        boost::bind(containsAllFunctions, first, _1, boost::ref(contains))
        );

    if (!contains)
    {
        return false;
    }
    
    // Is "second" missing any of the loops from "first"?
    
    contains = true;
    
    first.visitLoops(
        boost::bind(containsAllLoops, second, _1, boost::ref(contains))
        );
    
    if (!contains)
    {
        return false;
    }

    // Is "first" missing any of the loops from "second"?

    contains = true;
    
    second.visitLoops(
        boost::bind(containsAllLoops, first, _1, boost::ref(contains))
        );

    if (!contains)
    {
        return false;
    }
    
    // Is "second" missing any of the statements from "first"?
    
    contains = true;
    
    first.visitStatements(
        boost::bind(containsAllStatements, second, _1, boost::ref(contains))
        );

    if (!contains)
    {
        return false;
    }
    
    // Is "first" missing any of the statements from "second"?
    
    contains = true;
    
    second.visitStatements(
        boost::bind(containsAllStatements, first, _1, boost::ref(contains))
        );

    if (!contains)
    {
        return false;
    }

    // Otherwise the linked objects are equivalent    
    return true;
}
