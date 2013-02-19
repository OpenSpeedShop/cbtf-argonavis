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

/** @file Declaration of the LinkedObject class. */

#pragma once

#include <boost/filesystem.hpp>
#include <KrellInstitute/Messages/Symbol.h>
#include <KrellInstitute/SymbolTable/Address.h>
#include <set>
#include <string>

namespace KrellInstitute { namespace SymbolTable {

    class Function;
    class Statement;

    namespace Impl {
        class LinkedObjectImpl;
    }

    /**
     * A single executable or library (a "linked object").
     */
    class LinkedObject
    {

    public:

        /**
         * Construct a linked object from its full path name. The constructed
         * linked object initially has no symbols (functions, statements, etc.)
         *
         * @param path     Full path name of this linked object.
         */
        LinkedObject(const boost::filesystem::path& path);
        
        /**
         * Construct a linked object from a CBTF_Protocol_SymbolTable.
         *
         * @param message    Message containing this linked object.
         */
        LinkedObject(const CBTF_Protocol_SymbolTable& message);

        /**
         * Construct a linked object from an existing linked object.
         *
         * @param other    Linked object to be copied.
         */
        LinkedObject(const LinkedObject& other);
        
        /** Destructor. */
        virtual ~LinkedObject();
        
        /**
         * Replace this linked object with a copy of another one.
         *
         * @param other    Linked object to be copied.
         * @return         Resulting (this) linked object.
         */
        LinkedObject& operator=(const LinkedObject& other);

        /**
         * Type conversion to a CBTF_Protocol_SymbolTable.
         *
         * @return    Message containing this linked object.
         */
        operator CBTF_Protocol_SymbolTable() const;

        /**
         * Get the full path name of this linked object.
         *
         * @return    Full path name of this linked object.
         */
        boost::filesystem::path getPath() const;

        /**
         * Get the functions contained within this linked object. An empty set
         * is returned if no functions are found within this linked object.
         *
         * @return    Functions contained within this linked object.
         */
        std::set<Function> getFunctions() const;

        /**
         * Get the functions contained within this linked object at the given
         * address. An empty set is returned if no functions are found within
         * this linked object at that address.
         *
         * @param address    Address to be found.
         * @return           Functions contained within this linked object
         *                   at that address.
         *
         * @note    The address specified must be relative to the beginning of
         *          this linked object rather than an absolute address from the
         *          address space of a specific process.
         */
        std::set<Function> getFunctionsAt(const Address& address) const;

        /**
         * Get the functions contained within this linked object with the given
         * name. An empty set is returned if no functions are found within this
         * linked object with that name.
         *
         * @param name    Name of the function to find.
         * @return        Functions contained within this linked object
         *                with that name.
         */
        std::set<Function> getFunctionsByName(const std::string& name) const;
        
        /**
         * Get the statements contained within this linked object. An empty set
         * is returned if no statements are found within this linked object.
         *
         * @return    Statements contained within this linked object.
         */
        std::set<Statement> getStatements() const;

        /**
         * Get the statements contained within this linked object at the given
         * address. An empty set is returned if no statements are found within
         * this linked object at that address.
         *
         * @param address    Address to be found.
         * @return           Statements contained within this linked object
         *                   at that address.
         *
         * @note    The address specified must be relative to the beginning of
         *          this linked object rather than an absolute address from the
         *          address space of a specific process.
         */
        std::set<Statement> getStatementsAt(const Address& address) const;
        
        /**
         * Get the statements contained within this linked object for the
         * given source file. An empty set is returned if no statements are
         * found within this linked object for that source file.
         *
         * @param path    Source file for which to obtain statements.
         * @return        Statements contained within this linked object
         *                for that source file.
         */
        std::set<Statement> getStatementsBySourceFile(
            const boost::filesystem::path& path
            ) const;
        
    private:

        /**
         * Opaque pointer to this object's internal implementation details.
         * Provides information hiding, improves binary compatibility, and
         * reduces compile times.
         *
         * @sa http://en.wikipedia.org/wiki/Opaque_pointer
         */
        Impl::LinkedObjectImpl* dm_impl;

    }; // class LinkedObject
        
} } // namespace KrellInstitute::SymbolTable
