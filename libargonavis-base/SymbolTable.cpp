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

/** @file Definition of the SymbolTable class. */

#include "SymbolTable.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::Base::Impl;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SymbolTable::SymbolTable(const FileName& file) :
    dm_file(file),
    dm_functions(),
    dm_loops(),
    dm_statements()
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SymbolTable::SymbolTable(const CBTF_Protocol_SymbolTable& message) :
    dm_file(message.linked_object),
    dm_functions(),
    dm_loops(),
    dm_statements()
{
    //
    // Iterate over each function in the given CBTF_Protocol_SymbolTable
    // and construct the corresponding entries in this symbol table.
    //
    
    for (u_int i = 0; i < message.functions.functions_len; ++i)
    {
        const CBTF_Protocol_FunctionEntry& entry =
            message.functions.functions_val[i];
        
        EntityUID uid = dm_functions.add(
            FunctionFields(entry.name),
            AddressSet(entry.bitmaps.bitmaps_val, entry.bitmaps.bitmaps_len)
            );
    }

    //
    // Iterate over each statement in the given CBTF_Protocol_SymbolTable
    // and construct the corresponding entries in this symbol table.
    //

    for (u_int i = 0; i < message.statements.statements_len; ++i)
    {
        const CBTF_Protocol_StatementEntry& entry =
            message.statements.statements_val[i];

        EntityUID uid = dm_statements.add(
            StatementFields(entry.path,
                            static_cast<unsigned int>(entry.line),
                            static_cast<unsigned int>(entry.column)),
            AddressSet(entry.bitmaps.bitmaps_val, entry.bitmaps.bitmaps_len)
            );
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
SymbolTable::operator CBTF_Protocol_SymbolTable() const
{
    CBTF_Protocol_SymbolTable message;

    message.linked_object = dm_file;

    //
    // Allocate an appropriately sized array of CBTF_Protocol_FunctionEntry
    // within the message. Iterate over each function in this symbol table,
    // copying them into the message.
    //

    message.functions.functions_len = dm_functions.size();
    
    message.functions.functions_val =
        reinterpret_cast<CBTF_Protocol_FunctionEntry*>(
            malloc(std::max(1U, message.functions.functions_len) *
                   sizeof(CBTF_Protocol_FunctionEntry))
            );

    for (EntityUID i = 0; i < dm_functions.size(); ++i)
    {
        const FunctionFields& fields = dm_functions.fields(i);
        
        CBTF_Protocol_FunctionEntry& entry = 
            message.functions.functions_val[i];
 
        entry.name = strdup(fields.dm_name.c_str());

        dm_functions.addresses(i).extract(
            entry.bitmaps.bitmaps_val, entry.bitmaps.bitmaps_len
            );
    }
    
    //
    // Allocate an appropriately sized array of CBTF_Protocol_StatementEntry
    // within the message. Iterate over each statement in this symbol table,
    // copying them into the message.
    //

    message.statements.statements_len = dm_statements.size();

    message.statements.statements_val =
        reinterpret_cast<CBTF_Protocol_StatementEntry*>(
            malloc(std::max(1U, message.statements.statements_len) *
                   sizeof(CBTF_Protocol_StatementEntry))
            );

    for (EntityUID i = 0; i < dm_statements.size(); ++i)
    {
        const StatementFields& fields = dm_statements.fields(i);
        
        CBTF_Protocol_StatementEntry& entry = 
            message.statements.statements_val[i];

        entry.path = fields.dm_file;
        entry.line = static_cast<int>(fields.dm_line);
        entry.column = static_cast<int>(fields.dm_column);

        dm_statements.addresses(i).extract(
            entry.bitmaps.bitmaps_val, entry.bitmaps.bitmaps_len
            );
    }
    
    // Return the completed CBTF_Protocol_SymbolTable to the caller
    return message;
}
