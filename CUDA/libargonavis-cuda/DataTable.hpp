////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2016 Argo Navis Technologies. All Rights Reserved.
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
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <map>
#include <set>
#include <stddef.h>
#include <string>
#include <vector>
#include <mutex>

#include <KrellInstitute/Messages/CUDA_data.h>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/AddressVisitor.hpp>
#include <ArgoNavis/Base/BlobVisitor.hpp>
#include <ArgoNavis/Base/StackTrace.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

#include <ArgoNavis/CUDA/DataTransfer.hpp>
#include <ArgoNavis/CUDA/Device.hpp>
#include <ArgoNavis/CUDA/KernelExecution.hpp>

#include "BlobGenerator.hpp"
#include "EventTable.hpp"
#include "PartialEventTable.hpp"

namespace ArgoNavis { namespace CUDA { namespace Impl {

    /**
     * Table containing the performance data for one or more threads. This class
     * provides underlying implementation details for the PerformanceData class.
     */
    class DataTable
    {

    public:

        /** Type of handle (smart pointer) to a data table. */
        typedef boost::shared_ptr<DataTable> Handle;

        /** Structure containing per-thread data. */
        struct PerThreadData
        {
            /**
             * Index within DataTable::counters() for each of this thread's
             * sampled hardware performance counters.
             */
            std::vector<std::vector<std::string>::size_type> dm_counters;
            
            /** Table of this thread's data transfers. */
            EventTable<DataTransfer> dm_data_transfers;
            
            /** Table of this thread's kernel executions. */
            EventTable<KernelExecution> dm_kernel_executions;
            
            /** Processed periodic samples. */
            std::map<
                boost::uint64_t, std::vector<boost::uint64_t>
                > dm_periodic_samples;
            
            /** Unprocessed periodic samples. */
            std::vector<
                std::vector<boost::uint8_t>
                > dm_unprocessed_periodic_samples;
        };

        /** Visit the PC addresses within the given message. */
        static void visitPCs(const CBTF_cuda_data& message,
                             const Base::AddressVisitor& visitor);

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

        /**
         * Index, within devices(), of the device for which the given thread is
         * a GPU hardware performance counter sampling thread. Returns "none" if
         * the thread isn't a GPU hardware performance counter sampling thread.
         */
        boost::optional<std::size_t> device(
            const Base::ThreadName& thread
            ) const;

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

        /** Visit the (raw) performance data blobs for the given thread. */
        void visitBlobs(const Base::ThreadName& thread,
                        const Base::BlobVisitor& visitor) const;
        
    private:

        /** Structure containing per-host data. */
        struct PerHostData
        {
            /** Device ID for all known devices. */
            std::set<boost::uint32_t> dm_known_devices;
        };
        
        /** Structure containing per-process data. */
        struct PerProcessData
        {
            /** Table of this process' partial data transfers. */
            PartialEventTable<DataTransfer> dm_partial_data_transfers;

            /** Table of this process' partial kernel executions. */
            PartialEventTable<KernelExecution> dm_partial_kernel_executions;
        };
        
        /** Access the per-host data for the specified thread. */
        PerHostData& accessPerHostData(const Base::ThreadName& thread);

        /** Access the per-process data for the specified thread. */
        PerProcessData& accessPerProcessData(const Base::ThreadName& thread);
        
        /** Access the per-thread data for the specified thread. */
        PerThreadData& accessPerThreadData(const Base::ThreadName& thread);

        /** Find the given call site in (or add it to) the known call sites. */
        size_t findSite(boost::uint32_t site, const CBTF_cuda_data& data);

        /** 
         * Generate the context/device information and sampling config messages.
         */
        bool generate(const PerProcessData& per_process,
                      const PerThreadData& per_thread,
                      BlobGenerator& generator) const;
        
        /** Generate the messages for a DataTransfer event. */
        bool generate(const DataTransfer& event,
                      BlobGenerator& generator) const;
        
        /** Generate the messages for a KernelExecution event. */
        bool generate(const KernelExecution& event,
                      BlobGenerator& generator) const;
        
        /** Process a CUDA_CompletedExec message. */
        void process(const CUDA_CompletedExec& message,
                     PerProcessData& per_process);

        /** Process a CUDA_CompletedXfer message. */
        void process(const CUDA_CompletedXfer& message,
                     PerProcessData& per_process);

        /** Process a CUDA_ContextInfo message. */
        void process(const CUDA_ContextInfo& message,
                     PerProcessData& per_process);
        
        /** Process a CUDA_DeviceInfo message. */
        void process(const CUDA_DeviceInfo& message,
                     PerHostData& per_host,
                     PerProcessData& per_process);
        
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
        void process(const CUDA_OverflowSamples& message,
                     PerThreadData& per_thread);
        
        /** Process a CUDA_PeriodicSamples message. */
        void process(const CUDA_PeriodicSamples& message,
                     PerThreadData& per_thread);
        
        /** Process a CUDA_SamplingConfig message. */
        void process(const CUDA_SamplingConfig& message,
                     PerThreadData& per_thread);

        /** Process a DataTransfer event completions. */
        void process(
            const PartialEventTable<DataTransfer>::Completions& completions
            );

        /** Process a KernelExecution event completions. */
        void process(
            const PartialEventTable<KernelExecution>::Completions& completions
            );

        /** Process periodic samples. */
        void processPeriodicSamples(const boost::uint8_t* begin,
                                    const boost::uint8_t* end,
                                    PerThreadData& per_thread);

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
        mutable boost::mutex dm_hosts_mutex;

        /** Per-process data for all known processes. */
        std::map<Base::ThreadName, PerProcessData> dm_processes;
        mutable boost::mutex dm_processes_mutex;

        /** Per-thread data for all known threads. */
        std::map<Base::ThreadName, PerThreadData> dm_threads;
        mutable boost::mutex dm_threads_mutex;
        
    }; // class DataTable

} } } // namespace ArgoNavis::CUDA::Impl
