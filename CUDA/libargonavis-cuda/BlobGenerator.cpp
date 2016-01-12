////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015,2016 Argo Navis Technologies. All Rights Reserved.
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

/** @file Definition of the BlobGenerator class. */

#include <stddef.h>

#include <KrellInstitute/Messages/Blob.h>
#include <KrellInstitute/Messages/PerformanceData.hpp>

#include "BlobGenerator.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::CUDA;
using namespace ArgoNavis::CUDA::Impl;



/** Anonymous namespace hiding implementation details. */
namespace {

    /**
     * Maximum number of (CBTF_Protocol_Address) stack trace addresses contained
     * within each (CBTF_cuda_data) performance data blob.
     *
     * @note    Currently the only basis for this selection is that it matches
     *          MAX_ADDRESSES_PER_BLOB in "cbtf-argonavis/CUDA/collector/TLS.h".
     */
    const std::size_t kMaxAddressesPerBlob = 1024;
    
    /**
     * Maximum number of individual messages contained within each data blob.

     * Maximum number of individual (CBTF_cuda_messages) messages contained
     * within each (CBTF_cuda_data) performance data blob.
     *
     * @note    Currently the only basis for this selection is that it matches
     *          MAX_MESSAGES_PER_BLOB in "cbtf-argonavis/CUDA/collector/TLS.h".
     */
    const std::size_t kMaxMessagesPerBlob = 128;
    
} // namespace <anonymous>



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BlobGenerator::BlobGenerator(BlobVisitor& visitor) :
    dm_visitor(visitor),
    dm_terminate(false),
    dm_header(new CBTF_DataHeader()),
    dm_data(new CBTF_cuda_data()),
    dm_messages(kMaxMessagesPerBlob),
    dm_stack_traces(kMaxAddressesPerBlob)
{
    initialize();
}



//------------------------------------------------------------------------------
// ...
//
// TODO: Add condition to check for un-generated HWC data.
//------------------------------------------------------------------------------
BlobGenerator::~BlobGenerator()
{
    if (!dm_terminate && (dm_data->messages.messages_len > 0))
    {
        generate();
    }
}



//------------------------------------------------------------------------------
// This method is almost identical to TLS_add_current_call_site() found in
// "cbtf-argonavis/CUDA/collector/TLS.c".
//------------------------------------------------------------------------------
boost::uint32_t BlobGenerator::addSite(const StackTrace& site)
{
    int i, j;

    // Iterate over the addresses in the existing stack traces
    for (i = 0, j = 0; i < kMaxAddressesPerBlob; ++i)
    {
        // Is this the terminating null of an existing stack trace?
        if (dm_stack_traces[i] == 0)
        {
            // Terminate the search if a complete match has been found
            // between this stack trace and the existing stack trace.

            if (j == site.size())
            {
                break;
            }

            // Otherwise check for a null in the first or last entry, or
            // for consecutive nulls, all of which indicate the end of the
            // existing stack traces, and the need to add this stack trace
            // to the existing stack traces.
            
            else if ((i == 0) || 
                     (i == (kMaxAddressesPerBlob - 1)) ||
                     (dm_stack_traces[i - 1] == 0))
            {
                // Generate a blob for the current header and data if there
                // isn't enough room in the existing stack traces to add this
                // stack trace. Doing so frees up enough space for this stack
                // trace.
               
                if ((i + site.size()) >= kMaxAddressesPerBlob)
                {
                    generate();
                    i = 0;
                }
                
                // Add this stack trace to the existing stack traces
                for (j = 0; j < site.size(); ++j, ++i)
                {
                    dm_stack_traces[i] = site[j];
                    updateHeader(site[j]);
                }
                dm_stack_traces[i] = 0;
                dm_data->stack_traces.stack_traces_len = i + 1;
                
                break;
            }
            
            // Otherwise reset the pointer within this stack trace to zero
            else
            {
                j = 0;
            }
        }
        else
        {
            // Advance the pointer within this stack trace if the current
            // address within this stack trace matches the current address
            // within the existing stack traces. Otherwise reset the pointer
            // to zero.
            j = (site[j] == Base::Address(dm_stack_traces[i])) ? (j + 1) : 0;
        }
    }

    // Return the index of this stack trace within the existing stack traces
    return i - site.size();
}



//------------------------------------------------------------------------------
// This method is almost identical to TLS_add_message() found in
// "cbtf-argonavis/CUDA/collector/TLS.c".
//------------------------------------------------------------------------------
CBTF_cuda_message* BlobGenerator::addMessage()
{
    if (dm_data->messages.messages_len == kMaxMessagesPerBlob)
    {
        generate();
    }

    return &(dm_data->messages.messages_val[dm_data->messages.messages_len++]);
}


        
//------------------------------------------------------------------------------
// This method is almost identical to TLS_update_header_with_address() found in
// "cbtf-argonavis/CUDA/collector/TLS.c".
//------------------------------------------------------------------------------
void BlobGenerator::updateHeader(const Base::Address& address)
{
    if (address < Base::Address(dm_header->addr_begin))
    {
        dm_header->addr_begin = address;
    }
    if (address >= Base::Address(dm_header->addr_end))
    {
        dm_header->addr_end = address + 1;
    }
}



//------------------------------------------------------------------------------
// This method is almost identical to TLS_update_header_with_time() found in
// "cbtf-argonavis/CUDA/collector/TLS.c".
//------------------------------------------------------------------------------
void BlobGenerator::updateHeader(const Base::Time& time)
{
    if (time < Base::Time(dm_header->time_begin))
    {
        dm_header->time_begin = time;
    }
    if (time >= Base::Time(dm_header->time_end))
    {
        dm_header->time_end = time + 1;
    }
}



//------------------------------------------------------------------------------
// This method is almost identical to TLS_initialize_data() found in
// "cbtf-argonavis/CUDA/collector/TLS.c".
//------------------------------------------------------------------------------
void BlobGenerator::initialize()
{
    dm_header->time_begin = ~0;
    dm_header->time_end = 0;
    dm_header->addr_begin = ~0;
    dm_header->addr_end = 0;

    dm_data->messages.messages_len = 0;
    dm_data->messages.messages_val = &(dm_messages[0]);

    dm_data->stack_traces.stack_traces_len = 0;
    dm_data->stack_traces.stack_traces_val = &(dm_stack_traces[0]);

    dm_stack_traces.assign(kMaxAddressesPerBlob, 0);

    // ...
}



//------------------------------------------------------------------------------
// This method is roughly equivalent to TLS_send_data() found in
// "cbtf-argonavis/CUDA/collector/TLS.c".
//------------------------------------------------------------------------------
void BlobGenerator::generate()
{
    boost::shared_ptr<CBTF_Protocol_Blob> blob =
        KrellInstitute::Messages::pack<CBTF_cuda_data>(
            std::make_pair(dm_header, dm_data),
            *reinterpret_cast<xdrproc_t*>(&xdr_CBTF_cuda_data)
            );
    
    dm_terminate |= dm_visitor(blob);
    
    initialize();
}
