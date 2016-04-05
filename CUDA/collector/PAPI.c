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

/** @file Definition of PAPI functions. */

#include <inttypes.h>
#if defined(PAPI_FOUND)
#include <papi.h>
#endif
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <KrellInstitute/Messages/CUDA_data.h>

#include <KrellInstitute/Services/Assert.h>
#include <KrellInstitute/Services/Time.h>
#include <KrellInstitute/Services/Timer.h>

#include "collector.h"
#include "PAPI.h"
#include "Pthread_check.h"
#include "TLS.h"



#if defined(PAPI_FOUND)
/**
 * Checks that the given PAPI function call returns the value "PAPI_OK" or
 * "PAPI_VER_CURRENT". If the call was unsuccessful, the returned error is
 * reported on the standard error stream and the application is aborted.
 *
 * @param x    PAPI function call to be checked.
 */
#define PAPI_CHECK(x)                                               \
    do {                                                            \
        int RETVAL = x;                                             \
        if ((RETVAL != PAPI_OK) && (RETVAL != PAPI_VER_CURRENT))    \
        {                                                           \
            const char* description = PAPI_strerror(RETVAL);        \
            if (description != NULL)                                \
            {                                                       \
                fprintf(stderr, "[CBTF/CUDA] %s(): %s = %d (%s)\n", \
                        __func__, #x, RETVAL, description);         \
            }                                                       \
            else                                                    \
            {                                                       \
                fprintf(stderr, "[CBTF/CUDA] %s(): %s = %d\n",      \
                        __func__, #x, RETVAL);                      \
            }                                                       \
            fflush(stderr);                                         \
            abort();                                                \
        }                                                           \
    } while (0)
#endif



#if defined(PAPI_FOUND)
/**
 * The number of active CUDA contexts in this process. This value is atomically
 * incremented and decremented in PAPI_notify_cuda_context_[created|destroyed]()
 * and is used within them to determine when to perform PAPI initialization and
 * and finalization.
 *
 * Initializing the PAPI CUDA component before the application has itself called
 * cuInit() causes the process to hang on some systems. Thus PAPI initialization
 * must be deferred until after the application calls cuInit(), and must also be
 * tracked separately from the regular process-wide initialization controlled by
 * ThreadCount in "collector.c".
 */
static struct {
    int value;
    pthread_mutex_t mutex;
} ContextCount = { 0, PTHREAD_MUTEX_INITIALIZER };
#endif



#if defined(PAPI_FOUND)
/**
 * Callback invoked by PAPI every time an event counter overflows. I.e. reaches
 * the threshold value previously specified in parse_configuration().
 *
 * @param eventset           Eventset of the overflowing event(s).
 * @param address            Program counter address when the overflow occurred.
 * @param overflow_vector    Vector indicating the particular event(s) that have
 *                           overflowed. Call PAPI_get_overflow_event_index() to
 *                           decompose this vector into the individual events.
 * @param context            Thread context when the overflow occurred.
 */
static void papi_callback(int eventset, void* address,
                          long long overflow_vector, void* context)
{
    Assert(OverflowSamplingCount > 0);

    /* Access our thread-local storage */
    TLS* tls = TLS_get();
    
    /* Do nothing if data collection is paused for this thread */
    if (tls->paused)
    {
        return;
    }

    /* Get the time at which the overflow occurred */
    CBTF_Protocol_Time time = CBTF_GetTime();

    /* Get the PC at which the overflow occurred */
    CBTF_Protocol_Address pc = (CBTF_Protocol_Address)address;
    
    /* Find this eventset in the TLS for this thread */
    int s;
    for (s = 0; s < tls->eventset_count; ++s)
    {
        if (tls->eventsets[s].eventset == eventset)
        {
            break;
        }
    }
    Assert(s < tls->eventset_count);
    
    /* Determine which events overflowed */
    int events[MAX_EVENTS];
    memset(events, 0, sizeof(events));
    int event_count = tls->eventsets[s].event_count;
    PAPI_CHECK(PAPI_get_overflow_event_index(eventset, overflow_vector, events,
                                             &event_count));
    
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
        uint32_t bucket = (pc >> 4) % OVERFLOW_HASH_TABLE_SIZE;
        while ((hash_table[bucket] > 0) && (pcs[hash_table[bucket] - 1] != pc))
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
            pcs[tls->overflow_samples.message.pcs.pcs_len] = pc;
            memset(&counts[tls->overflow_samples.message.counts.counts_len],
                   0, OverflowSamplingCount * sizeof(uint64_t));            
            hash_table[bucket] = ++tls->overflow_samples.message.pcs.pcs_len;
            tls->overflow_samples.message.counts.counts_len += 
                OverflowSamplingCount;
                                    
            /* Update the header with this sample's address */
            TLS_update_header_with_address(tls, pc);            
        }

        /*
         * Increment the counts for the events that actually overflowed for
         * this sample. The funky triple-nested array indexing selects each
         * event that overflowed, maps its PAPI event index to its index in
         * the counts array, and then adds that result to the base index of
         * this PC address within the counts array.
         */
        int e, base = (hash_table[bucket] - 1) * OverflowSamplingCount;
        for (e = 0; e < event_count; ++e)
        {
            counts[base + tls->eventsets[s].event_to_overflow[events[e]]]++;
        }

        /* This sample has now been successfully added */
        break;
    }

    /* Update the header (and overflow samples message) with this sample time */

    TLS_update_header_with_time(tls, time);

    if (time < tls->overflow_samples.message.time_begin)
    {
        tls->overflow_samples.message.time_begin = time;
    }
    if (time >= tls->overflow_samples.message.time_end)
    {
        tls->overflow_samples.message.time_end = time;
    }
}
#endif



#if defined(PAPI_FOUND)
/**
 * Callback invoked by the timer service each time a timer interrupt occurs.
 *
 * @param context    Thread context at this timer interrupt.
 */
static void timer_callback(const ucontext_t* context)
{
    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Do nothing if data collection is paused for this thread */
    if (tls->paused)
    {
        return;
    }

    /* Collect a new event sample */

    EventSample sample;
    memset(&sample, 0, sizeof(EventSample));
    sample.time = CBTF_GetTime();
    
    int s;
    for (s = 0; s < tls->eventset_count; ++s)
    {
        long long counts[MAX_EVENTS];
        PAPI_CHECK(PAPI_read(tls->eventsets[s].eventset, (long long*)&counts));

        int e;
        for (e = 0; e < tls->eventsets[s].event_count; ++e)
        {
            sample.count[tls->eventsets[s].event_to_periodic[e]] = counts[e];
        }
    }

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
    const uint64_t* current = &sample.time;

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
            current = &sample.time - 1;
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
    TLS_update_header_with_time(tls, sample.time);
    
    /* Replace the previous event sample with the new event sample */
    memcpy(&tls->periodic_samples.previous, &sample, sizeof(EventSample));
}
#endif



/**
 * Called by the CUPTI callback to notify when a CUDA context is created.
*/
void PAPI_notify_cuda_context_created()
{
#if defined(PAPI_FOUND)
    /* Atomically increment the active CUDA context count */

    PTHREAD_CHECK(pthread_mutex_lock(&ContextCount.mutex));
    
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] PAPI_notify_cuda_context_created(): "
               "ContextCount.value = %d --> %d\n",
               ContextCount.value, ContextCount.value + 1);
    }
#endif
    
    if (ContextCount.value == 0)
    {
        /* Initialize PAPI */
        PAPI_CHECK(PAPI_library_init(PAPI_VER_CURRENT));
        PAPI_CHECK(PAPI_thread_init((unsigned long (*)())(pthread_self)));
        PAPI_CHECK(PAPI_multiplex_init());
    }
    
    ContextCount.value++;
    
    PTHREAD_CHECK(pthread_mutex_unlock(&ContextCount.mutex));
#endif
}



/**
 * Called by the CUPTI callback to notify when a CUDA context is destroyed.
 */
void PAPI_notify_cuda_context_destroyed()
{
#if defined(PAPI_FOUND)
    /* Atomically decrement the active CUDA context count */
    
    PTHREAD_CHECK(pthread_mutex_lock(&ContextCount.mutex));
    
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] PAPI_notify_cuda_context_destroyed(): "
               "ContextCount.value = %d --> %d\n",
               ContextCount.value, ContextCount.value - 1);
    }
#endif
    
    ContextCount.value--;

    if (ContextCount.value == 0)
    {
        /* Shutdown PAPI */
        PAPI_shutdown();
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&ContextCount.mutex));
#endif
}



/**
 * Start PAPI data collection for the current thread.
 */
void PAPI_start_data_collection()
{
#if defined(PAPI_FOUND)
    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Was PAPI data collection already started for this thread? */
    if (tls->eventset_count > 0)
    {
        return;
    }

#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] PAPI_start_data_collection()\n");
    }
#endif

    /* Atomically test whether PAPI has been started */
    
    PTHREAD_CHECK(pthread_mutex_lock(&ContextCount.mutex));
    
    if (ContextCount.value > 0)
    {
        /* Append event sampling configuration to our performance data blob */
        if (tls->appended_sampling_config &&
            (TheSamplingConfig.events.events_len > 0))
        {
            CBTF_cuda_message* raw_message = TLS_add_message(tls);
            Assert(raw_message != NULL);
            raw_message->type = SamplingConfig;
            
            CUDA_SamplingConfig* message = 
                &raw_message->CBTF_cuda_message_u.sampling_config;
            
            memcpy(message, &TheSamplingConfig, sizeof(CUDA_SamplingConfig));

            tls->appended_sampling_config = TRUE;
        }

        /* Iterate over each event in our event sampling configuration */
        uint i, overflow_count = 0, periodic_count = 0;
        for (i = 0; i < TheSamplingConfig.events.events_len; ++i)
        {
            CUDA_EventDescription* event = 
                &TheSamplingConfig.events.events_val[i];

            /* Look up the event code for this event */
            int code = PAPI_NULL;
            PAPI_CHECK(PAPI_event_name_to_code(event->name, &code));
            
            /* Look up the component for this event code */
            int component = PAPI_get_event_component(code);
            
            /* Search for an eventset corresponding to this component */
            int s;
            for (s = 0; s < tls->eventset_count; ++s)
            {
                if (tls->eventsets[s].component == component)
                {
                    break;
                }
            }

            /* Create a new eventset if one didn't already exist */
            if (s == tls->eventset_count)
            {
                tls->eventsets[s].component = component;
                tls->eventsets[s].eventset = PAPI_NULL;
                PAPI_CHECK(PAPI_create_eventset(&tls->eventsets[s].eventset));
                tls->eventsets[s].event_count = 0;
                tls->eventset_count++;
            }
            
            /* Add this event to the eventset */
            
            PAPI_CHECK(PAPI_add_event(tls->eventsets[s].eventset, code));
            
            tls->eventsets[s].event_to_periodic[
                tls->eventsets[s].event_count
                ] = periodic_count++;
            
            /* Setup overflow for this event if a threshold was specified */
            if (event->threshold > 0)
            {
                tls->eventsets[s].event_to_overflow[
                    tls->eventsets[s].event_count
                    ] = overflow_count++;
                
                PAPI_CHECK(PAPI_overflow(tls->eventsets[s].eventset,
                                         code,
                                         event->threshold,
                                         PAPI_OVERFLOW_FORCE_SW,
                                         papi_callback));
            }
            
            /* Increment the number of events in this eventset */
            ++tls->eventsets[s].event_count;
        }

        /* Start the sampling of our eventsets */
        int s;
        for (s = 0; s < tls->eventset_count; ++s)
        {
            PAPI_CHECK(PAPI_start(tls->eventsets[s].eventset));
        }
        
        CBTF_Timer(TheSamplingConfig.interval, timer_callback);
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&ContextCount.mutex));
#endif
}



/**
 * Stop PAPI data collection for the current thread.
 */
void PAPI_stop_data_collection()
{
#if defined(PAPI_FOUND)
    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Was PAPI data collection not started for this thread? */
    if (tls->eventset_count == 0)
    {
        return;
    }

#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] PAPI_stop_data_collection()\n");
    }
#endif

    /* Atomically test whether PAPI has been started */
    
    PTHREAD_CHECK(pthread_mutex_lock(&ContextCount.mutex));
    
    if (ContextCount.value > 0)
    {
        /* Stop the sampling of our eventsets */
        CBTF_Timer(0, NULL);

        int s;
        for (s = 0; s < tls->eventset_count; ++s)
        {
            PAPI_CHECK(PAPI_stop(tls->eventsets[s].eventset, NULL));
        }

        /* Cleanup and destroy our eventsets */
        for (s = 0; s < tls->eventset_count; ++s)
        {
            PAPI_CHECK(PAPI_cleanup_eventset(tls->eventsets[s].eventset));
            PAPI_CHECK(PAPI_destroy_eventset(&tls->eventsets[s].eventset));
        }
        tls->eventset_count = 0;
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&ContextCount.mutex));
#endif
}
