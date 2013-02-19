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

/** @file Declaration of the LinkedObjectImpl class. */

#pragma once

#include <boost/filesystem.hpp>
#include <KrellInstitute/Messages/Symbol.h>
#include <KrellInstitute/SymbolTable/Address.hpp>
#include <KrellInstitute/SymbolTable/Function.hpp>
#include <KrellInstitute/SymbolTable/Statement.hpp>
#include <set>
#include <string>

namespace KrellInstitute { namespace SymbolTable { namespace Impl {

    /**
     * Implementation details of the LinkedObject class. Anything that
     * would normally be a private member of LinkedObject is instead a
     * member of LinkedObjectImpl.
     */
    class LinkedObjectImpl
    {

    public:

        /** Construct a linked object from its full path name. */
        LinkedObjectImpl(const boost::filesystem::path& path);
        
        /** Construct a linked object from a CBTF_Protocol_SymbolTable. */
        LinkedObjectImpl(const CBTF_Protocol_SymbolTable& message);

        /** Construct a linked object from an existing linked object. */
        LinkedObjectImpl(const LinkedObjectImpl& other);

        /** Destructor. */
        virtual ~LinkedObjectImpl();
        
        /** Replace this linked object with a copy of another one. */
        LinkedObjectImpl& operator=(const LinkedObjectImpl& other);

        /** Type conversion to a CBTF_Protocol_SymbolTable. */
        operator CBTF_Protocol_SymbolTable() const;
        
        /** Get the full path name of this linked object. */
        boost::filesystem::path getPath() const;

        /** Get the functions contained within this linked object. */
        std::set<Function> getFunctions() const;

        /** Get the functions at the given address. */
        std::set<Function> getFunctionsAt(const Address& address) const;

        /** Get the functions with the given name. */
        std::set<Function> getFunctionsByName(const std::string& name) const;

        /** Get the statements contained within this linked object. */
        std::set<Statement> getStatements() const;

        /** Get the statements at the given address. */
        std::set<Statement> getStatementsAt(const Address& address) const;

        /** Get the statements for the given source file. */
        std::set<Statement> getStatementsBySourceFile(
            const boost::filesystem::path& path
            ) const;

    private:

        // ...

    }; // class LinkedObjectImpl
        
} } } // namespace KrellInstitute::SymbolTable::Impl
