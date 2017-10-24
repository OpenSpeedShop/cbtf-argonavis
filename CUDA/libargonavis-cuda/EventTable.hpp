////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015-2017 Argo Navis Technologies. All Rights Reserved.
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
#include <boost/bimap/set_of.hpp>
#include <boost/cstdint.hpp>
#include <map>
#include <set>
#include <utility>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/Raise.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

#include "EventClass.hpp"
#include "EventInstance.hpp"

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
            dm_actual(),
            dm_classes(),
            dm_instances()
        {
        }

        /** Add a new completed event to this table. */
        void add(const T& event)
        {
            dm_contexts.insert(event.context);

            typename Classes::right_const_iterator i = 
                dm_classes.right.find(event);

            if (i == dm_classes.right.end())
            {
                T copy(event);
                copy.clas = static_cast<boost::uint32_t>(dm_classes.size());

                i = dm_classes.right.insert(
                    std::make_pair(copy, copy.clas)
                    ).first;
            }

            EventInstance instance;

            instance.clas = i->second;
            instance.id = event.id;
            instance.time = event.time;
            instance.time_begin = event.time_begin;
            instance.time_end = event.time_end;

            dm_instances.insert(
                std::make_pair(
                    Base::TimeInterval(instance.time_begin, instance.time_end),
                    instance
                    )
                );
        }

        /** Add an existing event class to this table. */
        void addClass(T& event)
        {
            dm_contexts.insert(event.context);

            boost::uint32_t original = event.clas;
            
            typename Classes::right_const_iterator i = 
                dm_classes.right.find(event);

            if (i == dm_classes.right.end())
            {
                event.clas = static_cast<boost::uint32_t>(dm_classes.size());
                
                i = dm_classes.right.insert(
                    std::make_pair(event, event.clas)
                    ).first;
            }

            dm_actual.insert(std::make_pair(original, event.clas));
        }

        /** Add an existing event instance to this table. */
        void addInstance(EventInstance& instance)
        {
            std::map<boost::uint32_t, boost::uint32_t>::const_iterator i =
                dm_actual.find(instance.clas);

            if (i == dm_actual.end())
            {
                Base::raise<std::runtime_error>(
                    "Encountered unknown event class UID %1%.", instance.clas
                    );
            }

            instance.clas = i->second;

            dm_instances.insert(
                std::make_pair(
                    Base::TimeInterval(instance.time_begin, instance.time_end),
                    instance
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

            typename Instances::const_iterator i =
                dm_instances.lower_bound(interval.begin());

            if (i != dm_instances.begin())
            {
                --i;
            }

            typename Instances::const_iterator i_end =
                dm_instances.upper_bound(interval.end());
            
            for (; !terminate && (i != i_end); ++i)
            {
                if (i->first.intersects(interval))
                {
                    typename Classes::left_const_iterator j = 
                        dm_classes.left.find(i->second.clas);

                    if (j == dm_classes.left.end())
                    {
                        Base::raise<std::runtime_error>(
                            "Encountered unknown event class UID %1%.",
                            i->second.clas
                            );
                    }

                    T event = j->second;
                    event.clas = i->second.clas;
                    event.id = i->second.id;
                    event.time = i->second.time;
                    event.time_begin = i->second.time_begin;
                    event.time_end = i->second.time_end;

                    terminate |= !visitor(event);
                }
            }
        }

        /**
         * Visit all of the event classes in this table.
         *
         * @tparam V    Type of visitor for the event classes.
         *
         * @param visitor    Visitor invoked for each event
         *                   class contained within this table.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        template <typename V>
        void visitClasses(const V& visitor) const
        {
            bool terminate = false;

            typename Classes::left_const_iterator i = dm_classes.left.begin();
            typename Classes::left_const_iterator i_end = dm_classes.left.end();

            for (; !terminate && (i != i_end); ++i)
            {
                terminate |= !visitor(i->second);
            }
        }

        /**
         * Visit all of the event instances in this table.
         *
         * @tparam V    Type of visitor for the event instances.
         *
         * @param visitor    Visitor invoked for each event
         *                   instance contained within this table.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        template <typename V>
        void visitInstances(const V& visitor) const
        {
            bool terminate = false;

            typename Instances::const_iterator i = dm_instances.begin();            
            typename Instances::const_iterator i_end = dm_instances.end();

            for (; !terminate && (i != i_end); ++i)
            {
                terminate |= !visitor(i->second);
            }
        }

    private:

        /** Type of container used to store the known event classes. */
        typedef boost::bimap<
            boost::uint32_t, boost::bimaps::set_of<T, EventClass<T> >
            > Classes;

        /** Type of container used to store the known event instances. */
        typedef std::map<Base::TimeInterval, EventInstance> Instances;

        /** All known context addresses. */
        std::set<Base::Address> dm_contexts;

        /** Mapping of existing to actual unique identifiers. */
        std::map<boost::uint32_t, boost::uint32_t> dm_actual;

        /** Event class for each known unique identifer. */
        Classes dm_classes;
        
        /** Event instances indexed by their time interval. */
        Instances dm_instances;
         
    }; // class EventTable<T>

} } } // namespace ArgoNavis::CUDA::Impl
