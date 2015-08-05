////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013,2014 Krell Institute. All Rights Reserved.
// Copyright (c) 2015 Argo Navis Technologies. All Rights Reserved.
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
#include <boost/cstdint.hpp>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

#include <ArgoNavis/Base/AddressBitmap.hpp>
#include <ArgoNavis/Base/Raise.hpp>

using namespace ArgoNavis::Base;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressBitmap::AddressBitmap(const AddressRange& range) :
    dm_range(range),
    dm_bitmap(dm_range.width(), false)
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressBitmap::AddressBitmap(const std::set<Address>& addresses) :
    dm_range(*(addresses.begin()), *(addresses.rbegin())),
    dm_bitmap(dm_range.width(), false)
{
    for (std::set<Address>::const_iterator
             i = addresses.begin(); i != addresses.end(); ++i)
    {
        set(*i, true);
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressBitmap::AddressBitmap(const CBTF_Protocol_AddressBitmap& message) :
    dm_range(message.range),
    dm_bitmap(dm_range.width(), false)
{
    boost::uint64_t width = dm_range.width();    
    boost::uint64_t size = std::max<boost::uint64_t>(1, ((width - 1) / 8) + 1);
    
    BOOST_VERIFY(message.bitmap.data.data_len == size);
    
    for (boost::uint64_t i = 0; i < width; ++i)
    {
        dm_bitmap[i] = message.bitmap.data.data_val[i / 8] & (1 << (i % 8));
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressBitmap::operator CBTF_Protocol_AddressBitmap() const
{
    boost::uint64_t width = dm_range.width();    
    boost::uint64_t size = std::max<boost::uint64_t>(1, ((width - 1) / 8) + 1);

    CBTF_Protocol_AddressBitmap message;
    message.range = dm_range;
    message.bitmap.data.data_len = size;
    message.bitmap.data.data_val = reinterpret_cast<uint8_t*>(malloc(size));
    
    memset(message.bitmap.data.data_val, 0, size);

    for (boost::uint64_t i = 0; i < width; ++i)
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
AddressBitmap::operator std::string() const
{
    std::ostringstream stream;
    stream << *this;
    return stream.str();
}



//------------------------------------------------------------------------------
// Currently equality is determined by direct comparison of the address ranges
// and bitmaps. Not by whether the two address bitmaps contain identical "set"
// addresses. The later defintion might be more appropriate, but would also be
// more expensive to compute and was deemed unnecessary for now since the only
// place this is used, currently, is in the unit test.
//------------------------------------------------------------------------------
bool AddressBitmap::operator==(const AddressBitmap& other) const
{
    return (dm_range == other.dm_range) && (dm_bitmap == other.dm_bitmap);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const AddressRange& AddressBitmap::range() const
{
    return dm_range;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool AddressBitmap::get(const Address& address) const
{
    if (!dm_range.contains(address))
    {
        raise<std::invalid_argument>(
            "The given address (%1%) isn't contained within "
            "this bitmap's range (%2%).", address, dm_range
            );
    }
    
    return dm_bitmap[address - dm_range.begin()];
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void AddressBitmap::set(const Address& address, bool value)
{
    if (!dm_range.contains(address))
    {
        raise<std::invalid_argument>(
            "The given address (%1%) isn't contained within "
            "this bitmap's range (%2%).", address, dm_range
            );
    }
    
    dm_bitmap[address - dm_range.begin()] = value;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::set<AddressRange> AddressBitmap::ranges(bool value) const
{
    std::set<AddressRange> result;
    
    // Iterate over each address in this address bitmap
    bool in = false;
    Address begin;
    for (Address i = dm_range.begin(); i <= dm_range.end(); ++i)
    {
        // Is this address the beginning of a range?
        if (!in && (get(i) == value))
        {
            in = true;
            begin = i;
        }
        
        // Is this address the end of a range?
        else if (in && (get(i) != value))
        {
            in = false;
            result.insert(AddressRange(begin, i - 1));
        }
    }
    
    // Does a range end at the end of the address bitmap?
    if (in)
    {
        result.insert(AddressRange(begin, dm_range.end()));
    }
    
    // Return the resulting ranges to the caller
    return result;
}


 
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::ostream& ArgoNavis::Base::operator<<(std::ostream& stream,
                                          const AddressBitmap& bitmap)
{
    const AddressRange& range = bitmap.range();
    
    stream << range << ": ";
    
    bool has_false = false, has_true = false;

    for (Address i = range.begin(); i <= range.end(); ++i)
    {
        if (bitmap.get(i) == true)
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
        for (Address i = range.begin(); i <= range.end(); ++i)
        {
            stream << (bitmap.get(i) ? "1" : "0");
        }
    }
    
    return stream;
}
