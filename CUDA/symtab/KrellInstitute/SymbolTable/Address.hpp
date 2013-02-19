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

/** @file Declaration and definition of the Address class. */

#pragma once

#include <boost/assert.hpp>
#include <boost/format.hpp>
#include <boost/operators.hpp>
#include <cstdint>
#include <iostream>
#include <KrellInstitute/Messages/Address.h>
#include <limits>

namespace KrellInstitute { namespace SymbolTable {
        
    /**
     * All memory addresses are stored in 64-bit unsigned integers, allowing
     * for a unified representation of both 32-bit and 64-bit addresses, but
     * sacrificing storage efficiency when 32-bit addresses are processed.
     */
    class Address :
        public boost::addable<Address, int64_t>,
        public boost::totally_ordered<Address>
    {
        
    public:

        /** Construct the lowest possible address. */
        static Address TheLowest()
        {
            return Address(std::numeric_limits<uint64_t>::min());
        }
        
        /** Construct the highest possible address. */
        static Address TheHighest()
        {
            return Address(std::numeric_limits<uint64_t>::max());
        }
        
        /** Default constructor. */
        Address() :
            dm_value(0x0)
        {
        }

        /** Construct an address from a CBTF_Protocol_Address (uint64_t). */
        Address(const CBTF_Protocol_Address& message) :
            dm_value(message)
        {
        }
        
        /** Type conversion to a CBTF_Protocol_Address (uint64_t). */
        operator CBTF_Protocol_Address() const
        {
            return CBTF_Protocol_Address(dm_value);
        }

        /** Is this address less than another one? */
        bool operator<(const Address& other) const
        {
            return dm_value < other.dm_value;
        }
        
        /** Is this address equal to another one? */
        bool operator==(const Address& other) const
        {
            return dm_value == other.dm_value;
        }

        /** Add a signed offset to this address. */
        Address& operator+=(const int64_t& offset)
        {
            uint64_t result = dm_value + offset;
            BOOST_ASSERT((offset > 0) || (result <= dm_value));
            BOOST_ASSERT((offset < 0) || (result >= dm_value));
            dm_value += offset;
            return *this;
        }

        /** Subtract another address from this address. */
        int64_t operator-(const Address& other) const
        {
            int64_t difference = dm_value - other.dm_value;
            BOOST_ASSERT((*this > other) || (difference <= 0));
            BOOST_ASSERT((*this < other) || (difference >= 0));
            return difference;
        }
        
        /** Redirection to an output stream. */
        friend std::ostream& operator<<(std::ostream& stream,
                                        const Address& address)
        {
            stream << boost::str(boost::format(
                (address.dm_value > std::numeric_limits<uint32_t>::max()) ?
                "0x%016X" : "0x%08X"
                ) % dm_value);
            return stream;
        }
        
    private:
        
        /** Value of this address. */
        uint64_t dm_value;
        
    }; // class Address
        
} } // namespace KrellInstitute::SymbolTable
