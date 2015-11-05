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
#include <map>
#include <stddef.h>
#include <utility>
#include <vector>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>

namespace ArgoNavis { namespace CUDA { namespace Impl {
    
    /**
     * Table of partial events (kernel executions, data transfers, etc.)
     * contained within a data table. Partial events are those for which
     * all needed messages (e.g. enqueue, completion) haven't been seen.
     *
     * @tparam T    Type containing the information for the events.
     */
    template <typename T>
    class PartialEventTable
    {
        
    public:
        
        /**
         * Type used to return event completions. Each completion includes the
         * thread in which the event occurred and information for that event.
         */
        typedef std::vector<std::pair<Base::ThreadName, T> > Completions;
        
        /** Construct an empty partial event table. */
        PartialEventTable() :
            dm_contexts(),
            dm_devices(),
            dm_events(),
            dm_threads()
        {
        }
        
        /**
         * Add information about a context.
         *
         * @param context    Address of the context.
         * @param device     ID of the device for this context.
         * @return           Any events which are completed by this addition.
         */
        Completions addContext(const Base::Address& context,
                               boost::uint32_t device)
        {
            Completions completions;
            
            // ...
            
            return completions;
        }
        
        /**
         * Add information about a device.
         *
         * @param device    ID of the device.
         * @param index     Index within DataTable::dm_devices of this device.
         * @return          Any events which are completed by this addition.
         */
        Completions addDevice(boost::uint32_t device, std::size_t index)
        {
            Completions completions;
            
            // ...
            
            return completions;
        }
        
        /**
         * Add the enqueue of an event.
         *
         * @param id        Correlation ID of the event.
         * @param event     Partial information for this event.
         * @param thread    Thread in which this event occurred.
         * @return          Any events which are completed by this addition.
         */
        Completions addEnqueue(boost::uint32_t id, const T& event,
                               const Base::ThreadName& thread)
        {
            Completions completions;
            
            // ...
            
            return completions;
        }
        
        /**
         * Add the completion of an event.
         *
         * @param id         Correlation ID of the event.
         * @param event      Partial information for this event.
         * @param context    Address of the context for this event.
         * @return           Any events which are completed by this addition.
         */
        Completions addCompleted(boost::uint32_t id, const T& event,
                                 const Base::Address& context)
            
        {
            Completions completions;
            
            // ...
            
            return completions;
        }
        
    private:
        
        /** Device ID for each known context address. */
        std::map<Base::Address, boost::uint32_t> dm_contexts;

        /** Index within DataTable::dm_devices for each known device ID. */
        std::map<boost::uint32_t, std::size_t> dm_devices;
        
        /** Partial event for each known correlation ID. */
        std::map<boost::uint32_t, T> dm_events;

        /** Thread for each known correlation ID. */
        std::map<boost::uint32_t, Base::ThreadName> dm_threads;
        
    }; // class PartialEventTable<T>

} } } // namespace ArgoNavis::CUDA::Impl
