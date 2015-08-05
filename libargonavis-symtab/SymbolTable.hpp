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

/** @file Declaration of the SymbolTable class. */

#pragma once

#include <boost/shared_ptr.hpp>
#include <string>

#include <KrellInstitute/Messages/Symbol.h>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/FileName.hpp>

#include "EntityTable.hpp"

namespace ArgoNavis { namespace SymbolTable { namespace Impl {

    /**
     * Symbol table for a single executable or library. This class provides
     * the underlying implementation details for the LinkedObject, Function,
     * Loop, and Statement classes.
     */
    class SymbolTable
    {

    public:

        /** Type of handle (smart pointer) to a symbol table. */
        typedef boost::shared_ptr<SymbolTable> Handle;

        /** Structure containing the non-address fields of a function. */
        struct FunctionFields
        {
            /** Mangled name of this function. */
            std::string dm_name;
            
            /** Constructor from initial values. */
            FunctionFields(const std::string& name) :
                dm_name(name)
            {
            }

        }; // struct FunctionFields

        /** Structure containing the non-address fields of a loop. */
        struct LoopFields
        {
            /** Head address of this loop. */
            Base::Address dm_head;

            /** Constructor from initial values. */
            LoopFields(const Base::Address& head) :
                dm_head(head)
            {
            }

        }; // struct LoopFields

        /** Structure containing the non-address fields of a statement. */
        struct StatementFields
        {
            /** Name of this statement's source file. */
            Base::FileName dm_file;
            
            /** Line number of this statement. */
            unsigned int dm_line;
            
            /** Column number of this statement. */
            unsigned int dm_column;

            /** Constructor from initial values. */
            StatementFields(const Base::FileName& file,
                            unsigned int line, unsigned int column) :
                dm_file(file),
                dm_line(line),
                dm_column(column)
            {
            }
            
        }; // struct StatementFields
                
        /**
         * Construct a symbol table for the named linked object file. The symbol
         * table initially has no symbols (functions, loops, statements, etc.)
         *
         * @param file    Name of this symbol table's linked object file.
         */
        SymbolTable(const Base::FileName& file);

        /**
         * Construct a symbol table from a CBTF_Protocol_SymbolTable.
         *
         * @param messsage    Message containing this symbol table.
         */
        SymbolTable(const CBTF_Protocol_SymbolTable& message);
        
        /**
         * Type conversion to a CBTF_Protocol_SymbolTable.
         *
         * @return    Message containing this symbol table.
         */
        operator CBTF_Protocol_SymbolTable() const;

        /**
         * Get the name of this symbol table's linked object file.
         *
         * @return    Name of this symbol table's linked object file.
         */
        const Base::FileName& getFile() const
        {
            return dm_file;
        }

        /**
         * Access the table of functions in this symbol table.
         *
         * @return    Table of functions in this symbol table.
         */
        const EntityTable<FunctionFields>& functions() const
        {
            return dm_functions;
        }

        /**
         * Access the table of functions in this symbol table.
         *
         * @return    Table of functions in this symbol table.
         */
        EntityTable<FunctionFields>& functions()
        {
            return dm_functions;
        }

        /**
         * Access the table of loops in this symbol table.
         *
         * @return    Table of loops in this symbol table.
         */
        const EntityTable<LoopFields>& loops() const
        {
            return dm_loops;
        }

        /**
         * Access the table of loops in this symbol table.
         *
         * @return    Table of loops in this symbol table.
         */
        EntityTable<LoopFields>& loops()
        {
            return dm_loops;
        }

        /**
         * Access the table of statements in this symbol table.
         *
         * @return    Table of statements in this symbol table.
         */
        const EntityTable<StatementFields>& statements() const
        {
            return dm_statements;
        }

        /**
         * Access the table of statements in this symbol table.
         *
         * @return    Table of statements in this symbol table.
         */
        EntityTable<StatementFields>& statements()
        {
            return dm_statements;
        }
        
    private:

        /** Name of this symbol table's linked object file. */
        Base::FileName dm_file;

        /** Table of functions in this symbol table. */
        EntityTable<FunctionFields> dm_functions;

        /** Table of loops in this symbol table. */
        EntityTable<LoopFields> dm_loops;

        /** Table of statements in this symbol table. */
        EntityTable<StatementFields> dm_statements;

    }; // class SymbolTable

} } } // namespace ArgoNavis::SymbolTable::Impl
