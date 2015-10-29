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

/** @file Definition of the DataTable class. */

#include <ArgoNavis/CUDA/CachePreference.hpp>
#include <ArgoNavis/CUDA/CopyKind.hpp>
#include <ArgoNavis/CUDA/MemoryKind.hpp>

#include "DataTable.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::CUDA;
using namespace ArgoNavis::CUDA::Impl;



/** Anonymous namespace hiding implementation details. */
namespace {

    /** Convert a CUDA_CachePreference into a CachePreference. */
    CachePreference convert(const CUDA_CachePreference& value)
    {
        switch (value)
        {
        case InvalidCachePreference: return kInvalidCachePreference;
        case NoPreference: return kNoPreference;
        case PreferShared: return kPreferShared;
        case PreferCache: return kPreferCache;
        case PreferEqual: return kPreferEqual;
        default: return kInvalidCachePreference;
        }
    }

    /** Convert a CUDA_CopyKind into a CopyKind. */
    CopyKind convert(const CUDA_CopyKind& value)
    {
        switch (value)
        {
        case InvalidCopyKind: return kInvalidCopyKind;
        case UnknownCopyKind: return kUnknownCopyKind;
        case HostToDevice: return kHostToDevice;
        case DeviceToHost: return kDeviceToHost;
        case HostToArray: return kHostToArray;
        case ArrayToHost: return kArrayToHost;
        case ArrayToArray: return kArrayToArray;
        case ArrayToDevice: return kArrayToDevice;
        case DeviceToArray: return kDeviceToArray;
        case DeviceToDevice: return kDeviceToDevice;
        case HostToHost: return kHostToHost;
        default: return kInvalidCopyKind;
        }
    }

    /** Convert a CUDA_MemoryKind into a MemoryKind. */
    MemoryKind convert(const CUDA_MemoryKind& value)
    {
        switch (value)
        {
        case InvalidMemoryKind: return kInvalidMemoryKind;
        case UnknownMemoryKind: return kUnknownMemoryKind;
        case Pageable: return kPageable;
        case Pinned: return kPinned;
        case ::Device: return kDevice;
        case Array: return kArray;
        default: return kInvalidMemoryKind;
        }
    }
   
} // namespace <anonymous>



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DataTable::DataTable() :
    dm_counters(),
    dm_devices(),
    dm_interval(),
    dm_sites(),
    dm_hosts(),
    dm_processes(),
    dm_threads()
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const CBTF_cuda_data& message)
{
    PerHostData& per_host = accessPerHostData(thread);
    PerProcessData& per_process = accessPerProcessData(thread);
    PerThreadData& per_thread = accessPerThreadData(thread);
    
    for (u_int i = 0; i < message.messages.messages_len; ++i)
    {
        const CBTF_cuda_message& raw = message.messages.messages_val[i];
        
        switch (raw.type)
        {
            
        case CompletedExec:
            process(raw.CBTF_cuda_message_u.completed_exec, per_process);
            break;
            
        case CompletedXfer:
            process(raw.CBTF_cuda_message_u.completed_xfer, per_process);
            break;

        case ContextInfo:
            process(raw.CBTF_cuda_message_u.context_info);
            break;
            
        case DeviceInfo:
            process(raw.CBTF_cuda_message_u.device_info);
            break;

        case EnqueueExec:
            process(raw.CBTF_cuda_message_u.enqueue_exec, message, thread,
                    per_process);
            break;

        case EnqueueXfer:
            process(raw.CBTF_cuda_message_u.enqueue_xfer, message, thread,
                    per_process);
            break;

        case OverflowSamples:
            process(raw.CBTF_cuda_message_u.overflow_samples);
            break;

        case PeriodicSamples:
            process(raw.CBTF_cuda_message_u.periodic_samples);
            break;
            
        case SamplingConfig:
            process(raw.CBTF_cuda_message_u.sampling_config);
            break;
            
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DataTable::PerHostData& DataTable::accessPerHostData(const ThreadName& thread)
{
    ThreadName key(thread.host(), 0 /* Dummy PID */);
    
    std::map<ThreadName, PerHostData>::iterator i = dm_hosts.find(key);
    
    if (i == dm_hosts.end())
    {
        i = dm_hosts.insert(std::make_pair(key, PerHostData())).first;
    }
    
    return i->second;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DataTable::PerProcessData& DataTable::accessPerProcessData(
    const ThreadName& thread
    )
{
    ThreadName key(thread.host(), thread.pid());

    std::map<ThreadName, PerProcessData>::iterator i = dm_processes.find(key);
    
    if (i == dm_processes.end())
    {
        i = dm_processes.insert(std::make_pair(key, PerProcessData())).first;
    }
    
    return i->second;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DataTable::PerThreadData& DataTable::accessPerThreadData(
    const ThreadName& thread
    )
{
    ThreadName key(thread);

    std::map<ThreadName, PerThreadData>::iterator i = dm_threads.find(key);
    
    if (i == dm_threads.end())
    {
        i = dm_threads.insert(std::make_pair(key, PerThreadData())).first;
    }
    
    return i->second;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_CompletedExec& message,
                        PerProcessData& per_process)
{
    KernelExecution event;

    // event.device = ???
    event.time_begin = message.time_begin;
    event.time_end = message.time_end;
    event.function = message.function;
    event.grid = Vector3u(message.grid[0], message.grid[1], message.grid[2]);
    event.block = Vector3u(
        message.block[0], message.block[1], message.block[2]
        );
    event.cache_preference = convert(message.cache_preference);
    event.registers_per_thread = message.registers_per_thread;
    event.static_shared_memory = message.static_shared_memory;
    event.dynamic_shared_memory = message.dynamic_shared_memory;
    event.local_memory = message.local_memory;

    PartialEventTable<KernelExecution>::Completion completion = 
        per_process.dm_partial_kernel_executions.completed(message.id, event);
    
    if (completion)
    {
        PerThreadData& per_thread = accessPerThreadData(completion->first);
        per_thread.dm_kernel_executions.add(completion->second);
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_CompletedXfer& message,
                        PerProcessData& per_process)
{
    DataTransfer event;

    // event.device = ???
    event.time_begin = message.time_begin;
    event.time_end = message.time_end;
    event.size = message.size;
    event.kind = convert(message.kind);
    event.source_kind = convert(message.source_kind);
    event.destination_kind = convert(message.destination_kind);
    event.asynchronous = message.asynchronous;

    PartialEventTable<DataTransfer>::Completion completion = 
        per_process.dm_partial_data_transfers.completed(message.id, event);
    
    if (completion)
    {
        PerThreadData& per_thread = accessPerThreadData(completion->first);
        per_thread.dm_data_transfers.add(completion->second);
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_ContextInfo& message)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_DeviceInfo& message)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_EnqueueExec& message,
                        const struct CBTF_cuda_data& data,
                        const Base::ThreadName& thread,
                        PerProcessData& per_process)
{
    KernelExecution event;
    
    event.time = message.time;
    // event.call_site = ???
    
    PartialEventTable<KernelExecution>::Completion completion = 
        per_process.dm_partial_kernel_executions.enqueue(
            thread, message.id, event
            );
    
    if (completion)
    {
        PerThreadData& per_thread = accessPerThreadData(completion->first);
        per_thread.dm_kernel_executions.add(completion->second);
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_EnqueueXfer& message,
                        const struct CBTF_cuda_data& data,
                        const Base::ThreadName& thread,
                        PerProcessData& per_process)
{
    DataTransfer event;

    event.time = message.time;
    // event.call_site = ???
    
    PartialEventTable<DataTransfer>::Completion completion = 
        per_process.dm_partial_data_transfers.enqueue(
            thread, message.id, event
            );
    
    if (completion)
    {
        PerThreadData& per_thread = accessPerThreadData(completion->first);
        per_thread.dm_data_transfers.add(completion->second);
    }
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_OverflowSamples& message)
{
    // ...
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_PeriodicSamples& message)
{
    // ...
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const CUDA_SamplingConfig& message)
{
    // ...
}
