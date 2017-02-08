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

#include <boost/format.hpp>
#include <cmath>
#include <iostream>
#include <vector>

#include <ArgoNavis/Base/PeriodicSamples.hpp>

#define DEBUG_RESAMPLING

using namespace ArgoNavis::Base;



/** Anonymous namespace hiding implementation details. */
namespace {

    /** Convert a Time into a debugging string. */
    std::string debug(const Time& time)
    {
        return boost::str(boost::format("%u") %
                          static_cast<boost::uint64_t>(time));
    }

    /** Convert a TimeInterval into a debugging string. */
    std::string debug(const TimeInterval& interval)
    {
        return boost::str(boost::format("%u, %u") %
                          static_cast<boost::uint64_t>(interval.begin()) %
                          static_cast<boost::uint64_t>(interval.end()));
    }
    
} // namespace <anonymous>



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
    Time actual = rate ? *rate : Time(
        1000000 /* ms/ns */ * static_cast<boost::uint64_t>(
            round(static_cast<double>(this->rate()) / 1000000.0 /* ms/ns */)
            )
        );
    
#if defined(DEBUG_RESAMPLING)
    std::cout << std::endl;
    std::cout << "PeriodicSamples::resample("
              << (rate ? debug(*rate) : "<none>")
              << ")" << std::endl;
    std::cout << "           name = " << dm_name;
    std::cout << "           kind = ";
    switch (dm_kind)
    {
    case Count: std::cout << "Count"; break;
    case Percentage: std::cout << "Percentage"; break;
    case Rate: std::cout << "Rate"; break;
    }
    std::cout << std::endl;
    std::cout << "           size = " << dm_samples.size() << std::endl;
    std::cout << "     interval() = " << debug(interval()) << std::endl;
    std::cout << "         rate() = " << debug(this->rate()) << std::endl;
    std::cout << "         actual = " << debug(actual) << std::endl;
    std::cout << std::endl;
#endif

    switch (dm_kind)
    {
    case Count: return resampleSums(actual);
    case Percentage: return resampleAverages(actual);
    case Rate: return resampleAverages(actual);
    default: return resampleAverages(actual);
    }
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



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PeriodicSamples PeriodicSamples::resampleAverages(const Time& rate) const
{
    PeriodicSamples resampled(dm_name, dm_kind);

    // ...

    return resampled;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PeriodicSamples PeriodicSamples::resampleSums(const Time& rate) const
{
    // Determine the number of (re)samples

    boost::uint64_t b = interval().begin(), e = interval().end(), r = rate;
    
    TimeInterval all(r * (b / r), (r * ((1 + e) / r)) - 1);
    
    boost::uint64_t N = all.width() / r;
    
#if defined(DEBUG_RESAMPLING)
    std::cout << "            all = " << debug(all) << std::endl;
    std::cout << "              N = " << N << std::endl;
    
    int debug_count = 10;
#endif

    std::vector<std::uint64_t> values(N, 0);
    
    // Iterate over each of the original samples
    
    Time tprevious = all.begin();
    boost::uint64_t vprevious = 0;
    
    for (std::map<Time, boost::uint64_t>::const_iterator
             i = dm_samples.begin(); i != dm_samples.end(); ++i)
    {
#if defined(DEBUG_RESAMPLING)
        bool do_debug =
            (i == dm_samples.begin()) ||
            (i == --dm_samples.end()) ||
            (debug_count-- > 0);
#endif

        // Compute the time range covered by the original sample
        TimeInterval original(tprevious, i->first - 1);
        
        // Compute the deltas (time and values) for the original sample
        Time original_dt = original.width();
        boost::uint64_t original_dv = i->second - vprevious;

        // Compute the range of new samples covering this original sample
        boost::uint64_t jbegin = (tprevious - all.begin()) / r;
        boost::uint64_t jend = 1 + ((i->first - all.begin()) / r);

#if defined(DEBUG_RESAMPLING)
        if (do_debug)
        {
            std::cout << std::endl;
            
            if (i == dm_samples.begin())
            {
                std::cout << "              (First)" << std::endl;
            }
            
            if (i == --dm_samples.end())
            {
                std::cout << "              (Last)" << std::endl;
            }

            std::cout << "       i->first = " << debug(i->first) << std::endl;
            std::cout << "      i->second = " << i->second << std::endl;
            std::cout << "       original = " << debug(original) << std::endl;
            std::cout << "    original_dt = "
                      << debug(original_dt) << std::endl;
            std::cout << "    original_dv = " << original_dv << std::endl;
            std::cout << "         jbegin = " << jbegin << std::endl;
            std::cout << "           jend = " << jend << std::endl;
        }
#endif

        // Iterate over each new sample covering this original sample
        for (boost::uint64_t j = jbegin; j < jend; ++j)
        {
            // Compute the time range covered by this new sample
            TimeInterval nue(all.begin() + Time(j * r),
                             all.begin() + Time(((j + 1) * r) - 1));
            
            // Compute the intersection of the new and original samples
            TimeInterval intersection = original & nue;

#if defined(DEBUG_RESAMPLING)
            if (do_debug)
            {
                std::cout << "              j = " << j << std::endl;
                std::cout << "            nue = " << debug(nue) << std::endl;
                std::cout << "   intersection = "
                          << debug(intersection) << std::endl;
            }
#endif
            
            if (intersection.empty())
            {
                continue;
            }
            
            boost::uint64_t intersection_dt = intersection.width();            
            
            boost::uint64_t intersection_dv =
                (static_cast<boost::uint64_t>(original_dt) == 0) ?
                0 :
                static_cast<boost::uint64_t>(
                    round(static_cast<double>(original_dv) *
                          static_cast<double>(intersection_dt) /
                          static_cast<double>(original_dt))
                    );
            
#if defined(DEBUG_RESAMPLING)
            if (do_debug)
            {
                std::cout << "intersection_dt = "
                          << intersection_dt << std::endl;
                std::cout << "intersection_dv = "
                          << intersection_dv << std::endl;
            }
#endif
            
            values[j] += intersection_dv;
        }
    }

    // Construct and return the final, resampled, PeriodicSamples
    
    PeriodicSamples resampled(dm_name, dm_kind);
   
    for (boost::uint64_t n = 0; n < N; ++n)
    {
        resampled.add(all.begin() + Time(n * r), values[n]);
    }

#if defined(DEBUG_RESAMPLING)
    boost::uint64_t sum_original = 0, sum_resampled = 0;

    for (std::map<Time, boost::uint64_t>::const_iterator
             i = dm_samples.begin(); i != dm_samples.end(); ++i)
    {
        sum_original += i->second;
    }

    for (std::map<Time, boost::uint64_t>::const_iterator
             i = resampled.dm_samples.begin();
         i != resampled.dm_samples.end();
         ++i)
    {
        sum_resampled += i->second;
    }

    if (sum_resampled != sum_original)
    {
        std::cout << "WARNING: The sum of the resampled values ("
                  << sum_resampled
                  << ") was not equal to the sum of hte original values ("
                  << sum_original
                  << ")!" << std::endl;
    }    
#endif
    
    return resampled;
}
