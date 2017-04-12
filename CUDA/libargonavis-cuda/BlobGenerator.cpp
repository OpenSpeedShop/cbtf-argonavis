////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015-2017 Argo Navis Technologies. All Rights Reserved.
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

#include <boost/assert.hpp>
#include <cstring>
#include <stddef.h>
#include <stdlib.h>

#include <KrellInstitute/Messages/Blob.h>
#include <KrellInstitute/Messages/PerformanceData.hpp>

#include "BlobGenerator.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::CUDA;
using namespace ArgoNavis::CUDA::Impl;



/** Anonymous namespace hiding implementation details. */
namespace {

    /**
     * String uniquely identifying the CUDA collector. This must match the value
     * cbtf_collector_unique_id in "cbtf-argonavis/CUDA/collector/collector.c",
     * or other tools (e.g. Open|SpeedShop) won't know what to do with the blobs
     * generated by this class.
     */
    const char* const kCollectorUniqueID = "cuda";

    /**
     * Maximum number of stack trace addresses contained within each blob.
     *
     * @note    Currently the only basis for this selection is that it matches
     *          MAX_ADDRESSES_PER_BLOB in "cbtf-argonavis/CUDA/collector/TLS.h".
     */
    const std::size_t kMaxAddressesPerBlob = 1024;

    /**
     * Maximum number of periodic sample delta bytes within each blob.
     *
     * @note    Currently the only basis for this selection
     *          is that it matches MAX_DELTAS_BYTES_PER_BLOB
     *          in "cbtf-argonavis/CUDA/collector/TLS.h".
     */
    const std::size_t kMaxDeltaBytesPerBlob = 32 * 1024 /* 32 KB */;
    
    /**
     * Maximum number of individual messages contained within each blob.
     *
     * @note    Currently the only basis for this selection is that it matches
     *          MAX_MESSAGES_PER_BLOB in "cbtf-argonavis/CUDA/collector/TLS.h".
     */
    const std::size_t kMaxMessagesPerBlob = 128;
    
} // namespace <anonymous>



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
BlobGenerator::BlobGenerator(const Base::ThreadName& thread,
                             const BlobVisitor& visitor) :
    dm_thread(thread),
    dm_visitor(visitor),
    dm_empty(true),
    dm_terminate(false),
    dm_header(),
    dm_data(),    
    dm_periodic_samples(),
    dm_periodic_samples_previous()
{
    initialize();
}



//------------------------------------------------------------------------------
// Generate a blob for the remaining data (if any).
//------------------------------------------------------------------------------
BlobGenerator::~BlobGenerator()

{
    if (!dm_terminate && 
        ((dm_data->messages.messages_len > 0) ||
         (dm_periodic_samples.deltas.deltas_len > 0)))
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
    dm_empty = false;

    // Generate a blob for the current header and data if there isn't enough
    // room to hold another message. See the note in this method's header.

    if (full())
    {
        generate();
    }

    // Iterate over the addresses in the existing stack traces
    int i, j;
    for (i = 0, j = 0; i < kMaxAddressesPerBlob; ++i)
    {
        // Is this the terminating null of an existing stack trace?
        if(dm_data->stack_traces.stack_traces_val[i] = 0)
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
                     (dm_data->stack_traces.stack_traces_val[i - 1] == 0))
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
                    dm_data->stack_traces.stack_traces_val[i] = site[j];
                    updateHeader(site[j]);
                }
                dm_data->stack_traces.stack_traces_val[i] = 0;
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

            j = (site[j] ==
                    Address(dm_data->stack_traces.stack_traces_val[i])) ?
                (j + 1) : 0;
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
    dm_empty = false;

    if (full())
    {
        generate();
    }
    
    return &dm_data->messages.messages_val[dm_data->messages.messages_len++];
}



//------------------------------------------------------------------------------
// This method is almost identical to timer_callback() found in
// "cbtf-argonavis/CUDA/collector/PAPI.c".
//------------------------------------------------------------------------------
void BlobGenerator::addPeriodicSample(
    boost::uint64_t time,
    const std::vector<boost::uint64_t>& counts
    )
{
    dm_empty = false;

    std::vector<boost::uint64_t> sample;
    sample.push_back(time);
    sample.insert(sample.end(), counts.begin(), counts.end());
    
    if (dm_periodic_samples_previous.empty())
    {
        dm_periodic_samples_previous.assign(sample.size(), 0);
    }
    
    BOOST_ASSERT(sample.size() == dm_periodic_samples_previous.size());

    // Get a pointer to the periodic samples deltas for this blob
    boost::uint8_t* deltas = dm_periodic_samples.deltas.deltas_val;
    
    // Get the current index within the periodic samples deltas. The length
    // isn't updated until the ENTIRE event sample encoding has been added.
    // This facilitates easy restarting of the encoding in the event there
    // isn't enough room left in the array for the entire encoding.

    int index = dm_periodic_samples.deltas.deltas_len;
    
    // Get pointers to the values in the new (current) and previous event
    // samples. Note that the time and the actual event counts are treated
    // identically as 64-bit unsigned integers

    const boost::uint64_t* previous = &dm_periodic_samples_previous[0];
    const boost::uint64_t* current = &sample[0];

    // Iterate over each time and event count value in this event sample
    for (int i = 0, i_end = sample.size();
         i < i_end;
         ++i, ++previous, ++current)
    {
        // Compute the delta between the previous and current samples for this
        // value. The previous event sample is zeroed above as needed so there
        // is no need to treat the first sample specially here.
        
        boost::uint64_t delta = *current - *previous;

        // Select the appropriate top 2 bits of the first encoded byte (called
        // the "prefix" here) and number of bytes in the encoding based on the
        // actual numerical magnitude of the delta.

        boost::uint8_t prefix = 0;
        int num_bytes = 0;
        
        if (delta < 0x3FULL)
        {
            prefix = 0x00;
            num_bytes = 1;
        }
        else if (delta < 0x3FFFFFULL)
        {
            prefix = 0x40;
            num_bytes = 3;
        }
        else if (delta < 0x3FFFFFFFULL)
        {
            prefix = 0x80;
            num_bytes = 4;
        }
        else
        {
            prefix = 0xC0;
            num_bytes = 9;
        }
 
        // Generate a blob for the current header and data if there isn't
        // enough room in the existing periodic samples deltas array to add
        // this delta. Doing so frees up enough space for this delta. Restart
        // this event sample's encoding by reseting the loop variables, keeping
        // in mind that loop increment expresions are still applied after the
        // continue statement;

        if ((index + num_bytes) > kMaxDeltaBytesPerBlob)
        {
            generate();
            index = dm_periodic_samples.deltas.deltas_len;
            previous = &dm_periodic_samples_previous[0] - 1;
            current = &sample[0] - 1;
            i = -1;
            continue;
        }
        
        // Add the encoding of this delta to the periodic samples deltas one
        // byte at a time. The loop adds the full bytes from the delta, last
        // byte first, allowing the use of fixed-size shift. Finally, after
        // the loop, add the first byte, which includes the encoding prefix
        // and the first 6 bits of the delta.

        int j;
        for (j = index + num_bytes - 1; j > index; --j, delta >>= 8)
        {
            deltas[j] = delta & 0xFF;
        }
        deltas[j] = prefix | (delta & 0x3F);
        
        // Advance the current index within the periodic samples deltas
        index += num_bytes;
    }

    // Update the length of the periodic samples deltas array
    dm_periodic_samples.deltas.deltas_len = index;
    
    // Update the header with this sample time
    updateHeader(Time(time));
    
    // Replace the previous event sample with the new event sample
    dm_periodic_samples_previous.swap(sample);
}


        
//------------------------------------------------------------------------------
// This method is almost identical to TLS_update_header_with_address() found in
// "cbtf-argonavis/CUDA/collector/TLS.c".
//------------------------------------------------------------------------------
void BlobGenerator::updateHeader(const Address& address)
{
    if (address < Address(dm_header->addr_begin))
    {
        dm_header->addr_begin = address;
    }
    if (address >= Address(dm_header->addr_end))
    {
        dm_header->addr_end = address + 1;
    }
}



//------------------------------------------------------------------------------
// This method is almost identical to TLS_update_header_with_time() found in
// "cbtf-argonavis/CUDA/collector/TLS.c".
//------------------------------------------------------------------------------
void BlobGenerator::updateHeader(const Time& time)
{
    if (time < Time(dm_header->time_begin))
    {
        dm_header->time_begin = time;
    }
    if (time >= Time(dm_header->time_end))
    {
        dm_header->time_end = time + 1;
    }
}



//------------------------------------------------------------------------------
// This method is roughly equivalent to TLS_initialize_data() found in
// "cbtf-argonavis/CUDA/collector/TLS.c".
//------------------------------------------------------------------------------
void BlobGenerator::initialize()
{
    dm_header.reset(new CBTF_DataHeader());

    *dm_header = dm_thread; // Set fields from ThreadName
    dm_header->experiment = 0;
    dm_header->collector = 1;
    dm_header->id = const_cast<char*>(kCollectorUniqueID);
    dm_header->time_begin = ~0;
    dm_header->time_end = 0;
    dm_header->addr_begin = ~0;
    dm_header->addr_end = 0;

    dm_data.reset(new CBTF_cuda_data());
    
    dm_data->messages.messages_len = 0;
    dm_data->messages.messages_val =
        reinterpret_cast<CBTF_cuda_message*>(malloc(
            kMaxMessagesPerBlob * sizeof(CBTF_cuda_message)
            ));
    
    dm_data->stack_traces.stack_traces_len = 0;
    dm_data->stack_traces.stack_traces_val =
        reinterpret_cast<CBTF_Protocol_Address*>(malloc(
            kMaxAddressesPerBlob * sizeof(CBTF_Protocol_Address)
            ));

    memset(dm_data->stack_traces.stack_traces_val, 0,
           kMaxAddressesPerBlob * sizeof(CBTF_Protocol_Address));         

    dm_periodic_samples.deltas.deltas_len = 0;
    dm_periodic_samples.deltas.deltas_val =
        reinterpret_cast<boost::uint8_t*>(malloc(
            kMaxDeltaBytesPerBlob * sizeof(boost::uint8_t)
            ));
    
    dm_periodic_samples_previous.clear();
}



//------------------------------------------------------------------------------
// This method is roughly equivalent to is_full() found in
// "cbtf-argonavis/CUDA/collector/TLS.c".
//------------------------------------------------------------------------------
bool BlobGenerator::full() const
{
    std::size_t max_messages_per_blob = kMaxMessagesPerBlob;

    if (dm_periodic_samples.deltas.deltas_len > 0)
    {
        --max_messages_per_blob;
    }
    
    return dm_data->messages.messages_len == max_messages_per_blob;
}



//------------------------------------------------------------------------------
// This method is roughly equivalent to TLS_send_data() found in
// "cbtf-argonavis/CUDA/collector/TLS.c".
//------------------------------------------------------------------------------
void BlobGenerator::generate()
{
    if (dm_periodic_samples.deltas.deltas_len > 0)
    {
        CBTF_cuda_message* raw_message = 
            &dm_data->messages.messages_val[dm_data->messages.messages_len++];
        raw_message->type = PeriodicSamples;
        
        CUDA_PeriodicSamples* message =
            &raw_message->CBTF_cuda_message_u.periodic_samples;
        
        memcpy(message, &dm_periodic_samples, sizeof(CUDA_PeriodicSamples));

        // When generating a blob containing periodic samples, if the header's
        // address range is undefined, replace it with a range that covers ALL
        // addresses. Without this special case Open|SpeedShop tosses out data
        // blobs produced by the CUTPI metrics and events sampling thread.

        if ((dm_header->addr_begin == ~0) && (dm_header->addr_end == 0))
        {
            dm_header->addr_begin = 0;
            dm_header->addr_end	= ~0;
        }
    }
    
    boost::shared_ptr<CBTF_Protocol_Blob> blob =
        KrellInstitute::Messages::pack<CBTF_cuda_data>(
            std::make_pair(dm_header, dm_data),
            reinterpret_cast<xdrproc_t>(&xdr_CBTF_cuda_data)
            );
    
    dm_terminate |= !dm_visitor(blob);
    
    initialize();
}
