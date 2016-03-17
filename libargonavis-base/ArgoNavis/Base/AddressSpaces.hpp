////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013 Krell Institute. All Rights Reserved.
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

/** @file Declaration of the AddressSpaces class. */

#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <map>
#include <vector>

#include <KrellInstitute/Messages/LinkedObjectEvents.h>
#include <KrellInstitute/Messages/Symbol.h>
#include <KrellInstitute/Messages/ThreadEvents.h>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/AddressRange.hpp>
#include <ArgoNavis/Base/FileName.hpp>
#include <ArgoNavis/Base/LinkedObject.hpp>
#include <ArgoNavis/Base/LinkedObjectVisitor.hpp>
#include <ArgoNavis/Base/MappingVisitor.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>
#include <ArgoNavis/Base/ThreadVisitor.hpp>
#include <ArgoNavis/Base/Time.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

namespace ArgoNavis { namespace Base {

    /**
     * In-memory address spaces of one or more threads.
     */
    class AddressSpaces
    {

    public:
        
        /** Construct empty address spaces. */
        AddressSpaces();

        /**
         * Type conversion to a CBTF_Protocol_AttachedToThreads.
         *
         * @return    Message containing the threads in these address spaces.
         */
        operator CBTF_Protocol_AttachedToThreads() const;

        /**
         * Type conversion to a list of CBTF_Protocol_LinkedObjectGroup.
         *
         * @return    List of messages containing these address spaces.
         */
        operator std::vector<CBTF_Protocol_LinkedObjectGroup>() const;
        
        /**
         * Apply the given message, describing an initial list of linked
         * objects, to these address spaces.
         *
         * @param message    Message describing an initial linked objects list.
         */
        void apply(const CBTF_Protocol_LinkedObjectGroup& message);
        
        /**
         * Apply the given message, describing the load of a linked object,
         * to these address spaces.
         *
         * @param message    Message describing a loaded linked object.
         */
        void apply(const CBTF_Protocol_LoadedLinkedObject& message);

        /**
         * Apply the given message, describing the unload of a linked object,
         * to these address spaces.
         *
         * @param message    Message describing an unloaded linked object.
         */
        void apply(const CBTF_Protocol_UnloadedLinkedObject& message);

        /**
         * Apply the given message, describing the symbol table of a linked
         * object, to these address spaces.
         *
         * @param message    Message describing a linked object's symbol table.
         */
        void apply(const CBTF_Protocol_SymbolTable& message);

        /**
         * Load the given linked object into the address space of a single
         * thread at the specified address range and time.
         *
         * @param thread           Name of the thread doing the loading.
         * @param linked_object    Linked object to be loaded.
         * @param range            Address range of this linked object.
         * @param when             Time when this linked object was loaded.
         */
        void load(const ThreadName& thread,
                  const LinkedObject& linked_object,
                  const AddressRange& range,
                  const Time& when = Time::TheBeginning());
        
        /**
         * Unload the given linked object from the address space of a single
         * thread at the specified time.
         *
         * @param thread           Name of the thread doing the unloading.
         * @param linked_object    Linked object to be unloaded.
         * @param when             Time when this linked object was unloaded.
         */
        void unload(const ThreadName& thread,
                    const LinkedObject& linked_object,
                    const Time& when = Time::TheEnd());
        
        /**
         * Visit the threads contained within these address spaces.
         *
         * @param visitor    Visitor invoked for each thread contained
         *                   within these address spaces.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        void visitThreads(const ThreadVisitor& visitor) const;

        /**
         * Visit the linked objects contained within these address spaces.
         *
         * @param visitor    Visitor invoked for each linked object contained
         *                   within these address spaces.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        void visitLinkedObjects(const LinkedObjectVisitor& visitor) const;
        
        /**
         * Visit the linked objects contained within these address spaces
         * mapped by the given thread.
         *
         * @param thread     Name of the thread to be found.
         * @param visitor    Visitor invoked for each linked object contained
         *                   within these address spaces mapped by that thread.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        void visitLinkedObjects(const ThreadName& thread,
                                const LinkedObjectVisitor& visitor) const;

        /**
         * Visit the mappings contained within these address spaces.
         *
         * @param visitor    Visitor invoked for each mapping contained
         *                   within these address spaces.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        void visitMappings(const MappingVisitor& visitor) const;

        /**
         * Visit the mappings contained within these address spaces mapped
         * by the given thread.
         *
         * @param thread     Name of the thread to be found.
         * @param visitor    Visitor invoked for each mapping contained
         *                   within these address spaces mapped by that
         *                   thread.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        void visitMappings(const ThreadName& thread,
                           const MappingVisitor& visitor) const;
        
        /**
         * Visit the mappings contained within this address space mapped by
         * the given thread and intersecting the given address range and time
         * interval.
         *
         * @param thread      Name of the thread to be found.
         * @param range       Address range to be found.
         * @param interval    Time interval to be found.
         * @param visitor     Visitor invoked for each mapping contained
         *                    within these address spaces mapped by that
         *                    thread and interesecting that address range
         *                    and time interval.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        void visitMappings(const ThreadName& thread,
                           const AddressRange& range,
                           const TimeInterval& interval,
                           const MappingVisitor& visitor) const;
        
    private:

        /** Structure representing one mapping in these address spaces. */
        struct Mapping
        {
            /** Name of the thread containing this mapping. */
            ThreadName dm_thread;
            
            /** Linked object being mapped. */
            LinkedObject dm_linked_object;

            /** Address range of the linked object within the address space. */
            AddressRange dm_range;

            /** Time interval for the linked object within the address space. */
            TimeInterval dm_interval;
            
            /** Constructor from initial fields. */
            Mapping(const ThreadName& thread,
                    const LinkedObject& linked_object,
                    const AddressRange& range,
                    const TimeInterval& interval) :
                dm_thread(thread),
                dm_linked_object(linked_object),
                dm_range(range),
                dm_interval(interval)
            {
            }
            
        }; // struct Mapping

        /**
         * Type of associative container used to search for the mappings for a
         * given thread, linked object, or overlapping a given address range or
         * time interval.
         */
        typedef boost::multi_index_container<
            Mapping,
            boost::multi_index::indexed_by<
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::member<
                        Mapping, ThreadName, &Mapping::dm_thread
                        >
                    >,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::member<
                        Mapping, LinkedObject, &Mapping::dm_linked_object
                        >
                    >,
                boost::multi_index::ordered_non_unique<
                    boost::multi_index::composite_key<
                        Mapping,
                        boost::multi_index::member<
                            Mapping, ThreadName, &Mapping::dm_thread
                            >,
                        boost::multi_index::member<
                            Mapping, LinkedObject, &Mapping::dm_linked_object
                            >                        
                        >
                    >
                >
            > MappingIndex;

        /** Indexed list of linked objects in these address spaces. */
        std::map<FileName, LinkedObject> dm_linked_objects;

        /** Indexed list of mappings in these address spaces. */
        MappingIndex dm_mappings;

    }; // class AddressSpaces

    /**
     * Are the two given address spaces equivalent?
     *
     * @param first     First address spaces to be compared.
     * @param second    Second address spaces to be compared.
     * @return          Boolean "true" if the two address spaces are
     *                  equivalent, or "false" otherwise.
     */
    bool equivalent(const AddressSpaces& first, const AddressSpaces& second);
       
} } // namespace ArgoNavis::Base
