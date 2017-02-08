////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016,2017 Argo Navis Technologies. All Rights Reserved.
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

/** @file Definition of the PeriodicSamples class. */

#include <cmath>

#include <ArgoNavis/Base/PeriodicSamples.hpp>

using namespace ArgoNavis::Base;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PeriodicSamples::PeriodicSamples(const std::string& name, Kind kind) :
    dm_name(name),
    dm_kind(kind),
    dm_samples()
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
TimeInterval PeriodicSamples::interval() const
{
    if (dm_samples.empty())
    {
        return TimeInterval();
    }

    return TimeInterval(dm_samples.begin()->first, dm_samples.rbegin()->first);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Time PeriodicSamples::rate() const
{
    boost::uint64_t sum = 0, n = 0;
    Time previous;
    
    for (std::map<Time, boost::uint64_t>::const_iterator
             i = dm_samples.begin(); i != dm_samples.end(); ++i)
    {
        if (i != dm_samples.begin())
        {
            sum += i->first - previous;
        }
        
        previous = i->first;
    }

    return Time(static_cast<boost::uint64_t>(round(
        static_cast<double>(sum) / static_cast<double>(n)
        )));
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PeriodicSamples::add(const Time& time, boost::uint64_t value)
{
    dm_samples[time] = value;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PeriodicSamples PeriodicSamples::resample(
    const boost::optional<Time>& rate
    ) const
{
    Time actual = rate ? *rate :
        Time(1000000 /* ms/ns */ * static_cast<boost::uint64_t>(
            round(static_cast<double>(this->rate()) / 1000000.0 /* ms/ns */)
            ));
    
    // ...

    return *this;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PeriodicSamples::visit(const TimeInterval& interval,
                            const PeriodicSampleVisitor& visitor) const
{
    std::vector<boost::uint64_t> samples(1);
    bool terminate = false;
    
    for (std::map<Time, boost::uint64_t>::const_iterator
             i = dm_samples.lower_bound(interval.begin()),
             i_end = dm_samples.upper_bound(interval.end());
         !terminate && (i != i_end);
         ++i)
    {
        samples[0] = i->second;
        terminate |= !visitor(i->first, samples);
    }
}
