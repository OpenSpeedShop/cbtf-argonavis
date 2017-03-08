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

#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <cmath>
#include <map>
#include <set>
#include <stddef.h>
#include <utility>

#include <ArgoNavis/Base/PeriodicSamplesGroup.hpp>

using namespace ArgoNavis::Base;



/** Anonymous namespace hiding implementation details. */
namespace {

    /** Type used to hold accumulated single values. */
    typedef std::map<
        Time, std::pair<boost::uint64_t /* Count */, boost::uint64_t /* Sum */>
        > AccumulatedData;

    /** Typed used to hold a flattened PeriodicSamplesGroup. */
    typedef std::map<Time, std::vector<boost::uint64_t> > FlattenedData;
    
    /** Visitor used to accumulate single values. */
    bool accumulate(const Time& time,
                    const std::vector<boost::uint64_t>& values,
                    AccumulatedData& data)
    {
        AccumulatedData::iterator i = data.find(time);

        if (i == data.end())
        {
            i = data.insert(std::make_pair(time, std::make_pair(0, 0))).first;
        }

        i->second.first += 1;       
        i->second.second += values[0];

        return true;
    }

    /** Visitor used to flatten a PeriodicSamplesGroup. */
    bool flatten(const Time& time,
                 const std::vector<boost::uint64_t>& values,
                 const std::size_t& n, const std::size_t& N,
                 FlattenedData& data)
    {
        FlattenedData::iterator i = data.find(time);

        if (i == data.end())
        {
            i = data.insert(
                std::make_pair(time, std::vector<boost::uint64_t>(N, 0))
                ).first;            
        }

        i->second[n] = values[0];
        
        return true;
    }
    
} // namespace <anonymous>



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
boost::uint64_t ArgoNavis::Base::getTotalSampleCount(
    const PeriodicSamplesGroup& group
    )
{
    boost::uint64_t result = 0;

    for (PeriodicSamplesGroup::const_iterator
             i = group.begin(), i_end = group.end(); i != i_end; ++i)
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
             i = group.begin(), i_end = group.end(); i != i_end; ++i)
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
             i = group.begin(), i_end = group.end(); i != i_end; ++i)
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
    const boost::optional<TimeInterval>& interval_,
    const boost::optional<Time>& rate_
    )
{
    TimeInterval interval = interval_ ? *interval_ :
        getSmallestTimeInterval(group);
    
    Time rate = rate_ ? *rate_ :
        Time(1000000 /* ms/ns */ * static_cast<boost::uint64_t>(
            round(static_cast<double>(getAverageSamplingRate(group)) /
                  1000000.0 /* ms/ns */)
            ));

    PeriodicSamplesGroup result;

    for (PeriodicSamplesGroup::const_iterator
             i = group.begin(), i_end = group.end(); i != i_end; ++i)
    {
        result.push_back(i->resample(interval, rate));
    }
    
    return result;    
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PeriodicSamplesGroup ArgoNavis::Base::getResampledAndCombined(
    const PeriodicSamplesGroup& group,
    const boost::optional<TimeInterval>& interval,
    const boost::optional<Time>& rate
    )
{
    typedef std::pair<std::string, PeriodicSamples::Kind> NameAndKind;
    typedef std::set<std::size_t> GroupIndex;

    PeriodicSamplesGroup resampled = getResampled(group, interval, rate);

    std::map<NameAndKind, GroupIndex> unique;

    for (std::size_t i = 0; i < resampled.size(); ++i)
    {
        NameAndKind key = std::make_pair(
            resampled[i].name(), resampled[i].kind()
            );
        
        std::map<NameAndKind, GroupIndex>::iterator j = unique.find(key);
        
        if (j == unique.end())
        {
            j = unique.insert(std::make_pair(key, GroupIndex())).first;
        }

        j->second.insert(i);
    }

    PeriodicSamplesGroup resampledAndCombined;

    for (std::map<NameAndKind, GroupIndex>::const_iterator
             i = unique.begin(), i_end = unique.end(); i != i_end; ++i)
    {
        AccumulatedData data;

        for (std::set<std::size_t>::const_iterator
                 j = i->second.begin(), j_end = i->second.end();
             j != j_end;
             ++j)
        {
            group[*j].visit(
                group[*j].interval(),
                boost::bind(accumulate, _1, _2, boost::ref(data))
                );
        }
        
        PeriodicSamples samples(i->first.first, i->first.second);
        
        if (i->first.second == PeriodicSamples::kCount)
        {
            for (AccumulatedData::const_iterator
                     k = data.begin(), k_end = data.end(); k != k_end; ++k)
            {
                samples.add(k->first /* Time */, k->second.second /* Sum */);
            }
        }
        else
        {
            for (AccumulatedData::const_iterator
                     k = data.begin(), k_end = data.end(); k != k_end; ++k)
            {
                samples.add(
                    k->first /* Time */,
                    k->second.second /* Sum */ / k->second.first /* Count */
                    );
            }
        }
        
        resampledAndCombined.push_back(samples);
    }
    
    return resampledAndCombined;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ArgoNavis::Base::visitConcurrently(
    const PeriodicSamplesGroup& group,
    const boost::optional<TimeInterval>& interval,
    const PeriodicSampleVisitor& visitor
    )
{
    FlattenedData data;

    for (std::size_t n = 0, N = group.size(); n < N; ++n)
    {
        group[n].visit(
            group[n].interval(),
            boost::bind(flatten, _1, _2,
                        boost::cref(n), boost::cref(N), boost::ref(data))
            );
    }

    bool terminate = false;

    for (FlattenedData::const_iterator i = data.begin(), i_end = data.end();
         !terminate && (i != i_end);
         ++i)
    {
        if (!interval || interval->contains(i->first))
        {
            terminate |= !visitor(i->first, i->second);
        }
    }
}
