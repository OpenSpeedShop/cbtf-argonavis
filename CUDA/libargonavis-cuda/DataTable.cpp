////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2017 Argo Navis Technologies. All Rights Reserved.
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
#include <ArgoNavis/Base/XDR.hpp>

#include <ArgoNavis/CUDA/CachePreference.hpp>
#include <ArgoNavis/CUDA/CopyKind.hpp>
#include <ArgoNavis/CUDA/CounterKind.hpp>
#include <ArgoNavis/CUDA/MemoryKind.hpp>

#include "DataTable.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::CUDA;
using namespace ArgoNavis::CUDA::Impl;



/** Anonymous namespace hiding implementation details. */
namespace {

    /** Convert a char* into a string. */
    std::string convert(char* value)
    {
        return (value == NULL) ? std::string() : std::string(value);
    }

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

    /** Convert a CUDA_EventKind into a CounterKind. */
    CounterKind convert(const CUDA_EventKind& value)
    {
        switch (value)
        {
        case UnknownEventKind: return kUnknownCounterKind;
        case Count: return kCount;
        case Percentage: return kPercentage;
        case Rate: return kRate;
        default: return kUnknownCounterKind;
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

    /** Convert a CUDA_EventDescription into a CounterDescription. */
    CounterDescription convert(const CUDA_EventDescription& message)
    {
        CounterDescription description;

        description.name = convert(message.name);
        description.kind = convert(message.kind);
        description.threshold = message.threshold;

        return description;
    }
    
    /** Convert a CUDA_CompletedExec into a (partial) KernelExecution. */
    KernelExecution convert(const CUDA_CompletedExec& message)
    {    
        KernelExecution event;

        // event.clas = <Not Present>
        // event.device = <Not Present>
        // event.call_site = <Not Present>
        event.id = message.id;
        // event.context = <Not Present>
        // event.stream = <Not Present>
        // event.time = <Not Present>
        event.time_begin = message.time_begin;
        event.time_end = message.time_end;
        event.function = convert(message.function);
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

        // event.clas = <Not Present>
        // event.device = <Not Present>
        // event.call_site = <Not Present>
        event.id = message.id;
        // event.context = <Not Present>
        // event.stream = <Not Present>
        // event.time = <Not Present>
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

        device.name = convert(message.name);
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

        // event.clas = <Not Present>
        // event.device = <Not Present>
        // event.call_site = <Present But Must Be Translated By Caller>
        event.id = message.id;
        event.context = message.context;
        event.stream = message.stream;
        event.time = message.time;
        // event.time_begin = <Not Present>
        // event.time_end = <Not Present>
        // event.function = <Not Present>
        // event.grid = <Not Present>
        // event.block = <Not Present>
        // event.cache_preference = <Not Present>
        // event.registers_per_thread = <Not Present>
        // event.static_shared_memory = <Not Present>
        // event.dynamic_shared_memory = <Not Present>
        // event.local_memory = <Not Present>

        return event;
    }

    /** Convert a CUDA_EnqueueXfer into a (partial) DataTransfer. */
    DataTransfer convert(const CUDA_EnqueueXfer& message)
    {
        DataTransfer event;

        // event.clas = <Not Present>
        // event.device = <Not Present>
        // event.call_site = <Present But Must Be Translated By Caller>
        event.id = message.id;
        event.context = message.context;
        event.stream = message.stream;
        event.time = message.time;
        // event.time_begin = <Not Present>
        // event.time_end = <Not Present>
        // event.size = <Not Present>
        // event.kind = <Not Present>
        // event.source_kind = <Not Present>
        // event.destination_kind = <Not Present>
        // event.asynchronous = <Not Present>

        return event;
    }

    /** Convert a CUDA_ExecClass into a (partial) KernelExecution. */
    KernelExecution convert(const CUDA_ExecClass& message)
    {
        KernelExecution event;

        event.clas = message.clas;
        // event.device = <Not Present>
        // event.call_site = <Present But Must Be Translated By Caller>
        // event.id = <Not Present>
        event.context = message.context;
        event.stream = message.stream;
        // event.time = <Not Present>
        // event.time_begin = <Not Present>
        // event.time_end = <Not Present>
        event.function = convert(message.function);
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

    /** Convert a CUDA_ExecInstance into a EventInstance. */
    EventInstance convert(const CUDA_ExecInstance& message)
    {
        EventInstance instance;

        instance.clas = message.clas;
        instance.id = message.id;
        instance.time = message.time;
        instance.time_begin = message.time_begin;
        instance.time_end = message.time_end;

        return instance;
    }

    /** Convert a CUDA_XferInstance into a (partial) DataTransfer. */
    DataTransfer convert(const CUDA_XferClass& message)
    {
        DataTransfer event;

        event.clas = message.clas;
        // event.device = <Not Present>
        // event.call_site = <Present But Must Be Translated By Caller>
        // event.id = <Not Present>
        event.context = message.context;
        event.stream = message.stream;        
        // event.time = <Not Present>
        // event.time_begin = <Not Present>
        // event.time_end = <Not Present>
        event.size = message.size;
        event.kind = convert(message.kind);
        event.source_kind = convert(message.source_kind);
        event.destination_kind = convert(message.destination_kind);
        event.asynchronous = message.asynchronous;

        return event;
    }

    /** Convert a CUDA_XferInstance into a EventInstance. */
    EventInstance convert(const CUDA_XferInstance& message)
    {
        EventInstance instance;

        instance.clas = message.clas;
        instance.id = message.id;
        instance.time = message.time;
        instance.time_begin = message.time_begin;
        instance.time_end = message.time_end;

        return instance;
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

    /** Convert a CounterKind into a CUDA_EventKind. */
    CUDA_EventKind convert(const CounterKind& value)
    {
        switch (value)
        {
        case kUnknownCounterKind: return UnknownEventKind;
        case kCount: return Count;
        case kPercentage: return Percentage;
        case kRate: return Rate;
        default: return UnknownEventKind;
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

    /** Convert a CounterDescription into a CUDA_EventDescription. */
    CUDA_EventDescription convert(const CounterDescription& description)
    {
        CUDA_EventDescription message;

        message.name = strdup(description.name.c_str());
        message.kind = convert(description.kind);
        message.threshold = description.threshold;

        return message;
    }
    
    /** Convert a Device into a CUDA_DeviceInfo. */
    CUDA_DeviceInfo convert(const ArgoNavis::CUDA::Device& device)
    {
        CUDA_DeviceInfo message;

        // message.device = <Caller Must Provide This>
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
    
    /** Convert a DataTransfer into a CUDA_XferClass. */
    CUDA_XferClass convert(const DataTransfer& event)
    {
        CUDA_XferClass message;

        message.clas = event.clas;
        message.context = event.context;
        message.stream = event.stream;
        // message.call_site = <Caller Must Provide This>
        message.size = event.size;
        message.kind = convert(event.kind);
        message.source_kind = convert(event.source_kind);
        message.destination_kind = convert(event.destination_kind);
        message.asynchronous = event.asynchronous ? TRUE : FALSE;
        
        return message;        
    }

    /** Convert a KernelExecution into a CUDA_ExecClass. */
    CUDA_ExecClass convert(const KernelExecution& event)
    {
        CUDA_ExecClass message;

        message.clas = event.clas;
        message.context = event.context;
        message.stream = event.stream;
        // message.call_site = <Caller Must Provide This>
        message.function = strdup(event.function.c_str());
        message.grid[0] = event.grid.get<0>();
        message.grid[1] = event.grid.get<1>();
        message.grid[2] = event.grid.get<2>();
        message.block[0] = event.block.get<0>();
        message.block[1] = event.block.get<1>();
        message.block[2] = event.block.get<2>();
        message.cache_preference = convert(event.cache_preference);
        message.registers_per_thread = event.registers_per_thread;
        message.static_shared_memory = event.static_shared_memory;
        message.dynamic_shared_memory = event.dynamic_shared_memory;
        message.local_memory = event.local_memory;

        return message;
    }

    /** Convert a KernelInstance into a CUDA_[Exec|Xfer]Instance. */
    template <typename T>
    T convert(const EventInstance& instance)
    {
        T message;

        message.clas = instance.clas;
        message.id = instance.id;
        message.time = instance.time;
        message.time_begin = instance.time_begin;
        message.time_end = instance.time_end;

        return message;    
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

        case ::PeriodicSamples:
            process(raw.CBTF_cuda_message_u.periodic_samples, per_thread);
            break;
            
        case SamplingConfig:
            process(raw.CBTF_cuda_message_u.sampling_config, per_thread);
            break;

        case ExecClass:
            process(raw.CBTF_cuda_message_u.exec_class, message, per_process,
                    per_thread);
            break;

        case ExecInstance:
            process(raw.CBTF_cuda_message_u.exec_instance, per_thread);
            break;

        case XferClass:
            process(raw.CBTF_cuda_message_u.xfer_class, message, per_process,
                    per_thread);
            break;

        case XferInstance:
            process(raw.CBTF_cuda_message_u.xfer_instance, per_thread);
            break;

        }
    }
}



//------------------------------------------------------------------------------
// Note that it doesn't matter whether we search the partial data transfers or
// kernel executions for the device information as both tables contain exactly
// the same CUDA context to device mapping.
//------------------------------------------------------------------------------
boost::optional<std::size_t> DataTable::device(const ThreadName& thread) const
{
    PerProcessData& per_process = 
        const_cast<DataTable*>(this)->accessPerProcessData(thread);
    
    try
    {
        boost::optional<boost::uint64_t> tid = thread.tid();
         
        if (!tid)
        {
            return boost::none;
        }

        Address context = *tid;
        
        boost::uint32_t id = 
            per_process.dm_partial_data_transfers.device(context);

        std::size_t index =
            per_process.dm_partial_data_transfers.index(id);
        
        return index;            
    }
    catch (...)
    {
        return boost::none;
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
    
    BlobGenerator generator(thread, visitor, dm_interval);

    // Generate the context/device information and sampling config messages

    generate(per_process, per_thread, generator);

    if (generator.terminate())
    {
        return; // Terminate the iteration
    }
    
    // Add all of the data transfer classes to the generator

    per_thread.dm_data_transfers.visitClasses(boost::bind(
        &DataTable::generateXferClass, this, _1, boost::ref(generator)
        ));

    if (generator.terminate())
    {
        return; // Terminate the iteration
    }

    // Add all of the kernel execution classes to the generator

    per_thread.dm_kernel_executions.visitClasses(boost::bind(
        &DataTable::generateExecClass, this, _1, boost::ref(generator)
        ));

    if (generator.terminate())
    {
        return; // Terminate the iteration
    }
    
    // Add all of the data transfer instances to the generator

    per_thread.dm_data_transfers.visitInstances(boost::bind(
        &DataTable::generateXferInstance, this, _1, boost::ref(generator)
        ));

    if (generator.terminate())
    {
        return; // Terminate the iteration
    }

    // Add all of the kernel execution instances to the generator

    per_thread.dm_kernel_executions.visitInstances(boost::bind(
        &DataTable::generateExecInstance, this, _1, boost::ref(generator)
        ));

    if (generator.terminate())
    {
        return; // Terminate the iteration
    }
        
    // Add the periodic samples to the generator
    
    for (PeriodicSamples::const_iterator
             i = per_thread.dm_periodic_samples.begin();
         (i != per_thread.dm_periodic_samples.end()) && !generator.terminate();
         ++i)
    {
        generator.addPeriodicSample(i->first, i->second);
    }

    if (generator.terminate())
    {
        return; // Terminate the iteration
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
void DataTable::generate(const PerProcessData& per_process,
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
            return; // Terminate the iteration
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
            return; // Terminate the iteration
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
        return; // Terminate the iteration
    }
        
    message->type = SamplingConfig;
    CUDA_SamplingConfig& config = message->CBTF_cuda_message_u.sampling_config;

    config.interval = 0; // TODO: Use the real interval!?

    config.events.events_len = per_thread.dm_counters.size();

    config.events.events_val =
        allocateXDRCountedArray<CUDA_EventDescription>(
            config.events.events_len
            );
    
    for (std::vector<std::vector<std::string>::size_type>::const_iterator
             i = per_thread.dm_counters.begin();
         i  != per_thread.dm_counters.end();
         ++i)
    {
        config.events.events_val[*i] = convert(dm_counters[*i]);
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool DataTable::generateExecClass(const KernelExecution& clas,
                                  BlobGenerator& generator) const
{
    // Convert the event class into the corresponding message
    CUDA_ExecClass class_message = convert(clas);
    
    // Add this event class' call site to the blob generator
    //
    // Do NOT attempt to move the addSite() call after the addMessage() call.
    // The former also insures that at least one message can be added to the
    // blob; insuring that the call site and its referencing message are not
    // split between two blobs, which would be disastrous.

    class_message.call_site = generator.addSite(dm_sites[clas.call_site]);
    
    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }

    // Add this event class' message to the blob generator

    CBTF_cuda_message* message = generator.addMessage();
    
    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }

    message->type = ExecClass;

    memcpy(&message->CBTF_cuda_message_u.exec_class,
           &class_message, sizeof(CUDA_ExecClass));

    // Continue the iteration
    return true;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool DataTable::generateExecInstance(const EventInstance& instance,
                                     BlobGenerator& generator) const
{
    // Add this event instance's message to the blob generator

    CBTF_cuda_message* message = generator.addMessage();

    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }

    message->type = ExecInstance;

    CUDA_ExecInstance& exec_instance = 
        message->CBTF_cuda_message_u.exec_instance;

    exec_instance = convert<CUDA_ExecInstance>(instance);

    // Continue the iteration
    return true;    
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool DataTable::generateXferClass(const DataTransfer& clas,
                                  BlobGenerator& generator) const
{
    // Convert the event class into the corresponding message
    CUDA_XferClass class_message = convert(clas);

    // Add this event class' call site to the blob generator
    //
    // Do NOT attempt to move the addSite() call after the addMessage() call.
    // The former also insures that at least one message can be added to the
    // blob; insuring that the call site and its referencing message are not
    // split between two blobs, which would be disastrous.

    class_message.call_site = generator.addSite(dm_sites[clas.call_site]);
    
    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }

    // Add this event class' message to the blob generator

    CBTF_cuda_message* message = generator.addMessage();
    
    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }

    message->type = XferClass;

    memcpy(&message->CBTF_cuda_message_u.xfer_class,
           &class_message, sizeof(CUDA_XferClass));

    // Continue the iteration
    return true;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool DataTable::generateXferInstance(const EventInstance& instance,
                                     BlobGenerator& generator) const
{
    // Add this event instance's message to the blob generator

    CBTF_cuda_message* message = generator.addMessage();

    if (generator.terminate())
    {
        return false; // Terminate the iteration
    }

    message->type = XferInstance;

    CUDA_XferInstance& xfer_instance = 
        message->CBTF_cuda_message_u.xfer_instance;

    xfer_instance = convert<CUDA_XferInstance>(instance);

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
        CounterDescription description = convert(message.events.events_val[i]);
        
        std::vector<CounterDescription>::size_type j;
        for (j = 0; j < dm_counters.size(); ++j)
        {
            if (dm_counters[j].name == description.name)
            {
                break;
            }
        }
        if (j == dm_counters.size())
        {
            dm_counters.push_back(description);
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
void DataTable::process(const CUDA_ExecClass& message,
                        const CBTF_cuda_data& data,
                        const PerProcessData& per_process,
                        PerThreadData& per_thread)
{
    KernelExecution event = convert(message);

    event.device = per_process.dm_partial_kernel_executions.index(
        per_process.dm_partial_kernel_executions.device(message.context)
        );

    event.call_site = findSite(message.call_site, data);

    per_thread.dm_kernel_executions.addClass(event);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const CUDA_ExecInstance& message,
                        PerThreadData& per_thread)    
{
    EventInstance instance = convert(message);

    per_thread.dm_kernel_executions.addInstance(instance);

    dm_interval |= instance.time;
    dm_interval |= instance.time_begin;
    dm_interval |= instance.time_end;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const CUDA_XferClass& message,
                        const CBTF_cuda_data& data,
                        const PerProcessData& per_process,
                        PerThreadData& per_thread)
{
    DataTransfer event = convert(message);
    
    event.device = per_process.dm_partial_data_transfers.index(
        per_process.dm_partial_data_transfers.device(message.context)
        );
    
    event.call_site = findSite(message.call_site, data);
    
    per_thread.dm_data_transfers.addClass(event);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const CUDA_XferInstance& message,
                        PerThreadData& per_thread)
{
    EventInstance instance = convert(message);
    
    per_thread.dm_data_transfers.addInstance(instance);

    dm_interval |= instance.time;
    dm_interval |= instance.time_begin;
    dm_interval |= instance.time_end;
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

    std::vector<boost::uint64_t>::size_type n = 0;
    std::vector<boost::uint64_t>::size_type N = 1 + per_thread.dm_counters.size();

    std::vector<boost::uint64_t> samples(N, 0);

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
            dm_interval |= Time(samples[0]);
            
            per_thread.dm_periodic_samples.insert(
                std::make_pair(
                    samples[0],
                    std::vector<boost::uint64_t>(
                        samples.begin() + 1, samples.end()
                        )
                    )
                );
            n = 0;
        }
    }
}
