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
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
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
         * Add the enqueuing of an event.
         *
         * @param id        Correlation ID of the event.
         * @param event     Partial information for this event.
         * @param thread    Thread in which this event occurred.
         * @return          Any events which are completed by this addition.
         */
        Completions addEnqueued(boost::uint32_t id, const T& event,
                                const Base::ThreadName& thread)
        {
            Completions completions;

            IteratorByID i = dm_events.template get<ByID>().find(id);

            if (i == dm_events.template get<ByID>().end())
            {
                i = dm_events.insert(
                    PartialEventIndexRow(id, event, thread)
                    ).first;
            }
            else if (i->dm_enqueuing)
            {
                Base::raise<std::runtime_error>(
                    "Encountered multiple enqueuings of the event with "
                    "correlation ID %1%.", id
                    );
            }
            else
            {
                dm_events.template get<ByID>().replace(
                    i, PartialEventIndexRow(*i, event, thread)
                    );
                
                complete(i, completions);
            }
            
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

            IteratorByID i = dm_events.template get<ByID>().find(id);

            if (i == dm_events.template get<ByID>().end())
            {
                i = dm_events.insert(
                    PartialEventIndexRow(id, event, context)
                    ).first;
            }
            else if (i->dm_completion)
            {
                Base::raise<std::runtime_error>(
                    "Encountered multiple completions of the event with "
                    "correlation ID %1%.", id
                    );
            }
            else
            {
                dm_events.template get<ByID>().replace(
                    i, PartialEventIndexRow(*i, event, context)
                    );
                
                complete(i, completions);
            }
            
            return completions;
        }
        
    private:

        /** Structure containing one row of a partial event index. */
        struct PartialEventIndexRow
        {
            /** Correlation ID of the event. */
            boost::uint32_t dm_id;

            /** Unknown context address for this event. */
            Base::Address dm_context;

            /** Unknown device ID for this event. */
            boost::uint32_t dm_device;
            
            /** Thread in which this event occurred. */
            boost::optional<Base::ThreadName> dm_thread;

            /** Partial enqueuing information for this event. */
            boost::optional<T> dm_enqueuing;

            /** Partial completion information for this event. */
            boost::optional<T> dm_completion;

            /** Constructor from the enqueuing of an event. */
            PartialEventIndexRow(boost::uint32_t id, const T& event,
                                 const Base::ThreadName& thread) :
                dm_id(id),
                dm_context(0),
                dm_device(-1),
                dm_thread(thread),
                dm_enqueuing(event),
                dm_completion()
            {
            }
            
            /** Constructor from the completion of an event. */
            PartialEventIndexRow(boost::uint32_t id, const T& event,
                                 const Base::Address& context) :
                dm_id(id),
                dm_context(context),
                dm_device(-1),
                dm_thread(),
                dm_enqueuing(),
                dm_completion(event)
            {
            }

            /** Copy constructor with update from the enqueuing of an event. */
            PartialEventIndexRow(const PartialEventIndexRow& other,
                                 const T& event,
                                 const Base::ThreadName& thread) :
                dm_id(other.dm_id),
                dm_context(other.dm_context),
                dm_device(other.dm_device),
                dm_thread(thread),
                dm_enqueuing(event),
                dm_completion(other.dm_completion)
            {
            }
            
            /** Copy constructor with update from the completion of an event. */
            PartialEventIndexRow(const PartialEventIndexRow& other,
                                 const T& event,
                                 const Base::Address& context) :
                dm_id(other.dm_id),
                dm_context(context),
                dm_device(other.dm_device),
                dm_thread(other.dm_thread),
                dm_enqueuing(other.dm_enqueuing),
                dm_completion(event)
            {
            }

            /** Copy constructor with unknown device ID update. */
            PartialEventIndexRow(const PartialEventIndexRow& other,
                                 const boost::uint32_t& device) :
                dm_id(other.dm_id),
                dm_context(other.dm_context),
                dm_device(device),
                dm_thread(other.dm_thread),
                dm_enqueuing(other.dm_enqueuing),
                dm_completion(other.dm_completion)
            {
            }
            
        }; // struct PartialEventIndexRow

        /** Tag for the correlation ID index of PartialEventIndex. */
        struct ByID { };
       
        /** Tag for the unknown context address index of PartialEventIndex. */
        struct ByContext { };
        
        /** Tag for the unknown device ID index of PartialEventIndex. */
        struct ByDevice { };

        /** Type of associative container used to search for partial events. */
        typedef boost::multi_index_container<
            PartialEventIndexRow,
            boost::multi_index::indexed_by<
                boost::multi_index::ordered_unique<
                    boost::multi_index::tag<ByID>,
                    boost::multi_index::member<
                        PartialEventIndexRow,
                        boost::uint32_t,
                        &PartialEventIndexRow::dm_id
                        >
                    >,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<ByContext>,
                    boost::multi_index::member<
                        PartialEventIndexRow,
                        Base::Address,
                        &PartialEventIndexRow::dm_context
                        >
                    >,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::tag<ByDevice>,
                    boost::multi_index::member<
                        PartialEventIndexRow,
                        boost::uint32_t,
                        &PartialEventIndexRow::dm_device
                        >
                    >
                >
            > PartialEventIndex;

        /** Type of iterator used when searching by correlation ID. */
        typedef typename PartialEventIndex::
           template index<ByID>::type::iterator IteratorByID;
        
        /** Type of iterator used when searching by unknown context address. */
        typedef typename PartialEventIndex::
           template index<ByContext>::type::iterator IteratorByContext;

        /** Type of iterator used when searching by unknown device ID. */
        typedef typename PartialEventIndex::
           template index<ByDevice>::type::iterator IteratorByDevice;

        /** Complete the specified partial event if possible. */
        void complete(const IteratorByID& i, Completions& completions)
        {
            if (!i->dm_enqueuing || !i->dm_completion)
            {
                return;
            }
            
            const std::map<Base::Address, boost::uint32_t>::const_iterator
                j = dm_contexts.find(i->dm_context);

            if (j == dm_contexts.end())
            {
                return;
            }

            const std::map<boost::uint32_t, std::size_t>::const_iterator
                k = dm_devices.find(j->second);
            
            if (k == dm_devices.end())
            {
                dm_events.template get<ByID>().replace(
                    i, PartialEventIndexRow(*i, j->second)
                    );
                
                return;
            }

            T event = *(i->dm_completion);
            event.device = k->second;
            event.call_site = i->dm_enqueuing->call_site;
            event.time = i->dm_enqueuing->time;

            completions.push_back(std::make_pair(*(i->dm_thread), event));
            
            dm_events.template get<ByID>().erase(i);
        }
        
        /** Device ID for each known context address. */
        std::map<Base::Address, boost::uint32_t> dm_contexts;

        /** Index within DataTable::dm_devices for each known device ID. */
        std::map<boost::uint32_t, std::size_t> dm_devices;

        /** Partial events in this table. */
        PartialEventIndex dm_events;

    }; // class PartialEventTable<T>

} } } // namespace ArgoNavis::CUDA::Impl
