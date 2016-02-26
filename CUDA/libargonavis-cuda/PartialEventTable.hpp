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

#include <boost/bimap.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <map>
#include <stddef.h>
#include <utility>
#include <vector>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/Raise.hpp>
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
            dm_correlations(),
            dm_devices(),
            dm_events()
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
            
            Contexts::left_const_iterator i = dm_contexts.left.find(context);
            
            if (i == dm_contexts.left.end())
            {
                dm_contexts.left.insert(std::make_pair(context, device));
                
                Devices::const_iterator j = dm_devices.find(device);
                
                if (j != dm_devices.end())
                {
                    complete(completions, context, j->second);
                }
            }
            
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

            Devices::const_iterator i = dm_devices.find(device);

            if (i == dm_devices.end())
            {
                dm_devices.insert(std::make_pair(device, index));

                Contexts::right_const_iterator j = 
                    dm_contexts.right.find(device);
                
                if (j != dm_contexts.right.end())
                {
                    complete(completions, j->second, index);
                }
            }
            
            return completions;
        }
        
        /**
         * Add the enqueuing of an event.
         *
         * @param id         Correlation ID of the event.
         * @param event      Partial information for this event.
         * @param context    Address of the context for this event.
         * @param thread     Thread in which this event occurred.
         * @return           Any events which are completed by this addition.
         */
        Completions addEnqueued(boost::uint32_t id, const T& event,
                                const Base::Address& context,
                                const Base::ThreadName& thread)
        {
            Completions completions;

            typename Events::iterator i = dm_events.find(id);
            
            if (i == dm_events.end())
            {
                i = dm_events.insert(std::make_pair(id, PartialEvent())).first;
            }
            else if (i->second.dm_enqueuing)
            {
                Base::raise<std::runtime_error>(
                    "Encountered multiple enqueuings of the event with "
                    "correlation ID %1%.", id
                    );
            }
            
            i->second.dm_enqueuing = event;
            i->second.dm_context = context;
            i->second.dm_thread = thread;

            dm_correlations.insert(Correlations::value_type(context, id));
            
            complete(completions, i);
            
            return completions;
        }
        
        /**
         * Add the completion of an event.
         *
         * @param id         Correlation ID of the event.
         * @param event      Partial information for this event.
         * @return           Any events which are completed by this addition.
         */
        Completions addCompleted(boost::uint32_t id, const T& event)
        {
            Completions completions;

            typename Events::iterator i = dm_events.find(id);
            
            if (i == dm_events.end())
            {
                i = dm_events.insert(std::make_pair(id, PartialEvent())).first;
            }
            else if (i->second.dm_completion)
            {
                Base::raise<std::runtime_error>(
                    "Encountered multiple completions of the event with "
                    "correlation ID %1%.", id
                    );
            }

            i->second.dm_completion = event;
            
            complete(completions, i);
            
            return completions;
        }

        /** Get the device ID for the given context address. */
        boost::uint32_t device(const Base::Address& context) const
        {            
            Contexts::left_const_iterator i = dm_contexts.left.find(context);

            if (i == dm_contexts.left.end())
            {
                Base::raise<std::invalid_argument>(
                    "Unknown context address %1%.", context
                    );
            }

            return i->second;
        }

        /**
         * Get the index within DataTable::dm_devices for the given device ID.
         */
        std::size_t index(boost::uint32_t device) const
        {
            Devices::const_iterator i = dm_devices.find(device);

            if (i == dm_devices.end())
            {
                Base::raise<std::invalid_argument>(
                    "Unknown device ID %1%.", device
                    );
            }
            
            return i->second;
        }
        
    private:

        /** Structure containing information about a single partial event. */
        struct PartialEvent
        {
            /** Partial enqueuing information for this event. */
            boost::optional<T> dm_enqueuing;
            
            /** Partial completion information for this event. */
            boost::optional<T> dm_completion;

            /** Address of the context in which this event occurred. */
            boost::optional<Base::Address> dm_context;

            /** Thread in which this event occurred. */
            boost::optional<Base::ThreadName> dm_thread;
        };
        
        /** Type of container used to store the known context addresses. */
        typedef boost::bimap<Base::Address, boost::uint32_t> Contexts;
        
        /** Type of container used to store the known correlation IDs. */
        typedef boost::bimap<
            boost::bimaps::multiset_of<Base::Address>, boost::uint32_t
            > Correlations;

        /** Type of container used to store the known device IDs. */
        typedef std::map<boost::uint32_t, std::size_t> Devices;

        /** Type of container used to store the partial events. */
        typedef std::map<boost::uint32_t, PartialEvent> Events;

        /** Complete partial events for the specified context if possible. */
        void complete(Completions& completions,
                      const Base::Address& context, std::size_t index)
        {
            std::vector<boost::uint32_t> completed;

            for (Correlations::left_const_iterator
                     i = dm_correlations.left.lower_bound(context),
                     i_end = dm_correlations.left.upper_bound(context);
                 i != i_end;
                 ++i)
            {
                typename Events::iterator j = dm_events.find(i->second);
                
                if ((j == dm_events.end()) ||
                    !j->second.dm_enqueuing ||
                    !j->second.dm_completion ||
                    !j->second.dm_context ||
                    !j->second.dm_thread)
                {
                    continue;
                }
                
                T event = *(j->second.dm_completion);
                event.device = index;
                event.call_site = j->second.dm_enqueuing->call_site;
                event.context = j->second.dm_enqueuing->context;
                event.stream = j->second.dm_enqueuing->stream;
                event.time = j->second.dm_enqueuing->time;
            
                completions.push_back(
                    std::make_pair(*(j->second.dm_thread), event)
                    );

                completed.push_back(j->first);
            }

            for (std::vector<boost::uint32_t>::const_iterator
                     i = completed.begin(); i != completed.end(); ++i)
            {
                dm_correlations.right.erase(*i);
                dm_events.erase(*i);
            }
        }

        /** Complete the specified partial event if possible. */
        void complete(Completions& completions,
                      const typename std::map<
                          boost::uint32_t, PartialEvent
                          >::iterator& i)
        {
            if (!i->second.dm_enqueuing ||
                !i->second.dm_completion ||
                !i->second.dm_context ||
                !i->second.dm_thread)
            {
                return;
            }

            Contexts::left_const_iterator j = 
                dm_contexts.left.find(*(i->second.dm_context));
            
            if (j == dm_contexts.left.end())
            {
                return;
            }

            Devices::const_iterator k = dm_devices.find(j->second);

            if (k == dm_devices.end())
            {
                return;
            }

            T event = *(i->second.dm_completion);
            event.device = k->second;
            event.call_site = i->second.dm_enqueuing->call_site;
            event.context = i->second.dm_enqueuing->context;
            event.stream = i->second.dm_enqueuing->stream;
            event.time = i->second.dm_enqueuing->time;
            
            completions.push_back(
                std::make_pair(*(i->second.dm_thread), event)
                );

            dm_correlations.right.erase(i->first);
            dm_events.erase(i);
        }

        /** Device ID for each known context address. */
        Contexts dm_contexts;

        /** Correlation ID(s) for each known context address. */
        Correlations dm_correlations;
        
        /** Index within DataTable::dm_devices for each known device ID. */
        Devices dm_devices;
        
        /** Partial event for each known correlation ID. */
        Events dm_events;
        
    }; // class PartialEventTable<T>

} } } // namespace ArgoNavis::CUDA::Impl
