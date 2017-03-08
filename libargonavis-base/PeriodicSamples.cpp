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
             i = dm_samples.begin(), i_end = dm_samples.end(); i != i_end; ++i)
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
    const boost::optional<TimeInterval>& interval_,
    const boost::optional<Time>& rate_
    ) const
{
    TimeInterval interval = interval_ ? *interval_ : this->interval();

    Time rate = rate_ ? *rate_ : Time(
        1000000 /* ms/ns */ * static_cast<boost::uint64_t>(
            round(static_cast<double>(this->rate()) / 1000000.0 /* ms/ns */)
            )
        );
    
#if defined(DEBUG_RESAMPLING)
    std::cout << std::endl;
    std::cout << "PeriodicSamples::resample("
              << (interval_ ? debug(*interval_) : "<none>") << ", "
              << (rate_ ? debug(*rate_) : "<none>")
              << ")" << std::endl;
    std::cout << "           name = " << dm_name;
    std::cout << "           kind = ";
    switch (dm_kind)
    {
    case kCount: std::cout << "Count"; break;
    case kPercentage: std::cout << "Percentage"; break;
    case kRate: std::cout << "Rate"; break;
    }
    std::cout << std::endl;
    std::cout << "           size = " << dm_samples.size() << std::endl;
    std::cout << "     interval() = " << debug(this->interval()) << std::endl;
    std::cout << "         rate() = " << debug(this->rate()) << std::endl;
    std::cout << "       interval = " << debug(interval) << std::endl;    
    std::cout << "           rate = " << debug(rate) << std::endl;
    std::cout << std::endl;
#endif

    return (dm_kind == kCount) ?
        resampleDeltas(interval, rate) :
        resampleValues(interval, rate);
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
PeriodicSamples PeriodicSamples::resampleDeltas(const TimeInterval& interval,
                                                const Time& rate) const
{
    // Compute the number of new samples
    boost::uint64_t N = (interval.width() + rate - 1) / rate;
    
#if defined(DEBUG_RESAMPLING)
    std::cout << "              N = " << N << std::endl;
    
    int debug_count = 10;
#endif

    std::vector<std::uint64_t> values(N, 0);

    // Iterate over each of the original samples
    
    Time tprevious = Time::TheBeginning();
    boost::uint64_t vprevious = 0;

    for (std::map<Time, boost::uint64_t>::const_iterator
             i = dm_samples.begin(), i_end = dm_samples.end(); i != i_end; ++i)
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
        boost::uint64_t j_begin = (tprevious - interval.begin()) / rate;
        boost::uint64_t j_end = 1 + ((i->first - interval.begin()) / rate);

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
            std::cout << "        j_begin = " << j_begin << std::endl;
            std::cout << "          j_end = " << j_end << std::endl;
        }
#endif
        
        // Iterate over each new sample covering this original sample
        for (boost::uint64_t j = j_begin; j < j_end; ++j)
        {
            // Compute the time range covered by this new sample
            TimeInterval nue(interval.begin() + Time(j * rate),
                             interval.begin() + Time(((j + 1) * rate) - 1));
            
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
        
    // Construct and return the final resampled PeriodicSamples
    
    PeriodicSamples resampled(dm_name, dm_kind);
   
    for (boost::uint64_t n = 0; n < N; ++n)
    {
        resampled.add(interval.begin() + Time(n * rate), values[n]);
    }
    
#if defined(DEBUG_RESAMPLING)
    boost::uint64_t sum_original = 0, sum_resampled = 0;

    for (std::map<Time, boost::uint64_t>::const_iterator
             i = dm_samples.begin(), i_end = dm_samples.end(); i != i_end; ++i)
    {
        sum_original += i->second;
    }

    for (std::map<Time, boost::uint64_t>::const_iterator
             i = resampled.dm_samples.begin(),
             i_end = resampled.dm_samples.end();
         i != i_end;
         ++i)
    {
        sum_resampled += i->second;
    }
    
    if (sum_resampled != sum_original)
    {
        std::cout << "WARNING: The sum of the resampled values ("
                  << sum_resampled
                  << ") was not equal to the sum of the original values ("
                  << sum_original
                  << ")!" << std::endl;
    }
#endif
    
    return resampled;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PeriodicSamples PeriodicSamples::resampleValues(const TimeInterval& interval,
                                                const Time& rate) const
{
    // Compute the number of new samples
    boost::uint64_t N = (interval.width() + rate - 1) / rate;

#if defined(DEBUG_RESAMPLING)
    std::cout << "              N = " << N << std::endl;
    
    int debug_count = 10;
#endif

    // Construct and return the final resampled PeriodicSamples
    
    PeriodicSamples resampled(dm_name, dm_kind);
   
    for (boost::uint64_t n = 0; n < N; ++n)
    {
#if defined(DEBUG_RESAMPLING)
        bool do_debug = (n == 0) || (n == (N - 1)) || (debug_count-- > 0);
#endif

        // Compute the time of this new sample
        Time t = interval.begin() + Time(n * rate);

        // Compute the value of this new sample

        boost::uint64_t v = 0;
        
        std::map<Time, boost::uint64_t>::const_iterator min =
            dm_samples.lower_bound(t);

        std::map<Time, boost::uint64_t>::const_iterator max =
            dm_samples.upper_bound(t);

        if (max == dm_samples.begin())
        {
            v = dm_samples.begin()->second;
        }
        else if (min == dm_samples.end())
        {
            v = dm_samples.rbegin()->second;
        }
        else
        {
            if ((min != dm_samples.begin()) && (min->first != t))
            {
                --min;
            }
            
            double w = static_cast<double>(t - min->first) /
                static_cast<double>(max->first - min->first);
            
            v = static_cast<boost::uint64_t>(
                round(w * static_cast<double>(min->second) +
                      (1.0 - w) * static_cast<double>(max->second))
                );
        }
        
#if defined(DEBUG_RESAMPLING)
        if (do_debug)
        {
            std::cout << std::endl;
            std::cout << "              n = " << n << std::endl;
            std::cout << "              t = " << debug(t) << std::endl;
            std::cout << "            min = ";
            if (min == dm_samples.end())
            {
                std::cout << "<end>";
            }
            else
            {
                std::cout << min->first << ", " << min->second;
            }
            std::cout << std::endl;
            std::cout << "            max = ";
            if (max == dm_samples.end())
            {
                std::cout << "<end>";
            }
            else
            {
                std::cout << max->first << ", " << max->second;
            }
            std::cout << std::endl;
            std::cout << "              v = " << v << std::endl;
        }
#endif
        
        resampled.add(t, v);
    }

    return resampled;
}
