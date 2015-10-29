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

/** @file Definition of the AddressSpaces class. */

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/tuple/tuple.hpp>
#include <cstring>
#include <set>

#include <ArgoNavis/Base/AddressSpaces.hpp>

using namespace ArgoNavis::Base;
using namespace ArgoNavis::Base::Impl;



/** Anonymous namespace hiding implementation details. */
namespace {

    /**
     * Visitor used to determine if an arbitrary set of visited mappings
     * contains a mapping equivalent to the given mapping. The visitation
     * is terminated as soon as such a mapping is found.
     */
    bool containsEquivalentMapping(const ThreadName& thread,
                                   const LinkedObject& linked_object,
                                   const AddressRange& range,
                                   const TimeInterval& interval,
                                   const ThreadName& x_thread,
                                   const LinkedObject& x_linked_object,
                                   const AddressRange& x_range,
                                   const TimeInterval& x_interval,
                                   bool& contains)
    {
        contains |= ((thread == x_thread) &&
                     (linked_object.getFile() == x_linked_object.getFile()) &&
                     (range == x_range) &&
                     (interval == x_interval));
        return !contains;
    }
    
    /**
     * Visitor for determining if the given address spaces contain a mapping
     * equivalent to all mappings in an arbitrary set of visited mappings.
     * The visitation is terminated as soon as a mapping is encountered that
     * doesn't have an equivalent in the given address spaces.
     */
    bool containsAllMappings(const AddressSpaces& address_spaces,
                             const ThreadName& x_thread,
                             const LinkedObject& x_linked_object,
                             const AddressRange& x_range,
                             const TimeInterval& x_interval,
                             bool& contains)
    {
        bool contains_this = false;

        address_spaces.visitMappings(
            x_thread, x_range, x_interval, boost::bind(
                containsEquivalentMapping,
                x_thread, x_linked_object, x_range, x_interval,
                _1, _2, _3, _4,
                boost::ref(contains_this)
                )
            );
        
        contains &= contains_this;
        return contains;
    }
    
} // namespace <anonymous>



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSpaces::AddressSpaces() :
    dm_linked_objects(),
    dm_mappings()
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSpaces::operator CBTF_Protocol_AttachedToThreads() const
{
    std::set<ThreadName> threads;
    
    for (MappingIndex::const_iterator
             i = dm_mappings.begin(), iEnd = dm_mappings.end(); i != iEnd; ++i)
    {
        threads.insert(i->dm_thread);
    }

    CBTF_Protocol_AttachedToThreads message;
    memset(&message, 0, sizeof(message));

    message.threads.names.names_len = threads.size();
    message.threads.names.names_val =
        reinterpret_cast<CBTF_Protocol_ThreadName*>(
            malloc(std::max(1U, message.threads.names.names_len) *
                   sizeof(CBTF_Protocol_ThreadName))
            );

    u_int m = 0;
    for (std::set<ThreadName>::const_iterator
             i = threads.begin(); i != threads.end(); ++i, ++m)
    {
        CBTF_Protocol_ThreadName& entry = message.threads.names.names_val[m];
        entry = *i;
    }
    
    return message;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSpaces::operator std::vector<CBTF_Protocol_LinkedObjectGroup>() const
{
    std::set<ThreadName> threads;

    for (MappingIndex::const_iterator
             i = dm_mappings.begin(), iEnd = dm_mappings.end(); i != iEnd; ++i)
    {
        threads.insert(i->dm_thread);
    }

    std::vector<CBTF_Protocol_LinkedObjectGroup> message(threads.size());

    u_int m = 0;
    for (std::set<ThreadName>::const_iterator
             i = threads.begin(); i != threads.end(); ++i, ++m)
    {
        CBTF_Protocol_LinkedObjectGroup& entry = message[m];
        entry.thread = *i;

        entry.linkedobjects.linkedobjects_len = dm_mappings.get<0>().count(*i);
        entry.linkedobjects.linkedobjects_val =
            reinterpret_cast<CBTF_Protocol_LinkedObject*>(
                malloc(std::max(1U, entry.linkedobjects.linkedobjects_len) *
                       sizeof(CBTF_Protocol_LinkedObject))
                );        

        u_int n = 0;
        for (MappingIndex::nth_index<0>::type::const_iterator
                 j = dm_mappings.get<0>().lower_bound(*i),
                 jEnd = dm_mappings.get<0>().upper_bound(*i);
             j != jEnd;
             ++j, ++n)
        {
            CBTF_Protocol_LinkedObject& subentry = 
                entry.linkedobjects.linkedobjects_val[n];
            
            subentry.linked_object = j->dm_linked_object.getFile();
            subentry.range = j->dm_range;
            subentry.time_begin = j->dm_interval.begin();
            subentry.time_end = j->dm_interval.end() + 1;
            subentry.is_executable = false;
        }
    }
    
    return message;
}


        
//------------------------------------------------------------------------------
// Iterate over each mapping in the given CBTF_Protocol_LinkedObjectGroup and
// construct the corresponding entries in the specified thread's address space.
// Existing linked objects are used when found and new linked objects are added
// as necessary.
//------------------------------------------------------------------------------
void AddressSpaces::apply(const CBTF_Protocol_LinkedObjectGroup& message)
{
    for (u_int i = 0; i < message.linkedobjects.linkedobjects_len; ++i)
    {
        const CBTF_Protocol_LinkedObject& entry =
            message.linkedobjects.linkedobjects_val[i];
        
        FileName file = entry.linked_object;
        
        std::map<FileName, LinkedObject>::const_iterator j = 
            dm_linked_objects.find(file);
        
        if (j == dm_linked_objects.end())
        {
            j = dm_linked_objects.insert(
                std::make_pair(file, LinkedObject(file))
                ).first;
        }
        
        dm_mappings.insert(
            Mapping(ThreadName(message.thread), j->second, entry.range,
                    TimeInterval(entry.time_begin, entry.time_end - 1))
            );
    }
}



//------------------------------------------------------------------------------
// Iterate over each thread in the given CBTF_Protocol_LoadedLinkedObject and
// call loadLinkedObject() with values provided in the message.
//------------------------------------------------------------------------------
void AddressSpaces::apply(const CBTF_Protocol_LoadedLinkedObject& message)
{
    for (u_int i = 0; i < message.threads.names.names_len; ++i)
    {
        load(ThreadName(message.threads.names.names_val[i]),
             LinkedObject(message.linked_object),
             message.range, message.time);
    }
}



//------------------------------------------------------------------------------
// Iterate over each thread in the given CBTF_Protocol_UnloadedLinkedObject and
// call unloadLinkedObject() with values provided in the message.
//------------------------------------------------------------------------------
void AddressSpaces::apply(const CBTF_Protocol_UnloadedLinkedObject& message)
{
    for (u_int i = 0; i < message.threads.names.names_len; ++i)
    {
        unload(ThreadName(message.threads.names.names_val[i]),
               LinkedObject(message.linked_object),
               message.time);
    }
}



//------------------------------------------------------------------------------
// Construct a LinkedObject for the given CBTF_Protocol_SymbolTable, and then
// add it to these address spaces if the linked object isn't already present,
// or replace the existing linked object if it is already present. Replacement
// is somewhat complicated because not only does the (indexed) list of linked
// objects need to be updated, but all of the relevant mappings as well.
//------------------------------------------------------------------------------
void AddressSpaces::apply(const CBTF_Protocol_SymbolTable& message)
{
    FileName file = message.linked_object;
    
    std::map<FileName, LinkedObject>::iterator i = dm_linked_objects.find(file);
    
    if (i == dm_linked_objects.end())
    {
        dm_linked_objects.insert(std::make_pair(file, LinkedObject(message)));
    }
    else
    {
        LinkedObject linked_object(message);
        
        for (MappingIndex::nth_index<1>::type::iterator 
                 j = dm_mappings.get<1>().lower_bound(i->second),
                 jEnd = dm_mappings.get<1>().upper_bound(i->second);
             j != jEnd;
             ++j)
        {
            dm_mappings.get<1>().replace(
                j, Mapping(j->dm_thread, linked_object,
                           j->dm_range, j->dm_interval)
                );
        }
        
        i->second = linked_object;
    }
}



//------------------------------------------------------------------------------
// Construct a Mapping from the given values and add it to these address spaces.
// Existing linked objects are used when found and new linked objects are added
// as necessary.
//------------------------------------------------------------------------------
void AddressSpaces::load(const ThreadName& thread,
                         const LinkedObject& linked_object,
                         const AddressRange& range,
                         const Time& when)
{
    std::map<FileName, LinkedObject>::const_iterator i = 
        dm_linked_objects.find(linked_object.getFile());
    
    if (i == dm_linked_objects.end())
    {
        i = dm_linked_objects.insert(
            std::make_pair(linked_object.getFile(), linked_object)
            ).first;
    }
    
    dm_mappings.insert(
        Mapping(thread, i->second, range, TimeInterval(when, Time::TheEnd()))
        );
}



//------------------------------------------------------------------------------
// Locate all of the mappings for this thread that reference the linked object
// being unloaded, and that have an end time that is the last possible time, and
// update those end times to be the given time.
//------------------------------------------------------------------------------
void AddressSpaces::unload(const ThreadName& thread,
                           const LinkedObject& linked_object,
                           const Time& when)
{
    for (MappingIndex::nth_index<2>::type::iterator
             i = dm_mappings.get<2>().lower_bound(
                 boost::make_tuple(thread, linked_object)
                 ),
             iEnd = dm_mappings.get<2>().upper_bound(
                 boost::make_tuple(thread, linked_object)
                 );
         i != iEnd;
         ++i)
    {
        if (i->dm_interval.end() == Time::TheEnd())
        {
            dm_mappings.get<2>().replace(
                i, Mapping(i->dm_thread, i->dm_linked_object, i->dm_range,
                           TimeInterval(i->dm_interval.begin(), when))
                );
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void AddressSpaces::visitThreads(const ThreadVisitor& visitor) const
{
    bool terminate = false;
    std::set<ThreadName> visited;
    
    for (MappingIndex::const_iterator
             i = dm_mappings.begin(), iEnd = dm_mappings.end();
         !terminate && (i != iEnd);
         ++i)
    {
        if (visited.find(i->dm_thread) == visited.end())
        {
            visited.insert(i->dm_thread);
            terminate |= !visitor(i->dm_thread);
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void AddressSpaces::visitLinkedObjects(const LinkedObjectVisitor& visitor) const
{
    bool terminate = false;

    for (std::map<FileName, LinkedObject>::const_iterator
             i = dm_linked_objects.begin(), iEnd = dm_linked_objects.end();
         !terminate && (i != iEnd);
         ++i)
    {
        terminate |= !visitor(i->second);
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void AddressSpaces::visitLinkedObjects(const ThreadName& thread,
                                       const LinkedObjectVisitor& visitor) const
{
    bool terminate = false;
    std::set<LinkedObject> visited;

    for (MappingIndex::nth_index<0>::type::const_iterator
             i = dm_mappings.get<0>().lower_bound(thread),
             iEnd = dm_mappings.get<0>().upper_bound(thread);
         !terminate && (i != iEnd);
         ++i)
    {
        if (visited.find(i->dm_linked_object) == visited.end())
        {
            visited.insert(i->dm_linked_object);
            terminate |= !visitor(i->dm_linked_object);
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void AddressSpaces::visitMappings(const MappingVisitor& visitor) const
{
    bool terminate = false;

    for (MappingIndex::const_iterator
             i = dm_mappings.begin(), iEnd = dm_mappings.end();
         !terminate && (i != iEnd);
         ++i)
    {
        terminate |= !visitor(i->dm_thread, i->dm_linked_object,
                              i->dm_range, i->dm_interval);
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void AddressSpaces::visitMappings(const ThreadName& thread,
                                  const MappingVisitor& visitor) const
{
    bool terminate = false;

    for (MappingIndex::nth_index<0>::type::const_iterator
             i = dm_mappings.get<0>().lower_bound(thread),
             iEnd = dm_mappings.get<0>().upper_bound(thread);
         !terminate && (i != iEnd);
         ++i)
    {
        terminate |= !visitor(i->dm_thread, i->dm_linked_object,
                              i->dm_range, i->dm_interval);
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void AddressSpaces::visitMappings(const ThreadName& thread,
                                  const AddressRange& range,
                                  const TimeInterval& interval,
                                  const MappingVisitor& visitor) const
{
    bool terminate = false;

    for (MappingIndex::nth_index<0>::type::const_iterator
             i = dm_mappings.get<0>().lower_bound(thread),
             iEnd = dm_mappings.get<0>().upper_bound(thread);
         !terminate && (i != iEnd);
         ++i)
    {
        if (i->dm_range.intersects(range) &&
            i->dm_interval.intersects(interval))
        {
            terminate |= !visitor(thread, i->dm_linked_object,
                                  i->dm_range, i->dm_interval);
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool ArgoNavis::Base::equivalent(const AddressSpaces& first,
                                 const AddressSpaces& second)
{
    // Is "second" missing any of the mappings from "first"?
    
    bool contains = true;
    
    first.visitMappings(
        boost::bind(
            containsAllMappings, second, _1, _2, _3, _4, boost::ref(contains)
            )
        );
    
    if (!contains)
    {
        return false;
    }
    
    // Is "first" missing any of the mappings from "second"?

    contains = true;
    
    second.visitMappings(
        boost::bind(
            containsAllMappings, first, _1, _2, _3, _4, boost::ref(contains)
            )
        );
    
    if (!contains)
    {
        return false;
    }
    
    // Otherwise the address spaces are equivalent
    return true;
}
