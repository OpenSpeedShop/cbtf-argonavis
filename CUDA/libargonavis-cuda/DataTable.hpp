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

/** @file Declaration of the DataTable class. */

#pragma once

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>
#include <vector>

#include <KrellInstitute/Messages/CUDA_data.h>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/StackTrace.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

#include <ArgoNavis/CUDA/DataTransfer.hpp>
#include <ArgoNavis/CUDA/Device.hpp>
#include <ArgoNavis/CUDA/KernelExecution.hpp>

#include "EventTable.hpp"
#include "PartialEventTable.hpp"

namespace ArgoNavis { namespace CUDA { namespace Impl {

    /**
     * Table containing the performance data for one or more threads. This class
     * provides the underlying implementation for the PerformanceData class.
     */
    class DataTable
    {

    public:

        /** Type of handle (smart pointer) to a data table. */
        typedef boost::shared_ptr<DataTable> Handle;

        /** Structure containing per-thread data. */
        struct PerThreadData
        {
            /** Table of this thread's data transfers. */
            EventTable<DataTransfer> dm_data_transfers;
            
            /** Table of this thread's kernel executions. */
            EventTable<KernelExecution> dm_kernel_executions;
            
            // ...
        };

        /** Construct an empty data table. */
        DataTable();

        /** Process the performance data contained within the given message. */
        void process(const Base::ThreadName& thread,
                     const CBTF_cuda_data& message);
        
        /** Names of all sampled hardware performance counters. */
        const std::vector<std::string>& counters() const
        {
            return dm_counters;
        }

        /** Information about all known CUDA devices. */
        const std::vector<Device>& devices() const
        {
            return dm_devices;
        }

        /** Smallest time interval containing this performance data. */
        const Base::TimeInterval& interval() const
        {
            return dm_interval;
        }

        /** Call sites of all known CUDA requests. */
        const std::vector<Base::StackTrace>& sites() const
        {
            return dm_sites;
        }

        /** Access the per-thread data for all known threads. */
        const std::map<Base::ThreadName, PerThreadData>& threads() const
        {
            return dm_threads;
        }
        
    private:

        /** Structure containing per-host data. */
        struct PerHostData
        {
            /** Index in dm_devices for each known device ID. */
            std::map<
                boost::uint32_t, std::vector<Device>::size_type
                > dm_known_devices;
        };
        
        /** Structure containing per-process data. */
        struct PerProcessData
        {
            /** Device ID for each known context address. */
            std::map<Base::Address, boost::uint32_t> dm_known_contexts;

            /** Table of this process' partial data transfers. */
            PartialEventTable<DataTransfer> dm_partial_data_transfers;

            /** Table of this process' partial kernel executions. */
            PartialEventTable<KernelExecution> dm_partial_kernel_executions;

            // ...
        };
        
        /** Access the per-host data for the specified thread. */
        PerHostData& accessPerHostData(const Base::ThreadName& thread);

        /** Access the per-process data for the specified thread. */
        PerProcessData& accessPerProcessData(const Base::ThreadName& thread);
        
        /** Access the per-thread data for the specified thread. */
        PerThreadData& accessPerThreadData(const Base::ThreadName& thread);
        
        /** Process a CUDA_CompletedExec message. */
        void process(const CUDA_CompletedExec& message,
                     PerProcessData& per_process);

        /** Process a CUDA_CompletedXfer message. */
        void process(const CUDA_CompletedXfer& message,
                     PerProcessData& per_process);

        /** Process a CUDA_ContextInfo message. */
        void process(const CUDA_ContextInfo& message);
        
        /** Process a CUDA_DeviceInfo message. */
        void process(const CUDA_DeviceInfo& message);
        
        /** Process a CUDA_EnqueueExec message. */
        void process(const CUDA_EnqueueExec& message,
                     const CBTF_cuda_data& data,
                     const Base::ThreadName& thread,
                     PerProcessData& per_process);
        
        /** Process a CUDA_EnqueueXfer message. */
        void process(const CUDA_EnqueueXfer& message,
                     const CBTF_cuda_data& data,
                     const Base::ThreadName& thread,
                     PerProcessData& per_process);
        
        /** Process a CUDA_OverflowSamples message. */
        void process(const CUDA_OverflowSamples& message);
        
        /** Process a CUDA_PeriodicSamples message. */
        void process(const CUDA_PeriodicSamples& message);
        
        /** Process a CUDA_SamplingConfig message. */
        void process(const CUDA_SamplingConfig& message);

        /** Names of all sampled hardware performance counters. */
        std::vector<std::string> dm_counters;

        /** Information about all known CUDA devices. */
        std::vector<Device> dm_devices;

        /** Smallest time interval containing this performance data. */
        Base::TimeInterval dm_interval;
        
        /** Call sites of all known CUDA requests. */
        std::vector<Base::StackTrace> dm_sites;

        /** Per-host data for all known hosts. */
        std::map<Base::ThreadName, PerHostData> dm_hosts;

        /** Per-process data for all known processes. */
        std::map<Base::ThreadName, PerProcessData> dm_processes;

        /** Per-thread data for all known threads. */
        std::map<Base::ThreadName, PerThreadData> dm_threads;
        
    }; // class DataTable

} } } // namespace ArgoNavis::CUDA::Impl
