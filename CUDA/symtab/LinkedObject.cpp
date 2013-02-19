////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 Silicon Graphics, Inc. All Rights Reserved.
// Copyright (c) 2012 Argo Navis Technologies. All Rights Reserved.
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

#include <KrellInstitute/SymbolTable/LinkedObject.hpp>

#include "LinkedObjectImpl.hpp"

using namespace KrellInstitute::SymbolTable;
using namespace KrellInstitute::SymbolTable::Impl;



//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
LinkedObject(const boost::filesystem::path& path) :
    dm_impl(new LinkedObjectImpl(path))
{
}


        
//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
LinkedObject(const CBTF_Protocol_SymbolTable& message) :
    dm_impl(new LinkedObjectImpl(message))
{
}



//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
LinkedObject(const LinkedObject& other) :
    dm_impl(new LinkedObjectImpl(other))
{
}



//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
LinkedObject::~LinkedObject()
{
    delete dm_impl;
}


        
//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
LinkedObject& LinkedObject::operator=(const LinkedObject& other)
{
    *dm_impl = *other.dm_impl;
    return *this;
}



//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
SymbolTable::operator CBTF_Protocol_SymbolTable() const
{
    return *dm_impl;
}



//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
boost::filesystem::path LinkedObject::getPath() const
{
    return dm_impl->getPath();
}



//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
std::set<Function> LinkedObject::getFunctions() const
{
    return dm_impl->getFunctions();
}



//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
std::set<Function> LinkedObject::getFunctionsAt(const Address& address) const
{
    return dm_impl->getFunctionsAt(address);
}



//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
std::set<Function> LinkedObject::getFunctionsByName(
    const std::string& name
    ) const
{
    return dm_impl->getFunctionsByName(name);
}



//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
std::set<Statement> LinkedObject::getStatements() const
{
    return dm_impl->getStatements();
}



//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
std::set<Statement> LinkedObject::getStatementsAt(const Address& address) const
{
    return dm_impl->getStatementsAt(address);
}



//------------------------------------------------------------------------------
// Let the implementation do the real work.
//------------------------------------------------------------------------------
std::set<Statement> LinkedObject::getStatementsBySourceFile(
    const boost::filesystem::path& path
    )
{
    return dm_impl->getStatementsBySourceFile(path);
}
