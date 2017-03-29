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
#include <boost/optional.hpp>
#include <cmath>
#include <iostream>
#include <vector>

#include <ArgoNavis/Base/PeriodicSamples.hpp>

//#define DEBUG_RESAMPLING

using namespace ArgoNavis::Base;



/** Anonymous namespace hiding implementation details. */
namespace {

    /** Convert a Time into a debugging string. */
    std::string debug(const Time& time,
                      const boost::optional<Time>& origin = boost::none)
    {
        if (!origin)
        {
            return boost::str(boost::format("%u") %
                              static_cast<boost::uint64_t>(time));
        }       
        else if (time > *origin)
        {
            return boost::str(boost::format("+%u") %
                              (static_cast<boost::uint64_t>(time) -
                               static_cast<boost::uint64_t>(*origin)));
        }
        else if (time < *origin)
        {
            return boost::str(boost::format("-%u") %
                              (static_cast<boost::uint64_t>(*origin) -
                               static_cast<boost::uint64_t>(time)));
        }
        else
        {
            return "0";
        }        
    }

    /** Convert a TimeInterval into a debugging string. */
    std::string debug(const TimeInterval& interval,
                      const boost::optional<Time>& origin = boost::none)
    {
        return boost::str(boost::format("[%s, %s]") %
                          debug(interval.begin(), origin) %
                          debug(interval.end(), origin));
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
    if (dm_samples.size() < 2)
    {
        return Time();
    }
    
    boost::uint64_t sum = 0;
    Time previous = dm_samples.begin()->first;
    
    for (std::map<Time, boost::uint64_t>::const_iterator
             i = dm_samples.begin()++, i_end = dm_samples.end();
         i != i_end;
         ++i)
    {
        sum += i->first - previous;
        previous = i->first;
    }

    return Time(static_cast<boost::uint64_t>(round(
        static_cast<double>(sum) / static_cast<double>(dm_samples.size() - 1)
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
    std::cout << "           name = " << dm_name << std::endl;
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
    typedef std::map<Time, boost::uint64_t>::const_iterator IteratorType;
        
    // Compute the number of new samples
    boost::uint64_t N = ((interval.width() + rate - 1) / rate) + 1;

#if defined(DEBUG_RESAMPLING)
    Time origin = dm_samples.begin()->first;
    
    std::cout << "              N = " << N << std::endl;
    std::cout << "         origin = " << debug(origin) << std::endl;

    std::cout << std::endl;
    for (IteratorType i = dm_samples.begin(), i_end = dm_samples.end();
         i != i_end;
         ++i)
    {
        std::cout << "        original: " << debug(i->first, origin)
                  << " = " << i->second << std::endl;
    }
    
    int debug_count = 10;
#endif
    
    // Construct and return the final resampled PeriodicSamples
    
    PeriodicSamples resampled(dm_name, dm_kind);

    boost::uint64_t v = 0;
    
    for (boost::uint64_t n = 0; n < N; ++n)
    {
#if defined(DEBUG_RESAMPLING)
        bool do_debug = (n == 0) || (n == (N - 1)) || (debug_count-- > 0);
#endif

        // Compute the time of this new sample
        Time t = interval.begin() + Time(n * rate);
        
        // Compute the time range covered by this new sample
        TimeInterval nue(
            (n == 0) ?
                Time::TheBeginning() :
                interval.begin() + Time((n - 1) * rate) + 1,
            t
            );

#if defined(DEBUG_RESAMPLING)
        if (do_debug)
        {
            std::cout << std::endl;
            std::cout << "    ------->  n = " << n << std::endl;
            std::cout << "            nue = "
                      << debug(nue, origin) << std::endl;
        }
#endif
        
        // Compute the range of original samples covering this new sample
        
        IteratorType i_begin = dm_samples.lower_bound(nue.begin());
        IteratorType i_end = dm_samples.upper_bound(nue.end());

        if (i_begin != dm_samples.begin())
        {
            --i_begin;
        }
        
        if (i_end != dm_samples.end())
        {
            ++i_end;
        }
        
        // Iterate over each original sample covering this new sample
        for (IteratorType i = i_begin; i != i_end; ++i)
        {
            // Compute the time range covered by this original sample
            TimeInterval original(
                (i == dm_samples.begin()) ?
                    Time::TheBeginning() :
                    (--IteratorType(i))->first + 1,
                i->first
                );
            
            // Compute the value for the original sample
            boost::uint64_t total = i->second -
                ((i == dm_samples.begin()) ? 0 : (--IteratorType(i))->second);
            
            // Compute the weight value to be added to the new sample

            double weight = original.empty() ? 0.0 :
                (static_cast<double>((nue & original).width()) /
                 static_cast<double>(original.width()));

            boost::uint64_t value = static_cast<boost::uint64_t>(
                round(static_cast<double>(total) * weight)
                );

#if defined(DEBUG_RESAMPLING)
            if (do_debug)
            {
                std::cout << std::endl;
                std::cout << "       original = " << debug(original, origin)
                          << std::endl;
                std::cout << "          total = " << total << std::endl;
                std::cout << "         weight = " << weight << std::endl;
                std::cout << "          value = " << value << std::endl;
            }
#endif
                        
            // Add the weighted value to the new sample
            v += value;
        }

#if defined(DEBUG_RESAMPLING)
        if (do_debug)
        {
            std::cout << std::endl;
            std::cout << "              v = " << v << std::endl;
        }
#endif
        
        resampled.add(t, v);
    }
    
#if defined(DEBUG_RESAMPLING)
    std::cout << std::endl;    
    for (IteratorType i = resampled.dm_samples.begin(),
             i_end = resampled.dm_samples.end();
         i != i_end;
         ++i)
    {
        std::cout << "       resampled: " << debug(i->first, origin)
                  << " = " << i->second << std::endl;
    }

    boost::uint64_t sum_original = dm_samples.empty() ? 0 :
        (dm_samples.rbegin()->second - dm_samples.begin()->second);

    boost::uint64_t sum_resampled = resampled.dm_samples.empty() ? 0 :
        (resampled.dm_samples.rbegin()->second -
         resampled.dm_samples.begin()->second);
    
    if (sum_resampled != sum_original)
    {
        std::cout << std::endl
                  << "WARNING: The sum of the resampled values ("
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
    typedef std::map<Time, boost::uint64_t>::const_iterator IteratorType;
    
    // Compute the number of new samples
    boost::uint64_t N = ((interval.width() + rate - 1) / rate) + 1;

#if defined(DEBUG_RESAMPLING)
    Time origin = dm_samples.begin()->first;
    
    std::cout << "              N = " << N << std::endl;
    std::cout << "         origin = " << debug(origin) << std::endl;

    std::cout << std::endl;
    for (IteratorType i = dm_samples.begin(), i_end = dm_samples.end();
         i != i_end;
         ++i)
    {
        std::cout << "        original: " << debug(i->first, origin)
                  << " = " << i->second << std::endl;
    }
    
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

        IteratorType min = dm_samples.lower_bound(t);
        IteratorType max = dm_samples.upper_bound(t);

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
            
            double weight = static_cast<double>(t - min->first) /
                static_cast<double>(max->first - min->first);
            
            v = static_cast<boost::uint64_t>(
                round(weight * static_cast<double>(min->second) +
                      (1.0 - weight) * static_cast<double>(max->second))
                );
        }
        
#if defined(DEBUG_RESAMPLING)
        if (do_debug)
        {
            std::cout << std::endl;
            std::cout << "    ------->  n = " << n << std::endl;
            std::cout << "              t = " << debug(t, origin) << std::endl;
            std::cout << "            min = ";
            if (min == dm_samples.end())
            {
                std::cout << "<end>";
            }
            else
            {
                std::cout << debug(min->first, origin) << ", " << min->second;
            }
            std::cout << std::endl;
            std::cout << "            max = ";
            if (max == dm_samples.end())
            {
                std::cout << "<end>";
            }
            else
            {
                std::cout << debug(max->first, origin) << ", " << max->second;
            }
            std::cout << std::endl;
            std::cout << "              v = " << v << std::endl;
        }
#endif
        
        resampled.add(t, v);
    }

#if defined(DEBUG_RESAMPLING)
    std::cout << std::endl;
    for (IteratorType i = resampled.dm_samples.begin(),
             i_end = resampled.dm_samples.end();
         i != i_end;
         ++i)
    {
        std::cout << "       resampled: " << debug(i->first, origin)
                  << " = " << i->second << std::endl;
    }
#endif
    
    return resampled;
}
