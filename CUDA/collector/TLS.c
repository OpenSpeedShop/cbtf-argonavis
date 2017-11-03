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
#include <monitor.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <KrellInstitute/Services/Assert.h>
#include <KrellInstitute/Services/Collector.h>
#include <KrellInstitute/Services/Data.h>
#include <KrellInstitute/Services/TLS.h>
#include <KrellInstitute/Services/Unwind.h>

#include "collector.h"
#include "TLS.h"



/* Declare externals coming from libmonitor to avoid locating its header */
extern int monitor_mpi_comm_rank();
extern int monitor_get_thread_num();



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
 * Is the performance data blob in the given thread-local storage already
 * full? I.e. does it already contain the maximum number of messages?
 *
 * @param tls    Thread-local storage to be tested.
 * @return       Boolean flag indicating if the performance data blob is full.
 */
static bool is_full(TLS* tls)
{
    Assert(tls != NULL);

    u_int max_messages_per_blob = MAX_MESSAGES_PER_BLOB;

    if (tls->overflow_samples.message.pcs.pcs_len > 0)
    {
        --max_messages_per_blob;
    }

    if (tls->periodic_samples.message.deltas.deltas_len > 0)
    {
        --max_messages_per_blob;
    }

    return tls->data.messages.messages_len == max_messages_per_blob;
}



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
    CBTF_SetTLS(Key, tls);
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
    CBTF_SetTLS(Key, NULL);
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

    tls->overflow_samples.message.time_begin = ~0;
    tls->overflow_samples.message.time_end = 0;
    
    tls->overflow_samples.message.pcs.pcs_len = 0;
    tls->overflow_samples.message.pcs.pcs_val = tls->overflow_samples.pcs;
        
    tls->overflow_samples.message.counts.counts_len = 0;
    tls->overflow_samples.message.counts.counts_val = 
        tls->overflow_samples.counts;
        
    memset(tls->overflow_samples.hash_table, 0,
           sizeof(tls->overflow_samples.hash_table));

    tls->periodic_samples.message.deltas.deltas_len = 0;
    tls->periodic_samples.message.deltas.deltas_val = 
        tls->periodic_samples.deltas;
    
    memset(&tls->periodic_samples.previous, 0, sizeof(PeriodicSample));
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

    bool send = (tls->data.messages.messages_len > 0);

    if (tls->overflow_samples.message.pcs.pcs_len > 0)
    {
        CBTF_cuda_message* raw_message =
            &(tls->messages[tls->data.messages.messages_len++]);
        raw_message->type = OverflowSamples;
        
        CUDA_OverflowSamples* message =
            &raw_message->CBTF_cuda_message_u.overflow_samples;

        memcpy(message, &tls->overflow_samples.message,
               sizeof(CUDA_OverflowSamples));

        send = TRUE;
    }

    if (tls->periodic_samples.message.deltas.deltas_len > 0)
    {
        CBTF_cuda_message* raw_message =
            &(tls->messages[tls->data.messages.messages_len++]);
        raw_message->type = PeriodicSamples;

        CUDA_PeriodicSamples* message =
            &raw_message->CBTF_cuda_message_u.periodic_samples;
        
        memcpy(message, &tls->periodic_samples.message,
               sizeof(CUDA_PeriodicSamples));

        send = TRUE;
    }
    
    if (send)
    {
#if !defined(NDEBUG)
        if (IsDebugEnabled)
        {
            printf("[CUDA %d:%d] TLS_send_data(): "
                   "sending CBTF_cuda_data message (%u msg, %u pc)\n",
                   getpid(), monitor_get_thread_num(),
                   tls->data.messages.messages_len,
                   tls->data.stack_traces.stack_traces_len);
        }
#endif

        /*
         * At the point when cbtf_collector_start() is called the process has
         * not had a chance to call MPI_Init() yet. Thus the process does not
         * yet have a MPI rank. But by the time a performance data blob is to
         * be sent, MPI_Init() almost certainly has been called, so obtain the
         * MPI and OpenMP ranks and put them in the performance data header.
         */

        tls->data_header.rank = monitor_mpi_comm_rank();

        if (tls->data_header.omp_tid != -1)
        {
            tls->data_header.omp_tid = monitor_get_thread_num();
        }
        
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

    if (is_full(tls))
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
 *
 * @note    Call sites are always referenced by a message. And since cross-blob
 *          references aren't supported, a crash is all but certain if the call
 *          site and its referencing message were to be split across two blobs.
 *          So this function also insures there is room in the current blob for
 *          at least one more message before adding the call site.
 */
uint32_t TLS_add_current_call_site(TLS* tls)
{
    Assert(tls != NULL);

    /*
     * Send performance data for this thread if there isn't enough room
     * to hold another message. See the note in this function's header.
     */

    if (is_full(tls))
    {
        TLS_send_data(tls);
    }

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
             * Terminate the search if a complete match has been found
             * between this stack trace and the existing stack trace.
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
            
            /* Otherwise reset the pointer within this stack trace to zero */
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



/**
 * Add the specified overflow sample to the performance data blob contained
 * within the given thread-local storage. The current blob is sent and re-
 * initialized (cleared) if it is already full.
 *
 * @param tls       Thread-local storage to which the sample is to be added.
 * @param sample    Overflow sample to be added.
 */
void TLS_add_overflow_sample(TLS* tls, OverflowSample* sample)
{
    Assert(tls != NULL);
    Assert(sample != NULL);

    /* Get a pointer to the overflow samples PCs for this thread */
    uint64_t* const pcs = tls->overflow_samples.pcs;

    /* Get a pointer to the overflow samples counts for this thread */
    uint64_t* const counts = tls->overflow_samples.counts;
    
    /* Get a pointer to the overflow samples hash table for this thread */
    uint32_t* const hash_table = tls->overflow_samples.hash_table;

    /* Iterate until this sample is successfully added */
    while (true)
    {
        /*
         * Search the existing overflow samples for this sample's PC address.
         * Accelerate the search using the hash table and a simple linear probe.
         */
        uint32_t bucket = (sample->pc >> 4) % OVERFLOW_HASH_TABLE_SIZE;
        while ((hash_table[bucket] > 0) && 
               (pcs[hash_table[bucket] - 1] != sample->pc))
        {
            bucket = (bucket + 1) % OVERFLOW_HASH_TABLE_SIZE;
        }
        
        /* Did the search fail? */
        if (hash_table[bucket] == 0)
        {
            /*
             * Send performance data for this thread if there isn't enough room
             * in the existing overflow samples to add this sample's PC address.
             * Doing so frees up enoguh space for this sample.
             */
            if (tls->overflow_samples.message.pcs.pcs_len == 
                MAX_OVERFLOW_PCS_PER_BLOB)
            {
                TLS_send_data(tls);
                continue;
            }
            
            /* Add an entry for this sample to the existing overflow samples */
            pcs[tls->overflow_samples.message.pcs.pcs_len] = sample->pc;
            memset(&counts[tls->overflow_samples.message.counts.counts_len],
                   0, TheSamplingConfig.events.events_len * sizeof(uint64_t));
            hash_table[bucket] = ++tls->overflow_samples.message.pcs.pcs_len;
            tls->overflow_samples.message.counts.counts_len += 
                TheSamplingConfig.events.events_len;
            
            /* Update the header with this sample's address */
            TLS_update_header_with_address(tls, sample->pc);
        }

        /* Increment counts for the events that actually overflowed */
        int base = 
            (hash_table[bucket] - 1) * TheSamplingConfig.events.events_len;
        int e;
        for (e = 0; e < TheSamplingConfig.events.events_len; ++e)
        {
            if (sample->overflowed[e])
            {
                counts[base + e]++;
            }
        }
        
        /* This sample has now been successfully added */
        break;
    }

    /* Update the header (and overflow samples message) with this sample time */

    TLS_update_header_with_time(tls, sample->time);

    if (sample->time < tls->overflow_samples.message.time_begin)
    {
        tls->overflow_samples.message.time_begin = sample->time;
    }

    if (sample->time >= tls->overflow_samples.message.time_end)
    {
        tls->overflow_samples.message.time_end = sample->time;
    }
}



/**
 * Add the specified periodic sample to the performance data blob contained
 * within the given thread-local storage. The current blob is sent and re-
 * initialized (cleared) if it is already full.
 *
 * @param tls       Thread-local storage to which the sample is to be added.
 * @param sample    Periodic sample to be added.
 */
void TLS_add_periodic_sample(TLS* tls, PeriodicSample* sample)
{
    Assert(tls != NULL);
    Assert(sample != NULL);

    /* Get a pointer to the periodic samples deltas for this thread */
    uint8_t* const deltas = tls->periodic_samples.deltas;
    
    /*
     * Get the current index within the periodic samples deltas. The length
     * isn't updated until the ENTIRE event sample encoding has been added.
     * This facilitates easy restarting of the encoding in the event there
     * isn't enough room left in the array for the entire encoding.
     */
    int index = tls->periodic_samples.message.deltas.deltas_len;
    
    /*
     * Get pointers to the values in the new (current) and previous event
     * samples. Note that the time and the actual event counts are treated
     * identically as 64-bit unsigned integers.
     */
    
    const uint64_t* previous = &tls->periodic_samples.previous.time;
    const uint64_t* current = &sample->time;

    /* Iterate over each time and event count value in this event sample */
    int i, iEnd = TheSamplingConfig.events.events_len + 1;
    for (i = 0; i < iEnd; ++i, ++previous, ++current)
    {
        /*
         * Compute the delta between the previous and current samples for this
         * value. The previous event sample is zeroed by TLS_initialize_data(),
         * so there is no need to treat the first sample specially here.
         */
        
        uint64_t delta = *current - *previous;

        /*
         * Select the appropriate top 2 bits of the first encoded byte (called
         * the "prefix" here) and number of bytes in the encoding based on the
         * actual numerical magnitude of the delta.
         */

        uint8_t prefix = 0;
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

        /*
         * Send performance data for this thread if there isn't enough room in
         * the existing periodic samples deltas array to add this delta. Doing
         * so frees up enough space for this delta. Restart this event sample's
         * encoding by reseting the loop variables, keeping in mind that loop
         * increment expressions are still applied after a continue statement.
         */

        if ((index + num_bytes) > MAX_DELTAS_BYTES_PER_BLOB)
        {
            TLS_send_data(tls);
            index = tls->periodic_samples.message.deltas.deltas_len;
            previous = &tls->periodic_samples.previous.time - 1;
            current = &sample->time - 1;
            i = -1;
            continue;
        }

        /*
         * Add the encoding of this delta to the periodic samples deltas one
         * byte at a time. The loop adds the full bytes from the delta, last
         * byte first, allowing the use of fixed-size shift. Finally, after
         * the loop, add the first byte, which includes the encoding prefix
         * and the first 6 bits of the delta.
         */

        int j;
        for (j = index + num_bytes - 1; j > index; --j, delta >>= 8)
        {
            deltas[j] = delta & 0xFF;
        }
        deltas[j] = prefix | (delta & 0x3F);
        
        /* Advance the current index within the periodic samples deltas */
        index += num_bytes;
    }

    /* Update the length of the periodic samples deltas array */
    tls->periodic_samples.message.deltas.deltas_len = index;

    /* Update the header with this sample time */
    TLS_update_header_with_time(tls, sample->time);
    
    /* Replace the previous event sample with the new event sample */
    memcpy(&tls->periodic_samples.previous, sample, sizeof(PeriodicSample));
}
