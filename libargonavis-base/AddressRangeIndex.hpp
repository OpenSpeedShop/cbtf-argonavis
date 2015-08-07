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

/** @file Declaration of the AddressRangeIndex type. */

#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/AddressRange.hpp>

#include "EntityUID.hpp"

namespace ArgoNavis { namespace Base { namespace Impl {

    /** Structure containing one row of an address range index. */
    struct AddressRangeIndexRow
    {
        /** Unique identifier for an entity within this table. */
        EntityUID dm_uid;
        
        /** Address range for that entity. */
        AddressRange dm_range;
        
        /** Constructor from initial fields. */
        AddressRangeIndexRow(const EntityUID& uid, const AddressRange& range) :
            dm_uid(uid),
            dm_range(range)
        {
        }
        
        /** Key extractor for the address range's beginning. */
        struct range_begin
        {
            typedef Address result_type;
            
            result_type operator()(const AddressRangeIndexRow& row) const
            {
                return row.dm_range.begin();
            }
        };
        
    }; // struct AddressRangeIndexRow
            
    /**
     * Type of associative container used to search for the entities overlapping
     * a given address range.
     *
     * @note    EntityTable is the only place where AddressRangeIndex[Row] are
     *          used. So normally it would be sensible for those 2 types to be
     *          be private nested types in that class. But because EntityTable
     *          and boost::multi_index_container are both templates, the syntax
     *          for referencing AddressRangeIndex from EntityTable becomes very
     *          messy if the former is a nested type of the later. So they have
     *          been separated here.
     */
    typedef boost::multi_index_container<
        AddressRangeIndexRow,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_non_unique<
                boost::multi_index::member<
                    AddressRangeIndexRow,
                    EntityUID,
                    &AddressRangeIndexRow::dm_uid
                    >
                >,
            boost::multi_index::ordered_non_unique<
                typename AddressRangeIndexRow::range_begin
                >
            >
        > AddressRangeIndex;

} } } // namespace ArgoNavis::Base::Impl
