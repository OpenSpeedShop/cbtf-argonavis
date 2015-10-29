////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015 Argo Navis Technologies. All Rights Reserved.
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

#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <map>
#include <utility>

#include <ArgoNavis/Base/ThreadName.hpp>

namespace ArgoNavis { namespace CUDA { namespace Impl {

    /**
     * Table of partial events (kernel executions, data transfers, etc.)
     * contained within a data table. Partial events are those for which
     * either the enqueue or completion record has been seen. Not both.
     *
     * @tparam T    Type containing the information for the events.
     */
    template <typename T>
    class PartialEventTable
    {

    public:

        /**
         * Type used to indicate if an event was completed and, if so, the
         * thread in which the event occured and information for the event.
         */
        typedef boost::optional<std::pair<Base::ThreadName, T> > Completion;
        
        /** Construct an empty partial event table. */
        PartialEventTable() :
            dm_events(),
            dm_threads()
        {
        }

        /**
         * ...
         *
         * @param thread    ...
         * @param id        ...
         * @param event     ...
         * @return          ...
         */
        Completion enqueue(const Base::ThreadName& thread,
                           boost::uint32_t id, const T& event)
        {
            // ...
        }
        
        /**
         * ...
         *
         * @param thread    ...
         * @param id        ...
         * @param event     ...
         * @return          ...
         */
        Completion completed(boost::uint32_t id, const T& event)
        {
            // ...
        }
        
    private:

        /** Partial event for each known correlation ID. */
        std::map<boost::uint32_t, T> dm_events;

        /** Thread for each known correlation ID. */
        std::map<boost::uint32_t, Base::ThreadName> dm_threads;
        
    }; // class PartialEventTable<T>

} } } // namespace ArgoNavis::CUDA::Impl
