////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013,2017 Krell Institute. All Rights Reserved.
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

/** @file Declaration of the AddressSet class. */

#pragma once

#include <boost/cstdint.hpp>
#include <boost/operators.hpp>
#include <iostream>
#include <string>
#include <vector>

#include <KrellInstitute/Messages/Symbol.h>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/AddressRange.hpp>
#include <ArgoNavis/Base/AddressRangeVisitor.hpp>

namespace ArgoNavis { namespace Base {

    /**
     * A set of memory addresses. Used to represent a non-contiguous, possibly
     * large and/or fragmented, portion of an address space.
     *
     * @sa http://en.wikipedia.org/wiki/Set_data_structure
     */
    class AddressSet :
        public boost::additive<AddressSet>,
        public boost::bitwise<AddressSet>,
        public boost::equality_comparable<AddressSet>
    {
        
    public:

        /** Default constructor. */
        AddressSet();
        
        /** Construct an address set from an address. */
        AddressSet(const Address& address);
        
        /** Construct an address set from an address range. */
        AddressSet(const AddressRange& range);
        
        /**
         * Construct an address set from a CBTF_Protocol_AddressBitmap array.
         */
        AddressSet(CBTF_Protocol_AddressBitmap* messages, u_int len);
        
        /** Construct an address set from a CBTF_Protocol_AddressBitmap. */
        AddressSet(const CBTF_Protocol_AddressBitmap& message);
        
        /** Type conversion to a string. */
        operator std::string() const;
        
        /** Is this address set equal to another one? */
        bool operator==(const AddressSet& other) const;

        /** Negate this address set. */
        void operator~();

        /** Unite another address set with this one. */
        AddressSet& operator|=(const AddressSet& other);

        /** Intersect another address set with this one. */
        AddressSet& operator&=(const AddressSet& other);

        /** Symmetrically differentiate another address set with this one. */
        AddressSet& operator^=(const AddressSet& other);

        /** Add another address set to this one. */
        AddressSet& operator+=(const AddressSet& other);
        
        /** Remove another address set from this one. */
        AddressSet& operator-=(const AddressSet& other);

        /** Does this address set contain another one? */
        bool contains(const AddressSet& other) const;

        /** Is this address set empty? */
        bool empty() const;

        /** Extract an address set into a CBTF_Protocol_AddressBitmap array. */
        void extract(CBTF_Protocol_AddressBitmap*& messages, u_int& len) const;
        
        /** Does this address set intersect another one? */
        bool intersects(const AddressSet& other) const;
        
        /** Visit the contiguous address ranges in this address set. */
        void visit(const AddressRangeVisitor& visitor) const;
        
        /**
         * Redirection to an output stream.
         *
         * @param stream    Destination output stream.
         * @param set       Address set to be redirected.
         * @return          Destination output stream.
         */
        friend std::ostream& operator<<(std::ostream& stream,
                                        const AddressSet& set);
        
    private:

        /** Semi-opaque representation of this address set. */
        std::vector<boost::uint8_t> dm_data;
        
    }; // class AddressSet

} } // namespace ArgoNavis::Base
