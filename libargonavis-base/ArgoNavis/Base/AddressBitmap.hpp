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

/** @file Declaration of the AddressBitmap class. */

#pragma once

#include <boost/operators.hpp>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <KrellInstitute/Messages/Symbol.h>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/AddressRange.hpp>

namespace ArgoNavis { namespace Base {

    /**
     * A bitmap containing one bit per address within an address range. Used
     * to represent a non-contiguous, fragmented, portion of an address space.
     *
     * http://en.wikipedia.org/wiki/Bit_array
     */
    class AddressBitmap :
        public boost::equality_comparable<AddressBitmap>
    {
	
    public:

        /**
         * Construct an address bitmap, initially containing all "false"
         * values, for the specified address range.
         *
         * @param range    Address range covered by this address bitmap.
         */
        AddressBitmap(const AddressRange& range);

        /** Construct an address bitmap from a set of addresses. */
        AddressBitmap(const std::set<Address>& addresses);
        
        /** Construct an address bitmap from a CBTF_Protocol_AddressBitmap. */
        AddressBitmap(const CBTF_Protocol_AddressBitmap& message);
        
        /** Type conversion to a CBTF_Protocol_AddressBitmap. */
        operator CBTF_Protocol_AddressBitmap() const;

        /** Type conversion to a string. */
        operator std::string() const;

        /** Is this address bitmap equal to another one? */
        bool operator==(const AddressBitmap& other) const;
        
        /** Get the address range covered by this address bitmap. */
        const AddressRange& range() const;
        
        /**
         * Get the value of the given address in this address bitmap.
         *
         * @param address    Address to get.
         * @return           Value at that address.
         *
         * @throw std::invalid_argument    The given address isn't contained
         *                                 within this bitmap's range.
         */
        bool get(const Address& address) const;
        
        /**
         * Set the value of the given address in this address bitmap.
         *
         * @param address    Address to be set.
         * @param value      Value to set for this address.
         *
         * @throw std::invalid_argument    The given address isn't contained
         *                                 within this bitmap's range.
         */
        void set(const Address& address, bool value);
        
        /**
         * Get the set of contiguous address ranges in this address bitmap 
         * with the specified value.
         *
         * @param value    Value of interest.
         * @return         Set of contiguous address ranges with that value.
         */
        std::set<AddressRange> ranges(bool value) const;

    private:
        
        /** Address range covered by this address bitmap. */
        AddressRange dm_range;

        /** Contents of this address bitmap. */
        std::vector<bool> dm_bitmap;
        
    }; // class AddressBitmap

    /** Redirect an address bitmap to an output stream. */
    std::ostream& operator<<(std::ostream& stream, const AddressBitmap& bitmap);

} } // namespace ArgoNavis::Base
