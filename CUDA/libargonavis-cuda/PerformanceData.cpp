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

#include <ArgoNavis/CUDA/PerformanceData.hpp>

#include "DataTable.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::CUDA;



/** Anonymous namespace hiding implementation details. */
namespace {

    // ...
   
} // namespace <anonymous>



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PerformanceData::visitPCs(const CBTF_cuda_data& message,
                               AddressVisitor& visitor)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
PerformanceData::PerformanceData() :
    dm_data_table(new Impl::DataTable())
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PerformanceData::applyMessage(const CBTF_DataHeader& header,
                                   const CBTF_cuda_data& message)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PerformanceData::applyMessage(const ThreadName& thread,
                                   const CBTF_cuda_data& message)
{
    // ...
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
    // ...

    return std::vector<boost::uint64_t>();
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
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PerformanceData::visitKernelExecutions(
    const ThreadName& thread,
    const TimeInterval& interval,
    KernelExecutionVisitor& visitor
    ) const
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PerformanceData::visitPeriodicSamples(
    const ThreadName& thread,
    const TimeInterval& interval,
    const PeriodicSampleVisitor& visitor
    )
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void PerformanceData::visitThreads(const ThreadVisitor& visitor) const
{
    // ...
}
