////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Krell Institute. All Rights Reserved.
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

/** @file Declaration of the DyninstResolver class. */

#pragma once

#include <ArgoNavis/Base/AddressSet.hpp>
#include <ArgoNavis/Base/AddressSpaces.hpp>
#include <ArgoNavis/Base/LinkedObject.hpp>
#include <ArgoNavis/Base/Resolver.hpp>

namespace ArgoNavis { namespace Base {

    /**
     * Concrete implementation of Resolver using Dyninst to resolve symbols.
     *
     * @sa http://www.dyninst.org
     */
    class DyninstResolver :
        public Resolver
    {

    public:

        /**
         * Construct a Dyninst resolver for the given address spaces.
         *
         * @param spaces    Address spaces for which to resolve addresses.
         *
         * @note    Because the resolver keeps a reference (rather than a
         *          handle) to the address spaces, the caller must insure
         *          the resolver is released before the address spaces.
         */
        DyninstResolver(AddressSpaces& spaces);

        /** Destructor. */
        virtual ~DyninstResolver();
        
    protected:

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

        // ...
        
    }; // class DyninstResolver
       
} } // namespace ArgoNavis::Base

