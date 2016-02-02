/*******************************************************************************
** Copyright (c) 2012-2016 Argo Navis Technologies. All Rights Reserved.
**
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
** details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA  02111-1307  USA
*******************************************************************************/

/** @file Definition of the TLS support functions. */

#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <KrellInstitute/Services/Assert.h>
#include <KrellInstitute/Services/Collector.h>
#include <KrellInstitute/Services/Data.h>
#include <KrellInstitute/Services/TLS.h>
#include <KrellInstitute/Services/Unwind.h>

#include "collector.h"
#include "TLS.h"



#if defined(USE_EXPLICIT_TLS)
/**
 * Key used to look up our thread-local storage. This key <em>must</em> be
 * unique from any other key used by any of the CBTF services.
 */
static const uint32_t Key = 0xBADC00DA;
#else
/** Implicit thread-local storage. */
static __thread TLS Implicit;
#endif



/**
 * Allocate and zero-initialize the thread-local storage for the current thread.
 * This function <em>must</em> be called by a thread before that thread attempts
 * to call any of this file's other functions or memory corruption will result!
 */
void TLS_initialize()
{
#if defined(USE_EXPLICIT_TLS)
    TLS* tls = malloc(sizeof(TLS));
    Assert(tls != NULL);
    OpenSS_SetTLS(Key, tls);
#else
    TLS* tls = &Implicit;
#endif
    Assert(tls != NULL);
    memset(tls, 0, sizeof(TLS));
}



/**
 * Destroy the thread-local storage for the current thread.
 */
void TLS_destroy()
{
#if defined(USE_EXPLICIT_TLS)
    TLS* tls = CBTF_GetTLS(Key);
    Assert(tls != NULL);
    free(tls);
    OpenSS_SetTLS(Key, NULL);
#endif
}



/**
 * Access the thread-local storage for the current thread.
 *
 * @return    Thread-local storage for the current thread.
 */
TLS* TLS_get()
{
    /* Access our thread-local storage */
#if defined(USE_EXPLICIT_TLS)
    TLS* tls = CBTF_GetTLS(Key);
#else
    TLS* tls = &Implicit;
#endif
    Assert(tls != NULL);
    return tls;
}



/**
 * Initialize the performance data header and blob contained within the given
 * thread-local storage. This function <em>must</em> be called before any of
 * the collection routines attempts to add a message.
 *
 * @param tls    Thread-local storage to be initialized.
 */
void TLS_initialize_data(TLS* tls)
{
    Assert(tls != NULL);

    tls->data_header.time_begin = ~0;
    tls->data_header.time_end = 0;
    tls->data_header.addr_begin = ~0;
    tls->data_header.addr_end = 0;
    
    tls->data.messages.messages_len = 0;
    tls->data.messages.messages_val = tls->messages;
    
    tls->data.stack_traces.stack_traces_len = 0;
    tls->data.stack_traces.stack_traces_val = tls->stack_traces;
    
    memset(tls->stack_traces, 0, sizeof(tls->stack_traces));

#if defined(PAPI_FOUND)
    if (OverflowSamplingCount > 0)
    {
        CBTF_cuda_message* raw_message = 
            &(tls->messages[tls->data.messages.messages_len++]);
        raw_message->type = OverflowSamples;
        
        tls->overflow_samples.message =
            &raw_message->CBTF_cuda_message_u.overflow_samples;

        tls->overflow_samples.message->time_begin = ~0;
        tls->overflow_samples.message->time_end = 0;
        
        tls->overflow_samples.message->pcs.pcs_len = 0;
        tls->overflow_samples.message->pcs.pcs_val = tls->overflow_samples.pcs;
        
        tls->overflow_samples.message->counts.counts_len = 0;
        tls->overflow_samples.message->counts.counts_val = 
            tls->overflow_samples.counts;
        
        memset(tls->overflow_samples.hash_table, 0,
               sizeof(tls->overflow_samples.hash_table));
    }

    CBTF_cuda_message* raw_message = 
        &(tls->messages[tls->data.messages.messages_len++]);
    raw_message->type = PeriodicSamples;
    
    tls->periodic_samples.message =
        &raw_message->CBTF_cuda_message_u.periodic_samples;
    
    tls->periodic_samples.message->deltas.deltas_len = 0;
    tls->periodic_samples.message->deltas.deltas_val = 
        tls->periodic_samples.deltas;
    
    memset(&tls->periodic_samples.previous, 0, sizeof(EventSample));
#endif
}



/**
 * Send the performance data blob contained within the given thread-local
 * storage. The blob is re-initialized (cleared) after being sent. Nothing
 * is sent if the blob is empty.
 *
 * @param tls    Thread-local storage containing data to be sent.
 */
void TLS_send_data(TLS* tls)
{
    Assert(tls != NULL);

    if (tls->data.messages.messages_len > 0)
    {
#if !defined(NDEBUG)
        if (IsDebugEnabled)
        {
            printf("[CBTF/CUDA] TLS_send_data(): "
                   "sending CBTF_cuda_data message (%u msg, %u pc)\n",
                   tls->data.messages.messages_len,
                   tls->data.stack_traces.stack_traces_len);
        }
#endif

        cbtf_collector_send(
            &tls->data_header, (xdrproc_t)xdr_CBTF_cuda_data, &tls->data
            );
        TLS_initialize_data(tls);
    }
}



/**
 * Add a new message to the performance data blob contained within the given
 * thread-local storage. The current blob is sent and re-initialized (cleared)
 * if it is already full.
 *
 * @param tls    Thread-local storage to which a message is to be added.
 * @return       Pointer to the new message to be filled in by the caller.
 */
CBTF_cuda_message* TLS_add_message(TLS* tls)
{
    Assert(tls != NULL);

    if (tls->data.messages.messages_len == MAX_MESSAGES_PER_BLOB)
    {
        TLS_send_data(tls);
    }
    
    return &(tls->messages[tls->data.messages.messages_len++]);
}



/**
 * Update the performance data header contained within the given thread-local
 * storage with the specified time. Insures that the time interval defined by
 * time_begin and time_end contain the specified time.
 *
 * @param tls     Thread-local storage to be updated.
 * @param time    Time with which to update.
 */
void TLS_update_header_with_time(TLS* tls, CBTF_Protocol_Time time)
{
    Assert(tls != NULL);

    if (time < tls->data_header.time_begin)
    {
        tls->data_header.time_begin = time;
    }
    if (time >= tls->data_header.time_end)
    {
        tls->data_header.time_end = time + 1;
    }
}



/**
 * Update the performance data header contained within the given thread-local
 * storage with the specified address. Insures that the address range defined
 * by addr_begin and addr_end contain the specified address.
 *
 * @param tls     Thread-local storage to be updated.
 * @param addr    Address with which to update.
 */
void TLS_update_header_with_address(TLS* tls, CBTF_Protocol_Address addr)
{
    Assert(tls != NULL);

    if (addr < tls->data_header.addr_begin)
    {
        tls->data_header.addr_begin = addr;
    }
    if (addr >= tls->data_header.addr_end)
    {
        tls->data_header.addr_end = addr + 1;
    }
}



/**
 * Add a new stack trace for the current call site to the performance data
 * blob contained within the given thread-local storage.
 *
 * @param tls    Thread-local storage to which the stack trace is to be added.
 * @return       Index of this call site within the performance data blob.
 */
uint32_t TLS_add_current_call_site(TLS* tls)
{
    Assert(tls != NULL);

    /* Get the stack trace for the current call site */

    int frame_count = 0;
    uint64_t frame_buffer[CBTF_ST_MAXFRAMES];
    
    CBTF_GetStackTraceFromContext(
        NULL, FALSE, 0, CBTF_ST_MAXFRAMES, &frame_count, frame_buffer
        );

    /* Search for this stack trace amongst the existing stack traces */
    
    int i, j;
    
    /* Iterate over the addresses in the existing stack traces */
    for (i = 0, j = 0; i < MAX_ADDRESSES_PER_BLOB; ++i)
    {
        /* Is this the terminating null of an existing stack trace? */
        if (tls->stack_traces[i] == 0)
        {
            /*
             * Terminate the search if a complete match has been found between
             * this stack trace and the existing stack trace.
             */
            if (j == frame_count)
            {
                break;
            }

            /*
             * Otherwise check for a null in the first or last entry, or
             * for consecutive nulls, all of which indicate the end of the
             * existing stack traces, and the need to add this stack trace
             * to the existing stack traces.
             */
            else if ((i == 0) || 
                     (i == (MAX_ADDRESSES_PER_BLOB - 1)) ||
                     (tls->stack_traces[i - 1] == 0))
            {
                /*
                 * Send performance data for this thread if there isn't enough
                 * room in the existing stack traces to add this stack trace.
                 * Doing so frees up enough space for this stack trace.
                 */
                if ((i + frame_count) >= MAX_ADDRESSES_PER_BLOB)
                {
                    TLS_send_data(tls);
                    i = 0;
                }

                /* Add this stack trace to the existing stack traces */
                for (j = 0; j < frame_count; ++j, ++i)
                {
                    tls->stack_traces[i] = frame_buffer[j];
                    TLS_update_header_with_address(tls, tls->stack_traces[i]);
                }
                tls->stack_traces[i] = 0;
                tls->data.stack_traces.stack_traces_len = i + 1;
              
                break;
            }
            
            /* Otherwise reset the pointer within this stack trace to zero. */
            else
            {
                j = 0;
            }
        }
        else
        {
            /*
             * Advance the pointer within this stack trace if the current
             * address within this stack trace matches the current address
             * within the existing stack traces. Otherwise reset the pointer
             * to zero.
             */
            j = (frame_buffer[j] == tls->stack_traces[i]) ? (j + 1) : 0;
        }
    }

    /* Return the index of this stack trace within the existing stack traces */
    return i - frame_count;
}
