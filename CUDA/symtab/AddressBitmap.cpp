////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 Silicon Graphics, Inc. All Rights Reserved.
// Copyright (c) 2012 Argo Navis Technologies. All Rights Reserved.
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

/** @file Definition of the AddressBitmap class. */

#include <algorithm>
#include <boost/assert.h>
#include <cstdint>
#include <cstdlib>

#include "AddressBitmap.hpp"

using namespace KrellInstitute::SymbolTable;
using namespace KrellInstitute::SymbolTable::Impl;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressBitmap::AddressBitmap(const AddressRange& range) :
    dm_range(range),
    dm_bitmap(dm_range.getWidth(), false)
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressBitmap::AddressBitmap(const CBTF_Protocol_AddressBitmap& message) :
    dm_range(message.range),
    dm_bitmap(AddressRange(message.range).getWidth(), false)
{
    uint64_t size = std::max<uint64_t>(1, ((dm_range.getWidth() - 1) / 8) + 1);

    BOOST_VERIFY(message.bitmap.data.data_len == size);
    
    for (uint64_t i = 0; i < dm_range.getWidth(); ++i)
    {
        dm_bitmap[i] = message.bitmap.data.data_val[i / 8] & (1 << (i % 8));
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressBitmap::operator CBTF_Protocol_AddressBitmap() const
{
    uint64_t size = std::max<uint64_t>(1, ((dm_range.getWidth() - 1) / 8) + 1);
    
    CBTF_Protocol_AddressBitmap message;
    message.range = dm_range;
    message.bitmap.data.data_len = size;
    message.bitmap.data.data_val = reinterpret_cast<uint8_t*>(malloc(size));

    memset(message.bitmap.data.data_val, 0, size);

    for (uint64_t i = 0; i < dm_range.getWidth(); ++i)
    {
        if (dm_bitmap[i])
        {
            message.bitmap.data.data_val[i / 8] |= 1 << (i % 8);
        }
    }

    return message;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const AddressRange& AddressBitmap::getRange() const
{
    return dm_range;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool getValue(const Address& address) const
{
    BOOST_VERIFY(dm_range.doesContain(address));

    return dm_bitmap[address - dm_range.getBegin()];
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void setValue(const Address& address, const bool& value)
{
    BOOST_VERIFY(dm_range.doesContain(address));

    dm_bitmap[address - dm_range.getBegin()] = value;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::set<AddressRange> AddressBitmap::getContiguousRanges(
    const bool& value
    ) const
{
    std::set<AddressRange> ranges;
    
    // Iterate over each address in this address bitmap
    bool in_range = false;
    Address range_begin;    
    for (Address i = dm_range.getBegin(); i != dm_range.getEnd(); ++i)
    {
        // Is this address the beginning of a range?
        if (!in_range && (getValue(i) == value))
        {
            in_range = true;
            range_begin = i;
        }
        
        // Is this address the end of a range?
        else if (in_range && (getValue(i) != value))
        {
            in_range = false;
            ranges.insert(AddressRange(range_begin, i));
        }	
    }
    
    // Does a range end at the end of the address bitmap?
    if (in_range)
    {
        ranges.insert(AddressRange(range_begin, dm_range.getEnd()));
    }
    
    // Return the ranges to the caller
    return ranges;
}


 
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::ostream& operator<<(std::ostream& stream, const AddressBitmap& bitmap)
{
    stream << bitmap.dm_range << " ";
    
    bool has_false = false, has_true = false;
    for (int64_t i = 0; i < bitmap.dm_range.getWidth(); ++i)
    {
        if (bitmap.getBitmap()[i] == true)
        {
            has_true = true;
        }
        else
        {
            has_false = true;
        }
    }
    
    if (has_false && !has_true)
    {
        stream << "0...0";
    }
    else if (!has_false && has_true)
    {
        stream << "1...1";
    }
    else
    {
        for (int64_t i = 0; i < bitmap.dm_range.getWidth(); ++i)
        {
            stream << (bitmap.dm_bitmap[i] ? "1" : "0");
        }
    }
    
    return stream;
}
