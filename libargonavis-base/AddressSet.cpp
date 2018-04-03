////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013,2018 Krell Institute. All Rights Reserved.
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

/** @file Definition of the AddressSet class. */

#include <algorithm>
#include <deque>

#include <ArgoNavis/Base/AddressSet.hpp>

using namespace ArgoNavis::Base;



/** Anonymous namespace hiding implementation details. */
namespace {

    /**
     * Extract all contiguous address ranges within the given address bitmaps.
     *
     * @param bitmaps    Address bitmaps to be extracted.
     * @return           Contiguous address ranges in those address bitmaps.
     */
    std::set<AddressRange> extract(const std::vector<AddressBitmap>& bitmaps)
    {
        std::set<AddressRange> ranges;
        
        for (std::vector<AddressBitmap>::const_iterator
                 i = bitmaps.begin(); i != bitmaps.end(); ++i)
        {
            std::set<AddressRange> i_ranges = i->ranges(true);
            ranges.insert(i_ranges.begin(), i_ranges.end());
        }
        
        return ranges;
    }
    
    /**
     * Partition address ranges into address bitmaps. Addresses for functions
     * and statements are stored as pairings of an address range and a bitmap,
     * one bit per address in the range, that describe which addresses within
     * the range are associated with the function or statement. In the common
     * case where the addresses exhibit a high degree of spatial locality, a
     * single address range and bitmap is very effective. But there are cases,
     * such as inlined functions, where the degree of spatial locality can be
     * minimal. Under such circumstances, a single bitmap can grow very large
     * and it is more space efficient to use multiple bitmaps that individually
     * exhibit spatial locality. This function iteratively subdivides all the
     * addresses until each bitmap exhibits sufficient spatial locality.
     *
     * @param ranges    Address ranges to be partitioned.
     * @return          Address bitmaps representing these address ranges.
     *
     * @note    The criteria for subdividing an address set is as follows. The
     *          widest gap (spacing) between two adjacent addresses within the
     *          set is found. If the number of bits required to encode the gap
     *          within a bitmap is greater than the number of bits required to
     *          create a new address bitmap, the set is partitioned at the gap.
     */
    std::vector<AddressBitmap> partition(const std::set<AddressRange>& ranges)
    {
        std::vector<AddressBitmap> bitmaps;

        //
        // Set the partitioning criteria as the minimum number of bits required
        // for the binary representation of a CBTF_Protocol_AddressBitmap that
        // contains a single address. The size of this structure, rather than
        // that of the AddressBitmap class, is used because the main reason for
        // the partitioning is to minimize the eventual binary representation
        // of CBTF_Protocol_SymbolTable objects.
        // 
        
        const boost::int64_t kPartitioningCriteria = 8 /* Bits/Byte */ *
            (2 * sizeof(boost::uint64_t) /* Address Range */ +
             sizeof(boost::uint8_t) /* Single-Byte Bitmap */);
        
        // Convert the provided set of address ranges into a set of addresses
        std::set<Address> addresses;
        for (std::set<AddressRange>::const_iterator
                 i = ranges.begin(); i != ranges.end(); ++i)
        {
            for (Address j = i->begin(); j <= i->end(); ++j)
            {
                addresses.insert(j);
            }
        }
        
        //
        // Initialize a queue with this set of address and iterate over that
        // queue until it has been emptied.
        //
        
        std::deque<std::set<Address> > queue(1, addresses);        
        while (!queue.empty())
        {
            std::set<Address> i = queue.front();
            queue.pop_front();

            // Handle the special case of an empty address set by ignoring it
            if (i.empty())
            {
                continue;
            }

            //
            // Handle the special case of a single-element address set by
            // creating an address bitmap for it and adding that bitmap to
            // the results.
            //

            if (i.size() == 1)
            {
                bitmaps.push_back(AddressBitmap(i));
                continue;
            }

            //
            // Otherwise find the widest gap between any two adjacent addresses
            // within this address set. Also remember WHERE that widest gap was
            // located.
            //

            boost::int64_t widest_gap = 0;
            std::set<Address>::const_iterator widest_gap_at = i.begin();
            
            for (std::set<Address>::const_iterator
                     prev = i.begin(), current = ++i.begin();
                 current != i.end();
                 ++prev, ++current)
            {
                boost::int64_t gap = AddressRange(*prev, *current).width() - 1;
                
                if (gap > widest_gap)
                {
                    widest_gap = gap;
                    widest_gap_at = current;
                }
            }
            
            //
            // If the widest gap exceeds the partitioning criteria, partition
            // this address set at the gap and push both partitions onto the
            // queue. Otherwise create an address bitmap for this address set
            // and add that bitmap to the results.
            //
            
            if (widest_gap > kPartitioningCriteria)
            {
                queue.push_back(std::set<Address>(i.begin(), widest_gap_at));
                queue.push_back(std::set<Address>(widest_gap_at, i.end()));
            }
            else
            {
                bitmaps.push_back(AddressBitmap(i));
            }
            
        }

        // Return the final results to the caller        
        return bitmaps;
    }

} // namespace <anonymous>



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSet::AddressSet() :
    dm_bitmaps()
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSet::AddressSet(const Address& address)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSet::AddressSet(const AddressRange& range)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSet::AddressSet(const AddressBitmap& bitmap)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSet::AddressSet(const CBTF_Protocol_AddressBitmap& message)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSet::AddressSet(CBTF_Protocol_AddressBitmap* messages, u_int len) :
    dm_bitmaps()
{
    for (u_int i = 0; i < len; ++i)
    {
        dm_bitmaps.push_back(AddressBitmap(messages[i]));
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSet::operator std::set<AddressRange>() const
{
    return ::extract(dm_bitmaps);
}



AddressSet::operator std::string() const
{
    // ...

    return std::string();
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool AddressSet::operator==(const AddressSet& other) const
{
    return (*this ^ other).empty();
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void AddressSet::operator~()
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSet& AddressSet::operator|=(const AddressSet& other)
{
    // ...

    return *this;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSet& AddressSet::operator&=(const AddressSet& other)
{
    // ...

    return *this;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSet& AddressSet::operator^=(const AddressSet& other)
{
    // ...

    return *this;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSet& AddressSet::operator+=(const AddressSet& other)
{
    // ...

    return *this;
}



//------------------------------------------------------------------------------
// Construct a set of address ranges that contains all current address ranges
// for this set as well as the given new address ranges. Partition this new
// set of address ranges into address bitmaps. The new list of address bitmaps
// completely replaces any previous list.
//------------------------------------------------------------------------------
AddressSet& AddressSet::operator+=(const std::set<AddressRange>& ranges)
{
    std::set<AddressRange> all_ranges = ::extract(dm_bitmaps);
    all_ranges.insert(ranges.begin(), ranges.end());
    dm_bitmaps = ::partition(all_ranges);
    return *this;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressSet& AddressSet::operator-=(const AddressSet& other)
{
    // ...

    return *this;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool AddressSet::contains(const AddressSet& other) const
{
    return (*this | other) == *this;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool AddressSet::empty() const
{
    // ...

    return false;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void AddressSet::extract(CBTF_Protocol_AddressBitmap*& messages,
                         u_int& len) const
{
    len = dm_bitmaps.size();
    
    messages = reinterpret_cast<CBTF_Protocol_AddressBitmap*>(
        malloc(std::max(1U, len) * sizeof(CBTF_Protocol_AddressBitmap))
        );
    
    for (u_int i = 0; i < len; ++i)
    {
        messages[i] = dm_bitmaps[i];
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool AddressSet::intersects(const AddressSet& other) const
{
    return !(*this & other).empty();
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void AddressSet::visit(const AddressRangeVisitor& visitor) const
{    
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::ostream& ArgoNavis::Base::operator<<(std::ostream& stream,
                                          const AddressSet& set)
{
    // ...

    return stream;
}
