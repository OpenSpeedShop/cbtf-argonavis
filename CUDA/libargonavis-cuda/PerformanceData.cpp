////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014,2015 Argo Navis Technologies. All Rights Reserved.
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

/** @file Definition of the PerformanceData class. */

#include <boost/assert.hpp>
#include <ArgoNavis/CUDA/PerformanceData.hpp>
#include <map>
#include <stddef.h>

#include "DataTable.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::CUDA;
using namespace ArgoNavis::CUDA::Impl;



/** Anonymous namespace hiding implementation details. */
namespace {

    /** Type of container used to store periodic samples. */
    typedef std::map<
        boost::uint64_t, std::vector<boost::uint64_t>
        > PeriodicSamples;

    /** Find the smallest range of samples enclosing the given interval. */
    bool find(const PeriodicSamples& samples, const TimeInterval& interval,
              PeriodicSamples::const_iterator& min,
              PeriodicSamples::const_iterator& max)
    {
        if (samples.empty())
        {
            return false;
        }
        
        TimeInterval clamped = interval & 
            TimeInterval(samples.begin()->first, samples.rbegin()->first);
        
        if (clamped.empty())
        {
            return false;
        }
        
        min = samples.lower_bound(clamped.begin());
        max = samples.upper_bound(clamped.end());
        
        if ((min != samples.begin()) && (Time(min->first) != clamped.begin()))
        {
            --min;
        }

        if (max != samples.begin())
        {
            --max;
            if (Time(max->first) != clamped.end())
            {
                ++max;
            }
        }

        return true;
    }
    
} // namespace <anonymous>



//------------------------------------------------------------------------------
// Iterate over each of the individual CUDA messages that are "packed" into the
// specified performance data. For all of the messages containing stack traces
// or sampled PC addresses, invoke the given visitor.
//------------------------------------------------------------------------------
void PerformanceData::visitPCs(const CBTF_cuda_data& message,
                               AddressVisitor& visitor)
{
    bool terminate = false;
    
    for (u_int i = 0; !terminate && (i < message.messages.messages_len); ++i)
    {
        const CBTF_cuda_message& raw = message.messages.messages_val[i];
        
        switch (raw.type)
        {
            
        case EnqueueExec:
            {
                const CUDA_EnqueueExec& msg = 
                    raw.CBTF_cuda_message_u.enqueue_exec;
                
                for (uint32_t i = msg.call_site;
                     !terminate &&
                         (i < message.stack_traces.stack_traces_len) &&
                         (message.stack_traces.stack_traces_val[i] != 0);
                     ++i)
                {
                    terminate |= !visitor(
                        message.stack_traces.stack_traces_val[i]
                        );
                }
            }
            break;
            
        case EnqueueXfer:
            {
                const CUDA_EnqueueXfer& msg = 
                    raw.CBTF_cuda_message_u.enqueue_xfer;

                for (uint32_t i = msg.call_site;
                     !terminate && 
                         (i < message.stack_traces.stack_traces_len) &&
                         (message.stack_traces.stack_traces_val[i] != 0);
                     ++i)
                {
                    terminate |= !visitor(
                        message.stack_traces.stack_traces_val[i]
                        );
                }
            }
            break;

        case OverflowSamples:
            {
                const CUDA_OverflowSamples& msg =
                    raw.CBTF_cuda_message_u.overflow_samples;
                
                for (uint32_t i = 0; !terminate && (i < msg.pcs.pcs_len); ++i)
                {
                    terminate |= !visitor(msg.pcs.pcs_val[i]);
                }
            }
            break;
            
        default:
            break;
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PerformanceData::PerformanceData() :
    dm_data_table(new Impl::DataTable())
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PerformanceData::apply(const ThreadName& thread,
                            const CBTF_cuda_data& message)
{
    dm_data_table->process(thread, message);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const std::vector<std::string>& PerformanceData::counters() const
{
    return dm_data_table->counters();
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::vector<boost::uint64_t> PerformanceData::counts(
    const ThreadName& thread,
    const TimeInterval& interval
    ) const
{
    std::size_t N = dm_data_table->counters().size();
    std::vector<boost::uint64_t> counts(N, 0);
    
    std::map<ThreadName, DataTable::PerThreadData>::const_iterator i =
        dm_data_table->threads().find(thread);
    
    if (i == dm_data_table->threads().end())
    {
        return counts;
    }

    std::size_t M = i->second.dm_counters.size();
    BOOST_ASSERT(M <= N);
    
    PeriodicSamples::const_iterator j_min, j_max;

    if (!find(i->second.dm_periodic_samples, interval, j_min, j_max))
    {
        return counts;
    }
    
    TimeInterval sample_interval(j_min->first, j_max->first);
    
    boost::uint64_t interval_width = (interval & sample_interval).width();
    boost::uint64_t sample_width = sample_interval.width();

    BOOST_ASSERT((j_min->second.size() == M) && (j_max->second.size() == M));
    for (std::size_t m = 0; m < M; ++m)
    {
        std::size_t n = i->second.dm_counters[m];
        BOOST_ASSERT(n < N);
        
        boost::uint64_t count_delta = j_max->second[m] - j_min->second[m];
        counts[n] = count_delta * interval_width / sample_width;
    }
    
    return counts;
}



//------------------------------------------------------------------------------
// The fully qualfified name below when referring to Device should NOT be
// required since we are using the ArgoNavis::CUDA namespace above. But GCC
// 4.8.2 was complaining that the template paramter was invalid if the fully
// qualified name wasn't used. So for now...
//------------------------------------------------------------------------------
const std::vector<ArgoNavis::CUDA::Device>& PerformanceData::devices() const
{
    return dm_data_table->devices();
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const TimeInterval& PerformanceData::interval() const
{
    return dm_data_table->interval();
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const std::vector<StackTrace>& PerformanceData::sites() const
{
    return dm_data_table->sites();
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PerformanceData::visitBlobs(const ThreadName& thread,
                                 BlobVisitor& visitor) const
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PerformanceData::visitDataTransfers(
    const ThreadName& thread,
    const TimeInterval& interval,
    DataTransferVisitor& visitor
    ) const
{
    std::map<ThreadName, DataTable::PerThreadData>::const_iterator i =
        dm_data_table->threads().find(thread);
    
    if (i != dm_data_table->threads().end())
    {
        i->second.dm_data_transfers.visit<DataTransferVisitor>(
            interval, visitor
            );
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PerformanceData::visitKernelExecutions(
    const ThreadName& thread,
    const TimeInterval& interval,
    KernelExecutionVisitor& visitor
    ) const
{
    std::map<ThreadName, DataTable::PerThreadData>::const_iterator i =
        dm_data_table->threads().find(thread);
    
    if (i != dm_data_table->threads().end())
    {
        i->second.dm_kernel_executions.visit<KernelExecutionVisitor>(
            interval, visitor
            );
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PerformanceData::visitPeriodicSamples(
    const ThreadName& thread,
    const TimeInterval& interval,
    const PeriodicSampleVisitor& visitor
    )
{
    std::map<ThreadName, DataTable::PerThreadData>::const_iterator i =
        dm_data_table->threads().find(thread);
    
    if (i == dm_data_table->threads().end())
    {
        return;
    }

    std::size_t N = dm_data_table->counters().size();
    std::vector<boost::uint64_t> counts(N, 0);
    
    std::size_t M = i->second.dm_counters.size();
    BOOST_ASSERT(M <= N);
    
    bool terminate = false;

    PeriodicSamples::const_iterator j_min, j_max;

    if (!find(i->second.dm_periodic_samples, interval, j_min, j_max))
    {
        return;
    }

    ++j_max;
    
    for (PeriodicSamples::const_iterator
             j = j_min; !terminate && (j != j_max); ++j)
    {
        Time t(j->first);
        
        if (interval.contains(t))
        {
            counts.assign(N, 0);
            BOOST_ASSERT(j->second.size() == M);
            for (std::size_t m = 0; m < M; ++m)
            {
                std::size_t n = i->second.dm_counters[m];
                BOOST_ASSERT(n < N);

                counts[n] = j->second[m];
            }
            
            terminate |= !visitor(t, counts);
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PerformanceData::visitThreads(const ThreadVisitor& visitor) const
{
    bool terminate = false;

    for (std::map<ThreadName, DataTable::PerThreadData>::const_iterator
             i = dm_data_table->threads().begin(),
             i_end = dm_data_table->threads().end();
         !terminate && (i != i_end);
         ++i)
    {
        terminate |= !visitor(i->first);
    }
}
