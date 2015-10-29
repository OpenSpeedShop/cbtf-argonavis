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

/** @file Declaration of the Loop class. */

#pragma once

#include <boost/cstdint.hpp>
#include <boost/operators.hpp>
#include <boost/shared_ptr.hpp>
#include <iostream>
#include <set>
#include <string>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/AddressRange.hpp>

#include <ArgoNavis/Base/FunctionVisitor.hpp>
#include <ArgoNavis/Base/StatementVisitor.hpp>

namespace ArgoNavis { namespace Base {

    class LinkedObject;

    namespace Impl {
        template <typename T> class EntityTable;
        class SymbolTable;
    }

    /**
     * A source code loop within a linked object.
     */
    class Loop :
        public boost::totally_ordered<Loop>
    {
        template <typename T> friend class Impl::EntityTable;

    public:

        /**
         * Construct a loop within the given linked object from its head
         * address. The loop initially has no address ranges.
         *
         * @param linked_object    Linked object containing this loop.
         * @param head             Head address of this loop.
         *
         * @note    The address specified is relative to the beginning of
         *          the linked object containing this loop rather than an
         *          absolute address from the address space of a specific
         *          process.
         */
        Loop(const LinkedObject& linked_object, const Address& head);
        
        /**
         * Type conversion to a string.
         *
         * @return    String describing this loop.
         *
         * @note    This type conversion calls the Loop redirection to an
         *          output stream, and is intended for debugging use only.
         */
        operator std::string() const;
        
        /**
         * Is this loop less than another one?
         *
         * @param other    Loop to be compared.
         * @return         Boolean "true" if this loop is less than the
         *                 loop to be compared, or "false" otherwise.
         */
        bool operator<(const Loop& other) const;

        /**
         * Is this loop equal to another one?
         *
         * @param other    Loop to be compared.
         * @return         Boolean "true" if the loops are equal,
         *                 or "false" otherwise.
         */
        bool operator==(const Loop& other) const;

        /**
         * Create a deep copy of this loop.
         *
         * @param linked_object    Linked object containing the deep copy.
         * @return                 Deep copy of this loop.
         */
        Loop clone(LinkedObject& linked_object) const;

        /**
         * Associate the given address ranges with this loop. 
         *
         * @param ranges    Address ranges to associate with this loop.
         *
         * @note    The addresses specified are relative to the beginning
         *          of the linked object containing this loop rather than
         *          an absolute address from the address space of a specific
         *          process.
         */
        void add(const std::set<AddressRange>& ranges);

        /**
         * Get the linked object containing this loop.
         *
         * @return    Linked object containing this loop.
         */
        LinkedObject getLinkedObject() const;
        
        /**
         * Get the head address of this loop.
         *
         * @return    Head address of this loop.
         *
         * @note    The address specified is relative to the beginning of
         *          the linked object containing this loop rather than an
         *          absolute address from the address space of a specific
         *          process.
         */
        Address getHeadAddress() const;
        
        /**
         * Get the address ranges associated with this loop. An empty set
         * is returned if no address ranges are associated with this loop.
         *
         * @return    Address ranges associated with this loop.
         *
         * @note    The addresses specified are relative to the beginning
         *          of the linked object containing this loop rather than
         *          an absolute address from the address space of a specific
         *          process.
         */
        std::set<AddressRange> getAddressRanges() const;

        /**
         * Visit the definitions of this loop.
         *
         * @param visitor    Visitor invoked for each defintion of this loop.
         */
        void visitDefinitions(const StatementVisitor& visitor) const;

        /**
         * Visit the functions containing this loop.
         *
         * @param visitor    Visitor invoked for each function containing
         *                   this loop.
         */
        void visitFunctions(const FunctionVisitor& visitor) const;

        /**
         * Visit the statements associated with this loop.
         *
         * @param visitor    Visitor invoked for each statement associated 
         *                   with this loop.
         */
        void visitStatements(const StatementVisitor& visitor) const;

        /**
         * Redirection to an output stream.
         *
         * @param stream    Destination output stream.
         * @param loop      Loop to be redirected.
         * @return          Destination output stream.
         *
         * @note    This redirection dumps only the address of the
         *          symbol table containing, and unique identifier
         *          of, the loop. It is intended for debugging use.
         */
        friend std::ostream& operator<<(std::ostream& stream, const Loop& loop);
        
    private:

        /**
         * Construct a loop from its symbol table and unique identifier.
         *
         * @param symbol_table         Symbol table containing this loop.
         * @param unique_identifier    Unique identifier for this loop
         *                             within that symbol table.
         */
        Loop(const boost::shared_ptr<Impl::SymbolTable>& symbol_table,
             boost::uint32_t unique_identifier);
        
        /** Symbol table containing this loop. */
        boost::shared_ptr<Impl::SymbolTable> dm_symbol_table;
        
        /** Unique identifier for this loop within that symbol table. */
        boost::uint32_t dm_unique_identifier;
        
    }; // class Loop

    /**
     * Are the two given loops equivalent?
     *
     * @param first     First loop to be compared.
     * @param second    Second loop to be compared.
     * @return          Boolean "true" if the two loops are equivalent,
     *                  or "false" otherwise.
     *
     * @note    Differs from the Loop equality operator in that it compares
     *          the contents of the two loops rather than just their symbol
     *          table pointers and unique identifiers.
     */
    bool equivalent(const Loop& first, const Loop& second);
        
} } // namespace ArgoNavis::Base
