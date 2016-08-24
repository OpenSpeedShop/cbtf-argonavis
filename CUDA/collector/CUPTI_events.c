/*******************************************************************************
** Copyright (c) 2016 Argo Navis Technologies. All Rights Reserved.
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

/** @file Definition of CUPTI events functions. */

#include <cuda.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <KrellInstitute/Services/Assert.h>

#include "collector.h"
#include "CUDA_check.h"
#include "CUPTI_check.h"
#include "CUPTI_context.h"
#include "CUPTI_events.h"
#include "Pthread_check.h"

/** Macro definition enabling the use of the CUPTI event API. */
//#define ENABLE_CUPTI_EVENTS



/**
 * Table used to map CUDA context pointers to the information necessary to
 * collect CUPTI events. This table must be protected with a mutex because
 * there is no guarantee that a CUDA context won't be created or destroyed
 * at the same time a sample is taken.
 */
static struct {
    struct {

        /** CUDA context pointer. */
        CUcontext context;

        /** CUDA device for this context. */
        CUdevice device;

        /** Number of events. */
        int count;

        /** Identifiers for the events. */
        CUpti_EventID ids[MAX_EVENTS];

        /** Map event indicies to periodic count indicies. */
        int to_periodic[MAX_EVENTS];
        
        /** Handle to the CUPTI event group sets for this context. */
        CUpti_EventGroupSets* sets;

    } values[MAX_CONTEXTS];
    pthread_mutex_t mutex;
} Events = { { 0 }, PTHREAD_MUTEX_INITIALIZER };

/**
 * Current CUPTI event count origins.
 *
 * Each time a CUPTI event count is read via cuptiEventGroupReadAllEvents()
 * it is automatically reset. No choice. The event counts in PeroidicSample,
 * however, are expected to be monotonically increasing absolute counts. So
 * these counts are used to convert the event count deltas returnd by CUPTI
 * into absolute event counts.
 *
 * @note    It may seem obvious to use the TLS "periodic_samples.previous"
 *          field for this purpose. But that object is reset to all zeroes
 *          every time a performance data blob is emitted. Thus it doesn't
 *          serve sufficiently for our purpose here...
 *
 * @note    No locking is required here because this object is only accessed
 *          by CUPTI_events_sample(). And that function is only ever called
 *          from the main thread.
 */
PeriodicSample EventOrigins = { 0 };



/**
 * Start events data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be started.
 */
void CUPTI_events_start(CUcontext context)
{
#if !defined(ENABLE_CUPTI_EVENTS)
    return;
#endif

#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] CUPTI_events_start(%p)\n", context);
    }
#endif

    PTHREAD_CHECK(pthread_mutex_lock(&Events.mutex));

    /* Find an empty entry in the table for this context */

    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Events.values[i].context != NULL); ++i)
    {
        if (Events.values[i].context == context)
        {
            fprintf(stderr, "[CBTF/CUDA] CUPTI_events_start(): "
                    "Redundant call for CUPTI context pointer (%p)!", context);
            fflush(stderr);
            abort();
        }
    }
    
    if (i == MAX_CONTEXTS)
    {
        fprintf(stderr, "[CBTF/CUDA] CUPTI_events_start(): "
                "Maximum supported CUDA context pointers (%d) was reached!\n",
                MAX_CONTEXTS);
        fflush(stderr);
        abort();
    }

    Events.values[i].context = context;

    /*
     * Get the current context, saving it for possible later restoration,
     * and insure that the specified context is now the current context.
     */
    
    CUcontext current = NULL;
    CUDA_CHECK(cuCtxGetCurrent(&current));
    
    if (current != context)
    {
        CUDA_CHECK(cuCtxPopCurrent(&current));
        CUDA_CHECK(cuCtxPushCurrent(context));
    }

    /* Get the device for this context */
    CUDA_CHECK(cuCtxGetDevice(&Events.values[i].device));

    /* Iterate over each event in our event sampling configuration */
    uint e;
    for (e = 0; e < TheSamplingConfig.events.events_len; ++e)
    {
        CUDA_EventDescription* event = &TheSamplingConfig.events.events_val[e];
        
        /*
         * Look up the event id for this event. Note that an unidentified
         * event is NOT treated as fatal since it may simply be an event
         * that will be handled elsewhere. Just continue to the next event.
         */

        CUpti_EventID id;
        
        if (cuptiEventGetIdFromName(
                Events.values[i].device, event->name, &id
                ) != CUPTI_SUCCESS)
        {
            continue;
        }

        /* Add this event */
        Events.values[i].ids[Events.values[i].count] = id;
        Events.values[i].to_periodic[Events.values[i].count] = e;
        
        /* Increment the number of events */
        ++Events.values[i].count;

#if !defined(NDEBUG)
        if (IsDebugEnabled)
        {
            printf("[CBTF/CUDA] recording GPU event \"%s\" for context %p\n",
                   event->name, context);
        }
#endif
    }

    /* Are there any events to collect? */
    if (Events.values[i].count > 0)
    {
        /* Create the event groups to collect these events for this context */
        CUPTI_CHECK(cuptiEventGroupSetsCreate(
                        context,
                        Events.values[i].count * sizeof(CUpti_EventID),
                        Events.values[i].ids,
                        &Events.values[i].sets
                        ));
        
        /*
         * There is no way to support multi-pass events using the current
         * exection model. Warn the user and ignore all GPU events.
         */
        if (Events.values[i].sets->numSets > 1)
        {
            fprintf(stderr, "[CBTF/CUDA] CUPTI_events_start(): "
                    "The specified GPU events cannot be collected in a "
                    "single pass. Ignoring all GPU events.\n");
            fflush(stderr);
            
            /* Destroy all of the event group sets. */
            CUPTI_CHECK(cuptiEventGroupSetsDestroy(Events.values[i].sets));
            
            /* Insure CUPTI_events_[sample|stop] don't do anything */
            Events.values[i].count = 0;
        }
        else
        {
            /* Enable continuous event sampling for this context */
            CUPTI_CHECK(cuptiSetEventCollectionMode(
                            context, CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS
                            ));
            
            /* Enable collection of the event group set */
            CUPTI_CHECK(cuptiEventGroupSetEnable(
                            &Events.values[i].sets->sets[0]
                            ));
        }
    }
        
    /* Restore (if necessary) the previous value of the current context */
    if (current != context)
    {
        CUDA_CHECK(cuCtxPopCurrent(&context));
        CUDA_CHECK(cuCtxPushCurrent(current));
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&Events.mutex));
}



/**
 * Sample the CUPTI events for all active CUDA contexts.
 *
 * @param tls       Thread-local storage of the current thread.
 * @param sample    Periodic sample to hold the CUPTI event counts.
 */
void CUPTI_events_sample(TLS* tls, PeriodicSample* sample)
{
#if !defined(ENABLE_CUPTI_EVENTS)
    return;
#endif

    PTHREAD_CHECK(pthread_mutex_lock(&Events.mutex));

    /* Initialize a new periodic sample to hold the event deltas */
    PeriodicSample delta;
    memset(&delta, 0, sizeof(PeriodicSample));
    
    /* Iterate over each context in the table */
    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Events.values[i].context != NULL); ++i)
    {
        /* Skip this context if no events are being collected */
        if (Events.values[i].count == 0)
        {
            continue;
        }

        /* Read the counters for each event group for this context */

        uint64_t counts[MAX_EVENTS];
        CUpti_EventID ids[MAX_EVENTS];
        size_t counts_size, ids_size, event_count;

        size_t n = 0;
        
        int g;
        for (g = 0; g < Events.values[i].sets->sets[0].numEventGroups; ++g)
        {
            counts_size = (MAX_EVENTS - n) * sizeof(uint64_t);
            ids_size = (MAX_EVENTS - n) * sizeof(CUpti_EventID);
            event_count = 0;
            
            CUPTI_CHECK(cuptiEventGroupReadAllEvents(
                            Events.values[i].sets->sets[0].eventGroups[g],
                            CUPTI_EVENT_READ_FLAG_NONE,
                            &counts_size, &counts[n],
                            &ids_size, &ids[n],
                            &event_count
                            ));

            n += event_count;
        }

        /*
         * Insert the counter values into the sample. CUPTI doesn't
         * guarantee that the events will be read in the same order
         * they were added to the event group. That would clearly be
         * too easy. So an additional search is necessary here...
         */
        
        size_t e;
        for (e = 0; e < n; ++e)
        {
            int j;
            for (j = 0; j < MAX_EVENTS; ++j)
            {
                if (ids[e] == Events.values[i].ids[j])
                {
                    delta.count[Events.values[i].to_periodic[j]] += counts[e];
                    break;
                }
            }
        }
    }

    /*
     * Compute absolute events by adding the deltas to the event origins and
     * copy these absolute counts into the provided sample. But only do the
     * latter for non-zero absolute events to avoid overwriting samples from
     * the other data collection methods.
     */

    size_t e;
    for (e = 0; e < MAX_EVENTS; ++e)
    {
        EventOrigins.count[e] += delta.count[e];

        if (EventOrigins.count[e] > 0)
        {
            sample->count[e] = EventOrigins.count[e];
        }
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&Events.mutex));
}



/**
 * Stop CUPTI events data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be stopped.
 */
void CUPTI_events_stop(CUcontext context)
{
#if !defined(ENABLE_CUPTI_EVENTS)
    return;
#endif

#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] CUPTI_events_stop(%p)\n", context);
    }
#endif

    PTHREAD_CHECK(pthread_mutex_lock(&Events.mutex));

    /* Find the specified context in the table */

    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Events.values[i].context != NULL); ++i)
    {
        if (Events.values[i].context == context)
        {
            break;
        }
    }
    
    if (i == MAX_CONTEXTS)
    {
        fprintf(stderr, "[CBTF/CUDA] CUPTI_events_stop(): "
                "Unknown CUDA context pointer (%p) encountered!\n", context);
        fflush(stderr);
        abort();
    }

    /* Are any events being collected? */
    if (Events.values[i].count > 0)
    {
        /* Disable collection of the event group set */
        CUPTI_CHECK(cuptiEventGroupSetDisable(
                        &Events.values[i].sets->sets[0]
                        ));
        
        /* Destroy all of the event group sets */
        CUPTI_CHECK(cuptiEventGroupSetsDestroy(Events.values[i].sets));
        
        /* Insure CUPTI_events_sample doesn't do anything */
        Events.values[i].count = 0;
    }

    PTHREAD_CHECK(pthread_mutex_unlock(&Events.mutex));
}
