////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,2016 Argo Navis Technologies. All Rights Reserved.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
// details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////

/** @file Declaration and definition of the EventTable class. */

#pragma once

#include <map>
#include <set>
#include <utility>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

namespace ArgoNavis { namespace CUDA { namespace Impl {

    /**
     * Table of completed events (kernel executions, data transfers, etc.)
     * contained within a data table. Completed events are those for which
     * all needed messages (e.g. enqueue, completion) have been seen.
     *
     * @tparam T    Type containing the information for the events.
     */
    template <typename T>
    class EventTable
    {

    public:

        /** Construct an empty completed event table. */
        EventTable() :
	    dm_contexts(),
            dm_events()
        {
        }

        /** Add a new completed event to this table. */
        void add(const T& event)
        {
            dm_contexts.insert(event.context);
            dm_events.insert(
                std::make_pair(
                    Base::TimeInterval(event.time_begin, event.time_end), event
                    )
                );
        }

        /** All known context addresses. */
        const std::set<Base::Address>& contexts() const
        {
            return dm_contexts;
        }
        
        /**
         * Visit the events in this table intersecting an address range.
         *
         * @tparam V    Type of visitor for the events.
         *
         * @param interval    Time interval to be found.
         * @param visitor     Visitor invoked for each event
         *                    contained within this table.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        template <typename V>
        void visit(const Base::TimeInterval& interval, const V& visitor) const
        {
            bool terminate = false;

            typename std::map<Base::TimeInterval, T>::const_iterator i =
                dm_events.lower_bound(interval.begin());

            if (i != dm_events.begin())
            {
                --i;
            }

            typename std::map<Base::TimeInterval, T>::const_iterator i_end =
                dm_events.upper_bound(interval.end());
            
            for (; !terminate && (i != i_end); ++i)
            {
                if (i->first.intersects(interval))
                {
                    terminate |= !visitor(i->second);
                }
            }
        }

    private:

        /** All known context addresses. */
        std::set<Base::Address> dm_contexts;

        /** Completed events indexed by their time interval. */
        std::map<Base::TimeInterval, T> dm_events;
        
    }; // class EventTable<T>

} } } // namespace ArgoNavis::CUDA::Impl
