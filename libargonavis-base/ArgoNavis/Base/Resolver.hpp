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

/** @file Declaration of the Resolver class. */

#pragma once

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <map>

#include <ArgoNavis/Base/AddressSet.hpp>
#include <ArgoNavis/Base/AddressSpaces.hpp>
#include <ArgoNavis/Base/FileName.hpp>
#include <ArgoNavis/Base/LinkedObject.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

namespace ArgoNavis { namespace Base {

    /**
     * Abstract base class for a symbol table resolver that accepts addresses
     * and adds the corresponding source code constructs (functions, etc.) to
     * the appropriate linked object.
     */
    class Resolver :
        private boost::noncopyable
    {

    public:
        
        /** Destructor. */
        virtual ~Resolver();
        
        /**
         * Resolve all addresses in the given linked object.
         *
         * @param linked_object    Linked object to be resolved.
         */
        void operator()(const LinkedObject& linked_object);
        
        /**
         * Resolve specific addresses in the given thread and time interval.
         *
         * @param addresses    Addresses to be resolved.
         * @param thread       Name of the thread containing those addresses.
         * @param interval     Time interval over which to resolve addresses.
         */
        void operator()(const AddressSet& addresses,
                        const ThreadName& thread,
                        const TimeInterval& interval);
        
    protected:

        /**
         * Construct a resolver for the given address spaces.
         *
         * @param spaces    Address spaces for which to resolve addresses.
         *
         * @note    Because the resolver keeps a reference (rather than a
         *          handle) to the address spaces, the caller must insure
         *          the resolver is released before the address spaces.
         */
        Resolver(AddressSpaces& spaces);

        /**
         * Resolve specific addresses in the given linked object.
         *
         * @param addresses        Addresses to be resolved.
         * @param linked_object    Linked object containing those addresses.
         *
         * @note    The addresses specified are relative to the beginning of
         *          the linked object rather than absolute addresses from the
         *          address space of a specific process.
         */
        virtual void resolve(const AddressSet& addresses,
                             const LinkedObject& linked_object) = 0;
        
    private:
        
        /** Address spaces for which to resolve addresses. */
        AddressSpaces& dm_spaces;

        /** Indexed set of addresses already resolved. */
        std::map<FileName, AddressSet> dm_resolved;
        
    }; // class Resolver
       
} } // namespace ArgoNavis::Base
