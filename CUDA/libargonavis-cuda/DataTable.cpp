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

/** @file Definition of the DataTable class. */

#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <cstring>
#include <utility>

#include <ArgoNavis/Base/Raise.hpp>

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

    /** Convert a CUDA_CompletedExec into a (partial) KernelExecution. */
    KernelExecution convert(const CUDA_CompletedExec& message)
    {    
        KernelExecution event;

        event.id = message.id;
        event.time_begin = message.time_begin;
        event.time_end = message.time_end;
        event.function = message.function;
        event.grid = Vector3u(message.grid[0],
                              message.grid[1],
                              message.grid[2]);
        event.block = Vector3u(message.block[0],
                               message.block[1],
                               message.block[2]);
        event.cache_preference = convert(message.cache_preference);
        event.registers_per_thread = message.registers_per_thread;
        event.static_shared_memory = message.static_shared_memory;
        event.dynamic_shared_memory = message.dynamic_shared_memory;
        event.local_memory = message.local_memory;

        return event;
    }

    /** Convert a CUDA_CompletedXfer into a (partial) DataTransfer. */
    DataTransfer convert(const CUDA_CompletedXfer& message)
    {
        DataTransfer event;

        event.id = message.id;
        event.time_begin = message.time_begin;
        event.time_end = message.time_end;
        event.size = message.size;
        event.kind = convert(message.kind);
        event.source_kind = convert(message.source_kind);
        event.destination_kind = convert(message.destination_kind);
        event.asynchronous = message.asynchronous;
        
        return event;
    }
    
    /** Convert a CUDA_DeviceInfo into a Device. */
    ArgoNavis::CUDA::Device convert(const CUDA_DeviceInfo& message)
    {
        ArgoNavis::CUDA::Device device;

        device.name = message.name;
        device.compute_capability = Vector2u(message.compute_capability[0],
                                             message.compute_capability[1]);
        device.max_grid = Vector3u(message.max_grid[0],
                                   message.max_grid[1],
                                   message.max_grid[2]);
        device.max_block = Vector3u(message.max_block[0],
                                    message.max_block[1],
                                    message.max_block[2]);
        device.global_memory_bandwidth = message.global_memory_bandwidth;
        device.global_memory_size = message.global_memory_size;
        device.constant_memory_size = message.constant_memory_size;
        device.l2_cache_size = message.l2_cache_size;
        device.threads_per_warp = message.threads_per_warp;
        device.core_clock_rate = message.core_clock_rate;
        device.memcpy_engines = message.memcpy_engines;
        device.multiprocessors = message.multiprocessors;
        device.max_ipc = message.max_ipc;
        device.max_warps_per_multiprocessor = 
            message.max_warps_per_multiprocessor;
        device.max_blocks_per_multiprocessor = 
            message.max_blocks_per_multiprocessor;
        device.max_registers_per_block = message.max_registers_per_block;
        device.max_shared_memory_per_block = 
            message.max_shared_memory_per_block;
        device.max_threads_per_block = message.max_threads_per_block;

        return device;        
    }

    /** Convert a CUDA_EnqueueExec into a (partial) KernelExecution. */
    KernelExecution convert(const CUDA_EnqueueExec& message)
    {    
        KernelExecution event;
        event.id = message.id;
        event.context = message.context;
        event.stream = message.stream;
        event.time = message.time;
        return event;
    }

    /** Convert a CUDA_EnqueueXfer into a (partial) DataTransfer. */
    DataTransfer convert(const CUDA_EnqueueXfer& message)
    {
        DataTransfer event;
        event.id = message.id;
        event.context = message.context;
        event.stream = message.stream;
        event.time = message.time;
        return event;
    }

    /** Convert a CachePreference into a CUDA_CachePreference. */
    CUDA_CachePreference convert(const CachePreference& value)
    {
        switch (value)
        {
        case kInvalidCachePreference: return InvalidCachePreference;
        case kNoPreference: return NoPreference;
        case kPreferShared: return PreferShared;
        case kPreferCache: return PreferCache;
        case kPreferEqual: return PreferEqual;
        default: return InvalidCachePreference;
        }
    }

    /** Convert a CopyKind into a CUDA_CopyKind. */
    CUDA_CopyKind convert(const CopyKind& value)
    {
        switch (value)
        {
        case kInvalidCopyKind: return InvalidCopyKind;
        case kUnknownCopyKind: return UnknownCopyKind;
        case kHostToDevice: return HostToDevice;
        case kDeviceToHost: return DeviceToHost;
        case kHostToArray: return HostToArray;
        case kArrayToHost: return ArrayToHost;
        case kArrayToArray: return ArrayToArray;
        case kArrayToDevice: return ArrayToDevice;
        case kDeviceToArray: return DeviceToArray;
        case kDeviceToDevice: return DeviceToDevice;
        case kHostToHost: return HostToHost;
        default: return InvalidCopyKind;
        }
    }

    /** Convert a MemoryKind into a CUDA_MemoryKind. */
    CUDA_MemoryKind convert(const MemoryKind& value)
    {
        switch (value)
        {
        case kInvalidMemoryKind: return InvalidMemoryKind;
        case kUnknownMemoryKind: return UnknownMemoryKind;
        case kPageable: return Pageable;
        case kPinned: return Pinned;
        case kDevice: return ::Device;
        case kArray: return Array;
        default: return InvalidMemoryKind;
        }
    }

    /** Convert a Device into a CUDA_DeviceInfo. */
    CUDA_DeviceInfo convert(const ArgoNavis::CUDA::Device& device)
    {
        CUDA_DeviceInfo message;
        
        message.device = 0; // Caller must provide this
        message.name = strdup(device.name.c_str());
        message.compute_capability[0] = device.compute_capability.get<0>();
        message.compute_capability[1] = device.compute_capability.get<1>();
        message.max_grid[0] = device.max_grid.get<0>();
        message.max_grid[1] = device.max_grid.get<1>();
        message.max_grid[2] = device.max_grid.get<2>();
        message.max_block[0] = device.max_block.get<0>();
        message.max_block[1] = device.max_block.get<1>();
        message.max_block[2] = device.max_block.get<2>();
        message.global_memory_bandwidth = device.global_memory_bandwidth;
        message.global_memory_size = device.global_memory_size;
        message.constant_memory_size = device.constant_memory_size;
        message.l2_cache_size = device.l2_cache_size;
        message.threads_per_warp = device.threads_per_warp;
        message.core_clock_rate = device.core_clock_rate;
        message.memcpy_engines = device.memcpy_engines;
        message.multiprocessors = device.multiprocessors;
        message.max_ipc = device.max_ipc;
        message.max_warps_per_multiprocessor =
            device.max_warps_per_multiprocessor;
        message.max_blocks_per_multiprocessor = 
            device.max_blocks_per_multiprocessor;
        message.max_registers_per_block = device.max_registers_per_block;
        message.max_shared_memory_per_block = 
            device.max_shared_memory_per_block;
        message.max_threads_per_block = device.max_threads_per_block;
        
        return message;
    }

    /**
     * Convert a DataTransfer into a CUDA_EnqueueXfer and CUDA_CompletedXfer.
     */
    std::pair<CUDA_EnqueueXfer, CUDA_CompletedXfer> convert(
        const DataTransfer& event
        )
    {
        CUDA_EnqueueXfer enqueue;
        CUDA_CompletedXfer completed;

        enqueue.id = event.id;
        enqueue.context = event.context;
        enqueue.stream = event.stream;
        enqueue.time = event.time;
        enqueue.call_site = 0; // Caller must provide this
        
        completed.id = event.id;
        completed.time_begin = event.time_begin;
        completed.time_end = event.time_end;
        completed.size = event.size;
        completed.kind = convert(event.kind);
        completed.source_kind = convert(event.source_kind);
        completed.destination_kind = convert(event.destination_kind);
        completed.asynchronous = event.asynchronous ? TRUE : FALSE;

        return std::make_pair(enqueue, completed);
    }

    /**
     * Convert a KernelExecution into a CUDA_EnqueueExec and CUDA_CompletedExec.
     */
    std::pair<CUDA_EnqueueExec, CUDA_CompletedExec> convert(
        const KernelExecution& event
        )
    {
        CUDA_EnqueueExec enqueue;
        CUDA_CompletedExec completed;

        enqueue.id = event.id;
        enqueue.context = event.context;
        enqueue.stream = event.stream;
        enqueue.time = event.time;
        enqueue.call_site = 0; // Caller must provide this

        completed.id = event.id;
        completed.time_begin = event.time_begin;
        completed.time_end = event.time_end;
        completed.function = strdup(event.function.c_str());
        completed.grid[0] = event.grid.get<0>();
        completed.grid[1] = event.grid.get<1>();
        completed.grid[2] = event.grid.get<2>();
        completed.block[0] = event.block.get<0>();
        completed.block[1] = event.block.get<1>();
        completed.block[2] = event.block.get<2>();
        completed.cache_preference = convert(event.cache_preference);
        completed.registers_per_thread = event.registers_per_thread;
        completed.static_shared_memory = event.static_shared_memory;
        completed.dynamic_shared_memory = event.dynamic_shared_memory;
        completed.local_memory = event.local_memory;

        return std::make_pair(enqueue, completed);
    }
    
} // namespace <anonymous>



//------------------------------------------------------------------------------
// Iterate over each of the individual CUDA messages that are "packed" into the
// specified performance data. For all of the messages containing stack traces
// or sampled PC addresses, invoke the given visitor.
//------------------------------------------------------------------------------
void DataTable::visitPCs(const CBTF_cuda_data& message,
                         const AddressVisitor& visitor)
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
            process(raw.CBTF_cuda_message_u.context_info, per_process);
            break;
            
        case DeviceInfo:
            process(raw.CBTF_cuda_message_u.device_info, per_host, per_process);
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
            process(raw.CBTF_cuda_message_u.overflow_samples, per_thread);
            break;

        case PeriodicSamples:
            process(raw.CBTF_cuda_message_u.periodic_samples, per_thread);
            break;
            
        case SamplingConfig:
            process(raw.CBTF_cuda_message_u.sampling_config, per_thread);
            break;
            
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::visitBlobs(const Base::ThreadName& thread,
                           const Base::BlobVisitor& visitor) const
{
    std::map<ThreadName, PerProcessData>::const_iterator i_process = 
        dm_processes.find(ThreadName(thread.host(), thread.pid()));
    
    std::map<ThreadName, PerThreadData>::const_iterator i_thread = 
        dm_threads.find(thread);
    
    if ((i_process == dm_processes.end()) || (i_thread == dm_threads.end()))
    {
        return;
    }
    
    const PerProcessData& per_process = i_process->second;
    const PerThreadData& per_thread = i_thread->second;
    
    BlobGenerator generator(thread, visitor);

    // Generate the context/device information and sampling config messages
    
    generate(per_process, per_thread, generator);
    
    if (generator.terminate())
    {
        return; // Terminate the iteration
    }

    // Visit all of the data transfer events, adding them to the generator

    per_thread.dm_data_transfers.visit(
        TimeInterval(Time::TheBeginning(), Time::TheEnd()),
        boost::bind(
            static_cast<
                bool (DataTable::*)(const DataTransfer&, BlobGenerator&) const
                >(&DataTable::generate),
            this, _1, boost::ref(generator)
            )
        );

    if (generator.terminate())
    {
        return; // Terminate the iteration
    }

    // Visit all of the kernel execution events, adding them to the generator

    per_thread.dm_kernel_executions.visit(
        TimeInterval(Time::TheBeginning(), Time::TheEnd()),
        boost::bind(
            static_cast<
                bool (DataTable::*)(const KernelExecution&, 
                                    BlobGenerator&) const
            >(&DataTable::generate),
            this, _1, boost::ref(generator)
            )
        );

    if (generator.terminate())
    {
        return; // Terminate the iteration
    }
    
    // Add the periodic samples to the generator
    
    for (std::map<
             boost::uint64_t, std::vector<boost::uint64_t>
             >::const_iterator i = per_thread.dm_periodic_samples.begin();
         (i != per_thread.dm_periodic_samples.end()) && !generator.terminate();
         ++i)
    {
        generator.addPeriodicSample(i->first, i->second);
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
// Construct a stack trace containing all frames for the given call site, then
// search the known call sites for that stack trace. The existing call site is
// reused if it is found. Otherwise the stack trace is added to the known call
// sites.
//
// Note that a linear search is currently employed here because the number of
// unique CUDA call sites is expected to be low. If that ends up not being the
// case, and performance is inadequate, a hash of each stack trace could be
// computed and used to accelerate this search.
//------------------------------------------------------------------------------
size_t DataTable::findSite(boost::uint32_t site, const CBTF_cuda_data& data)
{
    StackTrace trace;

    for (boost::uint32_t i = site;
         (i < data.stack_traces.stack_traces_len) &&
             (data.stack_traces.stack_traces_val[i] != 0);
         ++i)
    {
        trace.push_back(data.stack_traces.stack_traces_val[i]);
    }
    
    std::vector<StackTrace>::size_type i;

    for (i = 0; i < dm_sites.size(); ++i)
    {
        if (dm_sites[i] == trace)
        {
            break;
        }
    }
    
    if (i == dm_sites.size())
    {
        dm_sites.push_back(trace);
    }

    return i;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool DataTable::generate(const PerProcessData& per_process,
                         const PerThreadData& per_thread,
                         BlobGenerator& generator) const
{
    std::map<Address, boost::uint32_t> contexts;
    std::map<boost::uint32_t, std::size_t> devices;

    // Visit all of the known context addresses for data transfer events,
    // lookup their device ID and index within DataTable::dm_devices, and
    // add this information to the maps above.
    
    for (std::set<Address>::const_iterator
             i = per_thread.dm_data_transfers.contexts().begin();
         i != per_thread.dm_data_transfers.contexts().end();
         ++i)
    {
        if (contexts.find(*i) != contexts.end())
        {
            continue;
        }

        boost::uint32_t device = 
            per_process.dm_partial_data_transfers.device(*i);

        contexts.insert(std::make_pair(*i, device));

        if (devices.find(device) != devices.end())
        {
            continue;
        }
        
        std::size_t index = 
            per_process.dm_partial_data_transfers.index(device);
        
        devices.insert(std::make_pair(device, index));
    }

    // Visit all of the known context addresses for kernel execution events,
    // lookup their device ID and index within DataTable::dm_devices, and add
    // this information to the maps above.
    
    for (std::set<Address>::const_iterator
             i = per_thread.dm_kernel_executions.contexts().begin();
         i != per_thread.dm_kernel_executions.contexts().end();
         ++i)
    {
        if (contexts.find(*i) != contexts.end())
        {
            continue;
        }

        boost::uint32_t device = 
            per_process.dm_partial_kernel_executions.device(*i);
        
        contexts.insert(std::make_pair(*i, device));

        if (devices.find(device) != devices.end())
        {
            continue;
        }
        
        std::size_t index = 
            per_process.dm_partial_kernel_executions.index(device);
        
        devices.insert(std::make_pair(device, index));
    }

    // Add the necessary context information messages to the blob generator

    for (std::map<Address, boost::uint32_t>::const_iterator
             i = contexts.begin(); i != contexts.end(); ++i)
    {
        CBTF_cuda_message* message = generator.addMessage();
        
        if (generator.terminate())
        {
            return false; // Terminate the iteration
        }

        message->type = ContextInfo;
        CUDA_ContextInfo& info = message->CBTF_cuda_message_u.context_info;

        info.context = i->first;
        info.device = i->second;
    }

    // Add the necessary device information messages to the blob generator
    
    for (std::map<boost::uint32_t, std::size_t>::const_iterator
             i = devices.begin(); i != devices.end(); ++i)
    {
        CBTF_cuda_message* message = generator.addMessage();
        
        if (generator.terminate())
        {
            return false; // Terminate the iteration
        }

        message->type = DeviceInfo;
        CUDA_DeviceInfo& info = message->CBTF_cuda_message_u.device_info;

        info = convert(dm_devices[i->second]);
        info.device = i->first;
    }

    // Add a CUDA_SamplingConfig message to the blob generator

    CBTF_cuda_message* message = generator.addMessage();
        
    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }
        
    message->type = SamplingConfig;    
    CUDA_SamplingConfig& config = message->CBTF_cuda_message_u.sampling_config;

    config.interval = 0; // TODO: Use the real interval!?

    config.events.events_len = per_thread.dm_counters.size();

    config.events.events_val =
        reinterpret_cast<CUDA_EventDescription*>(
            malloc(std::max(1U, config.events.events_len) *
                   sizeof(CUDA_EventDescription))
            );

    for (std::vector<std::vector<std::string>::size_type>::const_iterator
             i = per_thread.dm_counters.begin();
         i  != per_thread.dm_counters.end();
         ++i)
    {
        CUDA_EventDescription& description = config.events.events_val[*i];
        
        description.name = strdup(dm_counters[*i].c_str());
        description.threshold = 0; // TODO: Use the real threshold!?
    }
    
    // Continue the iteration
    return true;    
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool DataTable::generate(const DataTransfer& event,
                         BlobGenerator& generator) const
{
    // Convert the event into the corresponding enqueue and completed messages
    std::pair<CUDA_EnqueueXfer, CUDA_CompletedXfer> messages = convert(event);
    CUDA_EnqueueXfer& enqueue = messages.first;
    CUDA_CompletedXfer& completed = messages.second;

    // Add this event's call site to the blob generator

    enqueue.call_site = generator.addSite(dm_sites[event.call_site]);
    
    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }
    
    // Add this event's enqueue message to the blob generator

    CBTF_cuda_message* enqueue_message = generator.addMessage();
    
    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }

    enqueue_message->type = EnqueueXfer;
    memcpy(&enqueue_message->CBTF_cuda_message_u.enqueue_xfer,
           &enqueue, sizeof(CUDA_EnqueueXfer));

    generator.updateHeader(event.time);
    
    // Add this event's completed message to the blob generator
    
    CBTF_cuda_message* completed_message = generator.addMessage();
    
    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }

    completed_message->type = CompletedXfer;
    memcpy(&completed_message->CBTF_cuda_message_u.completed_xfer,
           &completed, sizeof(CUDA_CompletedXfer));

    generator.updateHeader(event.time_begin);
    generator.updateHeader(event.time_end);

    // Continue the iteration
    return true;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool DataTable::generate(const KernelExecution& event,
                         BlobGenerator& generator) const
{
    // Convert the event into the corresponding enqueue and completed messages
    std::pair<CUDA_EnqueueExec, CUDA_CompletedExec> messages = convert(event);
    CUDA_EnqueueExec& enqueue = messages.first;
    CUDA_CompletedExec& completed = messages.second;

    // Add this event's call site to the blob generator

    enqueue.call_site = generator.addSite(dm_sites[event.call_site]);

    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }

    // Add this event's enqueue message to the blob generator

    CBTF_cuda_message* enqueue_message = generator.addMessage();
    
    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }

    enqueue_message->type = EnqueueExec;
    memcpy(&enqueue_message->CBTF_cuda_message_u.enqueue_exec,
           &enqueue, sizeof(CUDA_EnqueueExec));

    generator.updateHeader(event.time);

    // Add this event's completed message to the blob generator
    
    CBTF_cuda_message* completed_message = generator.addMessage();
    
    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }

    completed_message->type = CompletedExec;
    memcpy(&completed_message->CBTF_cuda_message_u.completed_exec,
           &completed, sizeof(CUDA_CompletedExec));

    generator.updateHeader(event.time_begin);
    generator.updateHeader(event.time_end);

    // Continue the iteration
    return true;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_CompletedExec& message,
                        PerProcessData& per_process)
{
    process(per_process.dm_partial_kernel_executions.addCompleted(
                message.id, convert(message)
                ));
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_CompletedXfer& message,
                        PerProcessData& per_process)
{
    process(per_process.dm_partial_data_transfers.addCompleted(
                message.id, convert(message)
                ));
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_ContextInfo& message,
                        PerProcessData& per_process)
{
    process(per_process.dm_partial_data_transfers.addContext(
                message.context, message.device
                ));    
    process(per_process.dm_partial_kernel_executions.addContext(
                message.context, message.device
                ));
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_DeviceInfo& message,
                        PerHostData& per_host,
                        PerProcessData& per_process)
{
    if (per_host.dm_known_devices.find(message.device) != 
        per_host.dm_known_devices.end())
    {
        return;
    }

    per_host.dm_known_devices.insert(message.device);
    
    CUDA::Device device = convert(message);
    dm_devices.push_back(device);
    
    process(per_process.dm_partial_data_transfers.addDevice(
                message.device, dm_devices.size() - 1
                ));
    process(per_process.dm_partial_kernel_executions.addDevice(
                message.device, dm_devices.size() - 1
                ));
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_EnqueueExec& message,
                        const struct CBTF_cuda_data& data,
                        const Base::ThreadName& thread,
                        PerProcessData& per_process)
{
    KernelExecution event = convert(message);
    event.call_site = findSite(message.call_site, data);
    
    process(per_process.dm_partial_kernel_executions.addEnqueued(
                message.id, event, message.context, thread
                ));
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_EnqueueXfer& message,
                        const struct CBTF_cuda_data& data,
                        const Base::ThreadName& thread,
                        PerProcessData& per_process)
{
    DataTransfer event = convert(message);
    event.call_site = findSite(message.call_site, data);

    process(per_process.dm_partial_data_transfers.addEnqueued(
                message.id, event, message.context, thread
                ));
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_OverflowSamples& message,
                        PerThreadData& per_thread)
{
    // TODO: Eventually we should make overflow samples available too!
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const struct CUDA_PeriodicSamples& message,
                        PerThreadData& per_thread)
{
    const boost::uint8_t* begin = &message.deltas.deltas_val[0];

    const boost::uint8_t* end = 
        &message.deltas.deltas_val[message.deltas.deltas_len];
    
    if (!per_thread.dm_counters.empty())
    {
        processPeriodicSamples(begin, end, per_thread);
    }
    else
    {
        per_thread.dm_unprocessed_periodic_samples.push_back(
            std::vector<boost::uint8_t>(begin, end)
            );
    }
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const CUDA_SamplingConfig& message,
                        PerThreadData& per_thread)
{
    if (!per_thread.dm_counters.empty())
    {
        Base::raise<std::runtime_error>(
            "Encountered multiple CUDA_SamplingConfig for a thread."
            );
    }
    
    for (u_int i = 0; i < message.events.events_len; ++i)
    {
        std::string name(message.events.events_val[i].name);

        std::vector<std::string>::size_type j;
        for (j = 0; j < dm_counters.size(); ++j)
        {
            if (dm_counters[j] == name)
            {
                break;
            }
        }
        if (j == dm_counters.size())
        {
            dm_counters.push_back(name);
        }

        per_thread.dm_counters.push_back(j);
    }
    
    for (std::vector<std::vector<boost::uint8_t> >::const_iterator
             i = per_thread.dm_unprocessed_periodic_samples.begin();
         i != per_thread.dm_unprocessed_periodic_samples.end();
         ++i)
    {
        processPeriodicSamples(&(*i->begin()), &(*i->end()), per_thread);
    }
    
    per_thread.dm_unprocessed_periodic_samples.clear();
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(
    const PartialEventTable<DataTransfer>::Completions& completions
    )
{
    for (PartialEventTable<DataTransfer>::Completions::const_iterator
             i = completions.begin(); i != completions.end(); ++i)
    {
        PerThreadData& per_thread = accessPerThreadData(i->first);
        per_thread.dm_data_transfers.add(i->second);
        
        dm_interval |= i->second.time;
        dm_interval |= i->second.time_begin;
        dm_interval |= i->second.time_end;
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(
    const PartialEventTable<KernelExecution>::Completions& completions
    )
{
    for (PartialEventTable<KernelExecution>::Completions::const_iterator
             i = completions.begin(); i != completions.end(); ++i)
    {
        PerThreadData& per_thread = accessPerThreadData(i->first);
        per_thread.dm_kernel_executions.add(i->second);
        
        dm_interval |= i->second.time;
        dm_interval |= i->second.time_begin;
        dm_interval |= i->second.time_end;
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::processPeriodicSamples(const boost::uint8_t* begin,
                                       const boost::uint8_t* end,
                                       PerThreadData& per_thread)
{
    static int kAdditionalBytes[4] = { 0, 2, 3, 8 };

    std::vector<uint64_t>::size_type n = 0;
    std::vector<uint64_t>::size_type N = 1 + per_thread.dm_counters.size();

    std::vector<uint64_t> samples(N, 0);

    for (const boost::uint8_t* ptr = begin; ptr != end;)
    {
        boost::uint8_t encoding = *ptr >> 6;

        boost::uint64_t delta = 0;
        if (encoding < 3)
        {
            delta = static_cast<boost::uint64_t>(*ptr) & 0x3F;
        }
        ++ptr;
        for (int i = 0; i < kAdditionalBytes[encoding]; ++i)
        {
            delta <<= 8;
            delta |= static_cast<boost::uint64_t>(*ptr++);
        }

        samples[n++] += delta;

        if (n == N)
        {
            per_thread.dm_periodic_samples.insert(
                std::make_pair(
                    samples[0],
                    std::vector<uint64_t>(samples.begin() + 1, samples.end())
                    )
                );
            n = 0;
        }
    }
}
