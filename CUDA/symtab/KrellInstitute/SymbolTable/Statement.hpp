////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 Silicon Graphics, Inc. All Rights Reserved.
// Copyright (c) 2007 William Hachfeld. All Rights Reserved.
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

/** @file Declaration of the Statement class. */

#pragma once

#include <boost/filesystem.hpp>
#include <boost/operators.hpp>
#include <set>

namespace KrellInstitute { namespace SymbolTable {

    class Function;
    class LinkedObject;

    namespace Impl {
        class StatementImpl;
    }

    /**
     * A source code statement from a symbol table.
     */
    class Statement :
        public boost::totally_ordered<Statement>
    {

    public:
        
        /** Destructor. */
        virtual ~Statement();
        
        /**
         * Replace this statement with a copy of another one.
         *
         * @param other    Statement to be copied.
         * @return         Resulting (this) statement.
         */
        Statement& operator=(const Statement& other);

        /**
         * Is this statement less than another one?
         *
         * @param other    Statement to be compared.
         * @return         Boolean "true" if this statement is less than the
         *                 statement to be compared, or "false" otherwise.
         */
        bool operator<(const Statement& other) const;

        /**
         * Is this statement equal to another one?
         *
         * @param other    Statement to be compared.
         * @return         Boolean "true" if the statements are equal,
         *                 or "false" otherwise.
         */
        bool operator==(const Statement& other) const;

        /**
         * Get the linked object containing this statement.
         *
         * @return    Linked object containing this statement.
         */
        LinkedObject getLinkedObject() const;
        
        /**
         * Get the functions containing this statement. An empty set is
         * returned if no function contains this statement.
         *
         * @return    Functions containing this statement.
         */
        std::set<Function> getFunctions() const;

        /**
         * Get the full path name of this statement's source file.
         *
         * @return    Full path name of this statement's source file.
         */
        boost::filesystem::path getPath() const;

        /**
         * Get the line number of this statement.
         *
         * @return    Line number of this statement.
         */
        int getLine() const;

        /**
         * Get the column number of this statement.
         *
         * @return    Column number of this statement.
         */
        int getColumn() const;

    private:

        /** Construct a statement from its implementation details. */
        Statement(Impl::StatementImpl* impl);
        
        /**
         * Opaque pointer to this object's internal implementation details.
         * Provides information hiding, improves binary compatibility, and
         * reduces compile times.
         *
         * @sa http://en.wikipedia.org/wiki/Opaque_pointer
         */
        Impl::StatementImpl* dm_impl;

    }; // class Statement
        
} } // namespace KrellInstitute::SymbolTable
