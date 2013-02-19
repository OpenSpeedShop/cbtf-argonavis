////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 Silicon Graphics, Inc. All Rights Reserved.
// Copyright (c) 2012 Argo Navis Technologies. All Rights Reserved.
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

#include <iostream>
#include <KrellInstitute/Messages/Symbol.h>
#include <KrellInstitute/SymbolTable/Address.hpp>
#include <KrellInstitute/SymbolTable/AddressRange.hpp>
#include <set>
#include <vector>

namespace KrellInstitute { namespace SymbolTable { namespace Impl {

    /**
     * A bitmap containing one bit per address within an address range. Used
     * to represent a non-contiguous, fragmented, portion of an address space.
     */
    class AddressBitmap
    {
	
    public:

        /**
         * Construct an address bitmap, initially containing all "false"
         * values, for the specified address range.
         *
         * @param range    Address range covered by this address bitmap.
         */
        AddressBitmap(const AddressRange& range);
        
        /**
         * Construct an address bitmap from a CBTF_Protocol_AddressBitmap.
         *
         * @param message    Message containing this address bitmap.
         */
        AddressBitmap(const CBTF_Protocol_AddressBitmap& message);
        
        /**
         * Type conversion to a CBTF_Protocol_AddressBitmap.
         *
         * @return    Message containing this address bitmap.
         */
        operator CBTF_Protocol_AddressBitmap() const;
        
        /**
         * Get the address range covered by this address bitmap.
         *
         * @return    Address range covered by this address bitmap.
         */
        const AddressRange& getRange() const;
        
        /**
         * Get the value of the specified address in this address bitmap.
         *
         * @param address    Address to get.
         * @return           Value at that address.
         */
        bool getValue(const Address& address) const;
        
        /**
         * Set the value of the specified address in this address bitmap.
         *
         * @param address    Address to be set.
         * @param value      Value to set for this address.
         */
        void setValue(const Address& address, const bool& value);
        
        /**
         * Get the set of contiguous address ranges in this address bitmap 
         * with the specified value.
         *
         * @param value    Value of interest.
         * @return         Set of contiguous address ranges with that value.
         */
        std::set<AddressRange> getContiguousRanges(const bool& value) const;

        /**
         * Redirection to an output stream.
         *
         * @param stream    Destination output stream.
         * @param bitmap    Address bitmap to be redirected.
         * @return          Destination output stream.
         */
        friend std::ostream& operator<<(std::ostream& stream,
                                        const AddressBitmap& bitmap);
        
    private:
        
        /** Address range covered by this address bitmap. */
        AddressRange dm_range;

        /** Contents of this address bitmap. */
        std::vector<bool> dm_bitmap;
        
    }; // class AddressBitmap
            
} } } // namespace KrellInstitute::SymbolTable::Impl
