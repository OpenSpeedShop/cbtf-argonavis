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
    dm_sites()
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const CBTF_cuda_data& message)
{
    for (u_int i = 0; i < data.messages.messages_len; ++i)
    {
        const CBTF_cuda_message& message = data.messages.messages_val[i];
        
        switch (message.type)
        {

        case CompletedExec:
            process(thread, message.CBTF_cuda_message_u.completed_exec);
            break;

        case CompletedXfer:
            process(thread, message.CBTF_cuda_message_u.completed_xfer);
            break;

        case ContextInfo:
            process(thread, message.CBTF_cuda_message_u.context_info);
            break;
            
        case DeviceInfo:
            process(message.CBTF_cuda_message_u.device_info);
            break;

        case EnqueueExec:
            process(thread, message.CBTF_cuda_message_u.enqueue_exec, data);
            break;

        case EnqueueXfer:
            process(thread, message.CBTF_cuda_message_u.enqueue_xfer, data);
            break;

        case OverflowSamples:
            process(thread, message.CBTF_cuda_message_u.overflow_samples);
            break;

        case PeriodicSamples:
            process(thread, message.CBTF_cuda_message_u.periodic_samples);
            break;
            
        case SamplingConfig:
            process(thread, message.CBTF_cuda_message_u.sampling_config);
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
//------------------------------------------------------------------------------
const std::vector<Device>& DataTable::devices() const
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
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_CompletedExec& message)
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_CompletedXfer& message)
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_ContextInfo& message)
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_DeviceInfo& message)
{
}

        

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_EnqueueExec& message,
                        const struct CBTF_cuda_data& data)
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_EnqueueXfer& message,
                        const struct CBTF_cuda_data& data)
{
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_OverflowSamples& message)
{
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_PeriodicSamples& message)
{
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void DataTable::process(const Base::ThreadName& thread,
                        const struct CUDA_SamplingConfig& message)
{
}
