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

/** @file Declaration and definition of the EntityTable class. */

#pragma once

#include <boost/assert.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <set>
#include <utility>
#include <vector>

#include <ArgoNavis/Base/AddressRange.hpp>
#include <ArgoNavis/Base/AddressSet.hpp>

#include "AddressRangeIndex.hpp"
#include "EntityUID.hpp"

namespace ArgoNavis { namespace Base { namespace Impl {

    class SymbolTable;

    /**
     * Table of entities (functions, loops, statements, etc.) contained within
     * a symbol table. An index is used to accelerate visitation by addresses.
     *
     * @tparam T    Type containing the (non-address) fields for the entities.
     */
    template <typename T>
    class EntityTable
    {

    public:

        /** Construct an empty entity table. */
        EntityTable() :
            dm_entities(),
            dm_index()
        {
        }
        
        /**
         * Add a new entity to this table.
         *
         * @param fields       Fields of the new entity.
         * @param addresses    Optional, initial, addresses for the new entity.
         * @return             Unique identifier for the new entity.
         */
        EntityUID add(
            const T& fields,
            const boost::optional<AddressSet>& addresses = boost::none
            )
        {
            if (addresses)
            {
                dm_entities.push_back(std::make_pair(fields, *addresses));
                index(dm_entities.size() - 1, false);
            }
            else
            {
                dm_entities.push_back(std::make_pair(fields, AddressSet()));
            }
            
            return dm_entities.size() - 1;
        }
        
        /**
         * Associate the given address ranges with the given entity.
         *
         * @param uid       Unique identifier for the entity.
         * @param ranges    Address ranges to associate with that entity.
         */
        void add(const EntityUID& uid, const std::set<AddressRange>& ranges)
        {
            BOOST_ASSERT(uid < dm_entities.size());
            dm_entities[uid].second += ranges;
            index(uid, true);
        }

        /**
         * Get the addresses associated with the given entity.
         *
         * @param uid    Unique identifier for the entity.
         * @return       Addresses associated with that entity.
         */
        const AddressSet& addresses(const EntityUID& uid) const
        {
            BOOST_ASSERT(uid < dm_entities.size());
            return dm_entities[uid].second;
        }
        
        /**
         * Add a deep copy (clone) of the given entity to this table.
         *
         * @param table    Table containing the entity to be cloned.
         * @param uid      Unique identifier for the entity to be cloned.
         * @return         Unique identifier for the new entity.
         *
         * @note    By implementing the cloning here, rather than in the code
         *          of the external representation for the entities, we avoid
         *          the typically-expensive process of rebuilding the cloned
         *          entity's address set from its individual address ranges,
         *          because the address set can be copied directly here.
         */
        EntityUID clone(const EntityTable& table, const EntityUID& uid)
        {
            BOOST_ASSERT(uid < table.dm_entities.size());            
            dm_entities.push_back(table.dm_entities[uid]);
            index(dm_entities.size() - 1, false);
            return dm_entities.size() - 1;
        }
        
        /**
         * Get the fields associated with the given entity.
         *
         * @param uid    Unique identifier for the entity.
         * @return       Fields associated with that entity.
         */
        const T& fields(const EntityUID& uid) const
        {
            BOOST_ASSERT(uid < dm_entities.size());
            return dm_entities[uid].first;
        }
        
        /** Get the size of this table. */
        EntityUID size() const
        {
            return dm_entities.size();
        }
                
        /**
         * Visit all of the entities in this table.
         *
         * @tparam E    Type of external representation for the entities.
         * @tparam V    Type of visitor for the entities.
         *
         * @param symbol_table    Symbol table containing this table.
         * @param visitor         Visitor invoked for each entity
         *                        contained within this table.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        template <typename E, typename V>
        void visit(const boost::shared_ptr<SymbolTable>& symbol_table,
                   const V& visitor) const
        {
            E entity(symbol_table, 0);
            bool terminate = false;
            
            for (EntityUID i = 0, i_end = dm_entities.size();
                 !terminate && (i != i_end); 
                 ++i)
            {
                entity.dm_unique_identifier = i;
                terminate |= !visitor(entity);
            }
        }
        
        /**
         * Visit the entities in this table intersecting an address range.
         *
         * @tparam E    Type of external representation for the entities.
         * @tparam V    Type of visitor for the entities.
         *
         * @param range           Address range to be found.
         * @param symbol_table    Symbol table containing this table.
         * @param visitor         Visitor invoked for each entity
         *                        contained within this table.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         *
         * @todo    The current implementation of AddressSet is not efficient
         *          at storing a single AddressRange. Once that inadequacy is
         *          corrected, this method should be eliminated, and all uses
         *          of it within SymbolTable replaced with the visit() variant
         *          below that accepts an AddressSet.
         */
        template <typename E, typename V>
        void visit(const AddressRange& range,
                   const boost::shared_ptr<SymbolTable>& symbol_table,
                   const V& visitor) const
        {
            E entity(symbol_table, 0);
            bool terminate = false;
            boost::dynamic_bitset<> visited(dm_entities.size());

            AddressRangeIndex::nth_index<1>::type::const_iterator i =
                dm_index.get<1>().lower_bound(range.begin());
            
            if (i != dm_index.get<1>().begin())
            {
                --i;
            }
            
            AddressRangeIndex::nth_index<1>::type::const_iterator i_end = 
                dm_index.get<1>().upper_bound(range.end());
            
            for (; !terminate && (i != i_end); ++i)
            {
                if (i->dm_range.intersects(range) &&
                    (i->dm_uid < visited.size()) &&
                    !visited[i->dm_uid])
                {
                    visited[i->dm_uid] = true;
                    entity.dm_unique_identifier = i->dm_uid;
                    terminate |= !visitor(entity);
                }
            }
        }
        
        /**
         * Visit the entities in this table intesecting an address set.
         *
         * @tparam E    Type of external representation for the entities.
         * @tparam V    Type of visitor for the entities.
         *
         * @param set             Address set to be found.
         * @param symbol_table    Symbol table containing this table.
         * @param visitor         Visitor invoked for each entity
         *                        contained within this table.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         *
         * @todo    The current implementation of AddressSet does not provide
         *          an interface for visiting the address ranges in the set.
         *          Once that inadequacy is corrected, this method should be
         *          modified to use that visitation pattern instead.
         */
        template <typename E, typename V>
        void visit(const AddressSet& set,
                   const boost::shared_ptr<SymbolTable>& symbol_table,
                   const V& visitor) const
        {
            E entity(symbol_table, 0);
            bool terminate = false;
            boost::dynamic_bitset<> visited(dm_entities.size());
            
            std::set<AddressRange> ranges = set;
            
            for (std::set<AddressRange>::const_iterator
                     r = ranges.begin(); !terminate && (r != ranges.end()); ++r)
            {
                const AddressRange& range = *r;
                
                AddressRangeIndex::nth_index<1>::type::const_iterator i = 
                    dm_index.get<1>().lower_bound(range.begin());
            
                if (i != dm_index.get<1>().begin())
                {
                    --i;
                }
                
                AddressRangeIndex::nth_index<1>::type::const_iterator i_end = 
                    dm_index.get<1>().upper_bound(range.end());
                
                for (; !terminate && (i != i_end); ++i)
                {
                    if (i->dm_range.intersects(range) &&
                        (i->dm_uid < visited.size()) &&
                        !visited[i->dm_uid])
                    {
                        visited[i->dm_uid] = true;
                        entity.dm_unique_identifier = i->dm_uid;
                        terminate |= !visitor(entity);
                    }
                }
            }
        }
        
    private:

        /** Type of container used to store the list of entities. */
        typedef std::vector< std::pair<T, AddressSet> > List;
        
        /**
         * Index the given entity by its addresses.
         *
         * @param uid           Unique identifier for the entity to be indexed.
         * @param reindexing    Boolean "true" if the entity is being reindexed,
         *                      or "false" otherwise. This is an optimization to
         *                      eliminate the erase() in the cases where it is
         *                      known to be unnecessary.
         */
        void index(const EntityUID& uid, bool reindexing)
        {
            if (reindexing)
            {
                dm_index.get<0>().erase(uid);
            }
            
            std::set<AddressRange> ranges = dm_entities[uid].second;
            
            for (std::set<AddressRange>::const_iterator
                     i = ranges.begin(); i != ranges.end(); ++i)
            {
                dm_index.insert(AddressRangeIndexRow(uid, *i));
            }
        }
        
        /** List of entities in this table. */
        List dm_entities;
        
        /** Index used to find entities by addresses. */
        AddressRangeIndex dm_index;
        
    }; // class EntityTable<T>

} } } // namespace ArgoNavis::Base::Impl
