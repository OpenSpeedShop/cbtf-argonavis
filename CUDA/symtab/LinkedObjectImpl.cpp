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

/** @file Definition of the LinkedObjectImpl class. */

#include "LinkedObjectImpl.hpp"

using namespace KrellInstitute::SymbolTable;
using namespace KrellInstitute::SymbolTable::Impl;



//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
LinkedObjectImpl(const boost::filesystem::path& path)
{
    // ...
}



//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
LinkedObjectImpl(const CBTF_Protocol_SymbolTable& message)
{
    // ...
}



//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
LinkedObjectImpl(const LinkedObjectImpl& other)
{
    // ...
}



//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
LinkedObjectImpl::~LinkedObjectImpl()
{
    // ...
}


        
//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
LinkedObjectImpl& LinkedObjectImpl::operator=(const LinkedObjectImpl& other)
{
    if (this != &other)
    {
        // ...
    }
    return *this;
}



//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
SymbolTable::operator CBTF_Protocol_SymbolTable() const
{
    CBTF_Protocol_SymbolTable message;

    // ...

    return message;
}



//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
boost::filesystem::path LinkedObjectImpl::getPath() const
{
    // ...

    return boost::filesystem::path();
}



//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
std::set<Function> LinkedObjectImpl::getFunctions() const
{
    // ...

    return std::set<Function>();
}



//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
std::set<Function> LinkedObjectImpl::getFunctionsAt(
    const Address& address
    ) const
{
    // ...

    return std::set<Function>();
}



//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
std::set<Function> LinkedObjectImpl::getFunctionsByName(
    const std::string& name
    ) const
{
    // ...

    return std::set<Function>();
}



//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
std::set<Statement> LinkedObjectImpl::getStatements() const
{
    // ...

    return std::set<Statement>();
}



//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
std::set<Statement> LinkedObjectImpl::getStatementsAt(
    const Address& address
    ) const
{
    // ...

    return std::set<Statement>();
}



//------------------------------------------------------------------------------
// ...
//------------------------------------------------------------------------------
std::set<Statement> LinkedObjectImpl::getStatementsBySourceFile(
    const boost::filesystem::path& path
    ) const
{
    // ...
    
    return std::set<Statement>();
}
