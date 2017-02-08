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

/** @file Definition of the PeriodicSamplesGroup functions. */

#include <cmath>

#include <ArgoNavis/Base/PeriodicSamplesGroup.hpp>

using namespace ArgoNavis::Base;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
boost::uint64_t ArgoNavis::Base::getTotalSampleCount(
    const PeriodicSamplesGroup& group
    )
{
    boost::uint64_t result = 0;

    for (PeriodicSamplesGroup::const_iterator
             i = group.begin(); i != group.end(); ++i)
    {
        result += i->size();
    }
    
    return result;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
TimeInterval ArgoNavis::Base::getSmallestTimeInterval(
    const PeriodicSamplesGroup& group
    )
{
    TimeInterval result;

    for (PeriodicSamplesGroup::const_iterator
             i = group.begin(); i != group.end(); ++i)
    {
        result |= i->interval();
    }
    
    return result;    
}



//------------------------------------------------------------------------------
// Note that a weighted average is NOT being used here. Should it?
//------------------------------------------------------------------------------
Time ArgoNavis::Base::getAverageSamplingRate(const PeriodicSamplesGroup& group)
{
    boost::uint64_t result = 0;

    for (PeriodicSamplesGroup::const_iterator
             i = group.begin(); i != group.end(); ++i)
    {
        result += i->rate();
    }

    return Time(static_cast<boost::uint64_t>(round(
        static_cast<double>(result) / static_cast<double>(group.size())
        )));
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PeriodicSamplesGroup ArgoNavis::Base::getResampled(
    const PeriodicSamplesGroup& group,
    const boost::optional<Time>& rate
    )
{
    PeriodicSamplesGroup result;
    
    Time actual = rate ? *rate :
        Time(1000000 /* ms/ns */ * static_cast<boost::uint64_t>(
            round(static_cast<double>(getAverageSamplingRate(group)) /
                  1000000.0 /* ms/ns */)
            ));
    
    for (PeriodicSamplesGroup::const_iterator
             i = group.begin(); i != group.end(); ++i)
    {
        result.push_back(i->resample(actual));
    }
    
    return result;    
}
