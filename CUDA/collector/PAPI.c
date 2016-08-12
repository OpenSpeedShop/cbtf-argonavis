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
 * Callback invoked by PAPI every time an event counter overflows. I.e. reaches
 * the threshold value previously specified in parse_configuration().
 *
 * @param event_set          Eventset of the overflowing event(s).
 * @param address            Program counter address when the overflow occurred.
 * @param overflow_vector    Vector indicating the particular event(s) that have
 *                           overflowed. Call PAPI_get_overflow_event_index() to
 *                           decompose this vector into the individual events.
 * @param context            Thread context when the overflow occurred.
 */
static void papi_callback(int event_set, void* address,
                          long long overflow_vector, void* context)
{
    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Do nothing if data collection is paused for this thread */
    if (tls->paused)
    {
        return;
    }

    /* Initialize a new overflow sample */
    OverflowSample sample;
    memset(&sample, 0, sizeof(OverflowSample));
    sample.time = CBTF_GetTime();
    sample.pc = (CBTF_Protocol_Address)address;
    
    /* Find this event set in the TLS for this thread */
    int s;
    for (s = 0; s < tls->papi_event_set_count; ++s)
    {
        if (tls->papi_event_sets[s].event_set == event_set)
        {
            break;
        }
    }
    Assert(s < tls->papi_event_set_count);
    
    /* Determine which events overflowed */
    int events[MAX_EVENTS];
    memset(events, 0, sizeof(events));
    int event_count = tls->papi_event_sets[s].event_count;
    PAPI_CHECK(PAPI_get_overflow_event_index(event_set, overflow_vector,
                                             events, &event_count));
    
    /*
     * Mark (in this sample) the events that actually overflowed. The funky
     * triple-nested array indexing selects each event that overflowed, and
     * maps its PAPI event index to its index in the overflow counts array.
     */
    int e;
    for (e = 0; e < event_count; ++e)
    {
        sample.overflowed[
            tls->papi_event_sets[s].event_to_overflow[events[e]]
            ] = TRUE;
    }
    
    /* Add this sample to the performance data blob for this thread */
    TLS_add_overflow_sample(tls, &sample);
}
#endif



/**
 * Initialize PAPI for this process.
 */
void PAPI_initialize()
{
#if defined(PAPI_FOUND)

#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] PAPI_initialize()\n");
    }
#endif

    PAPI_CHECK(PAPI_library_init(PAPI_VER_CURRENT));
    PAPI_CHECK(PAPI_thread_init((unsigned long (*)())(pthread_self)));
    PAPI_CHECK(PAPI_multiplex_init());
#endif
}



/**
 * Start PAPI data collection for the current thread.
 */
void PAPI_start_data_collection()
{
#if defined(PAPI_FOUND)

#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] PAPI_start_data_collection()\n");
    }
#endif

    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Iterate over each event in our event sampling configuration */
    uint e;
    for (e = 0; e < TheSamplingConfig.events.events_len; ++e)
    {
        CUDA_EventDescription* event = &TheSamplingConfig.events.events_val[e];
        
        /*
         * Look up the event code for this event. Note that an unidentified
         * event is NOT treated as fatal since it may simply be an event
         * that will be handled by CUPTI. Just continue to the next event.
         */
        int code = PAPI_NULL;
        if (PAPI_event_name_to_code(event->name, &code) != PAPI_OK)
        {
            continue;
        }
        
        /* Look up the component for this event code */
        int component = PAPI_get_event_component(code);

        /* Search for an event set corresponding to this component */
        int s;
        for (s = 0; s < tls->papi_event_set_count; ++s)
        {
            if (tls->papi_event_sets[s].component == component)
            {
                break;
            }
        }
        
        /* Create a new event set if one didn't already exist */
        if (s == tls->papi_event_set_count)
        {
            tls->papi_event_sets[s].component = component;
            tls->papi_event_sets[s].event_set = PAPI_NULL;
            PAPI_CHECK(PAPI_create_eventset(
                           &tls->papi_event_sets[s].event_set
                           ));
            tls->papi_event_sets[s].event_count = 0;
            tls->papi_event_set_count++;
        }
        
        /* Add this event to the event_set */
        
        PAPI_CHECK(PAPI_add_event(tls->papi_event_sets[s].event_set, code));
        
        tls->papi_event_sets[s].event_to_periodic[
            tls->papi_event_sets[s].event_count
            ] = e;
        
        /* Setup overflow for this event if a threshold was specified */
        if (event->threshold > 0)
        {
            tls->papi_event_sets[s].event_to_overflow[
                tls->papi_event_sets[s].event_count
                ] = e;
            
            PAPI_CHECK(PAPI_overflow(tls->papi_event_sets[s].event_set,
                                     code,
                                     event->threshold,
                                     PAPI_OVERFLOW_FORCE_SW,
                                     papi_callback));
        }
        
        /* Increment the number of events in this event set */
        ++tls->papi_event_sets[s].event_count;
    }
    
    /* Start the sampling of our event sets */
    int s;
    for (s = 0; s < tls->papi_event_set_count; ++s)
    {
        PAPI_CHECK(PAPI_start(tls->papi_event_sets[s].event_set));
    }
#endif
}



/**
 * Sample the PAPI counters for the current thread.
 */
void PAPI_sample()
{
#if defined(PAPI_FOUND)
    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Do nothing if data collection is paused for this thread */
    if (tls->paused)
    {
        return;
    }

    /* Initialize a new periodic sample */
    PeriodicSample sample;
    memset(&sample, 0, sizeof(PeriodicSample));
    sample.time = CBTF_GetTime();
        
    /* Read the counters for each of our event sets */
    int s;
    for (s = 0; s < tls->papi_event_set_count; ++s)
    {
        long long counts[MAX_EVENTS];
        PAPI_CHECK(PAPI_read(tls->papi_event_sets[s].event_set,
                             (long long*)&counts));
        
        int e;
        for (e = 0; e < tls->papi_event_sets[s].event_count; ++e)
        {
            sample.count[
                tls->papi_event_sets[s].event_to_periodic[e]
                ] = counts[e];
        }
    }

    /* Add this sample to the performance data blob for this thread */
    TLS_add_periodic_sample(tls, &sample);
#endif
}



/**
 * Stop PAPI data collection for the current thread.
 */
void PAPI_stop_data_collection()
{
#if defined(PAPI_FOUND)

#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] PAPI_stop_data_collection()\n");
    }
#endif

    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Stop the sampling of our event sets */
    int s;
    for (s = 0; s < tls->papi_event_set_count; ++s)
    {
        PAPI_CHECK(PAPI_stop(tls->papi_event_sets[s].event_set, NULL));
    }
    
    /* Cleanup and destroy our event sets */
    for (s = 0; s < tls->papi_event_set_count; ++s)
    {
        PAPI_CHECK(PAPI_cleanup_eventset(tls->papi_event_sets[s].event_set));
        PAPI_CHECK(PAPI_destroy_eventset(&tls->papi_event_sets[s].event_set));
    }
    tls->papi_event_set_count = 0;
#endif
}



/**
 * Finalize PAPI for this process.
 */
void PAPI_finalize()
{
#if defined(PAPI_FOUND)

#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] PAPI_finalize()\n");
    }
#endif

    PAPI_shutdown();
#endif
}
