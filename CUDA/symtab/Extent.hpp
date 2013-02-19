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

/** @file Declaration and definition of the Extent class. */

#pragma once

#include <boost/operators.hpp>
#include <iostream>
#include <KrellInstitute/SymbolTable/AddressRange.hpp>
#include <KrellInstitute/SymbolTable/TimeInterval.hpp>

namespace KrellInstitute { namespace SymbolTable {

    /**
     * An open-ended interval in two dimensions: time and address space. Used to
     * represent when and where a DSO, function, etc. is located.
     *
     * @sa http://en.wikipedia.org/wiki/Interval_(mathematics)
     */
    class Extent :
        public boost::andable<Extent>,
        public boost::orable<Extent>,
        public boost::totally_ordered<Extent>
    {

    public:
        
        /** Default constructor. */
        Extent() :
            dm_interval(),
            dm_range()
        {
        }

        /** Construct an extent from the time interval and address range. */
        Extent(const TimeInterval& interval, const AddressRange& range) :
            dm_interval(interval),
            dm_range(range)
        {
        }

        /** Is this extent less than another one? */
        bool operator<(const Extent& other) const
        {
            if (dm_interval < other.dm_interval)
            {
                return true;
            }
            if (dm_interval > other.dm_interval)
            {
                return false;
            }
            return dm_range < other.dm_range;
        }

        /** Is this extent equal to another one? */
        bool operator==(const Extent& other) const
        {
            return (dm_interval == other.dm_interval) &&
                (dm_range == other.dm_range);
        }

        /** Union this extent with another one. */
        Extent& operator|=(const Extent& other)
        {
            dm_interval |= other.dm_interval;
            dm_range |= other.dm_range;
            return *this;
        }
        
        /** Intersect this extent with another one. */
        Extent& operator&=(const Extent& other)
        {
            dm_interval &= other.dm_interval;
            dm_range &= other.dm_range;
            return *this;
        }
        
        /** Get the time interval of this extent. */
        const TimeInterval& getTimeInterval() const
        { 
            return dm_interval;
        }

        /** Get the address range of this extent. */
        const AddressRange& getAddressRange() const
        {
            return dm_range; 
        }
        
        /** Is this extent empty? */
        bool isEmpty() const
        {
            return dm_interval.isEmpty() || dm_range.isEmpty();
        }

        /** Does this extent intersect another extent? */
        bool doesIntersect(const Extent& other) const
        {
            return !(*this & extent).empty();
        }
        
        /** Redirection to an output stream. */
        friend std::ostream& operator<<(std::ostream& stream,
                                        const Extent& extent)
        {
            stream << extent.dm_interval << " x " << extent.dm_range;
            return stream;
        }
        
    private:

        /** Time interval of this extent. */
        TimeInterval dm_interval;
        
        /** Address range of this extent. */
        AddressRange dm_range;

    }; // class Extent
        
} } // namespace KrellInstitute::SymbolTable
