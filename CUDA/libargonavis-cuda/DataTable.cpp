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

#include "DataTable.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::CUDA;
using namespace ArgoNavis::CUDA::Impl;



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
    for (u_int i = 0; i < message.messages.messages_len; ++i)
    {
        const CBTF_cuda_message& raw = message.messages.messages_val[i];
        
        switch (raw.type)
        {
            
        case CompletedExec:
            process(thread, raw.CBTF_cuda_message_u.completed_exec);
            break;
            
        case CompletedXfer:
            process(thread, raw.CBTF_cuda_message_u.completed_xfer);
            break;

        case ContextInfo:
            process(thread, raw.CBTF_cuda_message_u.context_info);
            break;
            
        case DeviceInfo:
            process(thread, raw.CBTF_cuda_message_u.device_info);
            break;

        case EnqueueExec:
            process(thread, raw.CBTF_cuda_message_u.enqueue_exec, message);
            break;

        case EnqueueXfer:
            process(thread, raw.CBTF_cuda_message_u.enqueue_xfer, message);
            break;

        case OverflowSamples:
            process(thread, raw.CBTF_cuda_message_u.overflow_samples);
            break;

        case PeriodicSamples:
            process(thread, raw.CBTF_cuda_message_u.periodic_samples);
            break;
            
        case SamplingConfig:
            process(thread, raw.CBTF_cuda_message_u.sampling_config);
            break;
            
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const std::vector<std::string>& DataTable::counters() const
{
    return dm_counters;
}



//------------------------------------------------------------------------------
// The fully qualfified name below when referring to Device should NOT be
// required since we are using the ArgoNavis::CUDA namespace above. But GCC
// 4.8.2 was complaining that the template paramter was invalid if the fully
// qualified name wasn't used. So for now...
//------------------------------------------------------------------------------
const std::vector<ArgoNavis::CUDA::Device>& DataTable::devices() const
{
    return dm_devices;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const TimeInterval& DataTable::interval() const
{
    return dm_interval;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const std::vector<StackTrace>& DataTable::sites() const
{
    return dm_sites;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DataTable::PerHostData& DataTable::host(const ThreadName& thread)
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
DataTable::PerProcessData& DataTable::process(const ThreadName& thread)
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
DataTable::PerThreadData& DataTable::thread(const ThreadName& thread)
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
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_CompletedExec& message)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_CompletedXfer& message)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_ContextInfo& message)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_DeviceInfo& message)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_EnqueueExec& message,
                        const struct CBTF_cuda_data& data)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_EnqueueXfer& message,
                        const struct CBTF_cuda_data& data)
{
    // ...
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_OverflowSamples& message)
{
    // ...
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_PeriodicSamples& message)
{
    // ...
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_SamplingConfig& message)
{
    // ...
}
