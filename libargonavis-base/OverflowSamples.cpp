////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Argo Navis Technologies. All Rights Reserved.
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

/** @file Definition of the OverflowSamples class. */

#include <ArgoNavis/Base/OverflowSamples.hpp>

using namespace ArgoNavis::Base;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
OverflowSamples::OverflowSamples(const std::string& name) :
    dm_name(name),
    dm_samples()
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressRange OverflowSamples::range() const
{
    if (dm_samples.empty())
    {
        return AddressRange();
    }

    return AddressRange(dm_samples.begin()->first, dm_samples.rbegin()->first);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void OverflowSamples::add(const Address& address, boost::uint64_t value)
{
    dm_samples[address] = value;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void OverflowSamples::visit(const AddressRange& range,
                            const OverflowSampleVisitor& visitor) const
{
    std::vector<boost::uint64_t> samples(1);
    bool terminate = false;
    
    for (std::map<Address, boost::uint64_t>::const_iterator
             i = dm_samples.lower_bound(range.begin()),
             i_end = dm_samples.upper_bound(range.end());
         !terminate && (i != i_end);
         ++i)
    {
        samples[0] = i->second;
        terminate |= !visitor(i->first, samples);
    }
}
