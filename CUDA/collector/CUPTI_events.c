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



/**
 * Table used to map CUDA context pointers to CUPTI event groups. This table
 * must be protected with a mutex because there is no guarantee that a CUDA
 * context won't be created or destroyed at the same time a sample is taken.
 */
static struct {
    struct {

        /** CUDA context pointer. */
        CUcontext context;
        
        /** Handle for this context's event group. */
        CUpti_EventGroup event_group;

        /** Number of events in this event group. */
        int event_count;

        /** Identifiers of the events in this event group. */
        CUpti_EventID event_ids[MAX_EVENTS];

        /**
         * Map event indicies in this event group to periodic count indicies.
         */
        int event_to_periodic[MAX_EVENTS];

    } values[MAX_CONTEXTS];
    pthread_mutex_t mutex;
} EventGroups = { { 0 }, PTHREAD_MUTEX_INITIALIZER };



/**
 * Start events data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be started.
 */
void CUPTI_events_start(CUcontext context)
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] CUPTI_events_start(%p)\n", context);
    }
#endif

    PTHREAD_CHECK(pthread_mutex_lock(&EventGroups.mutex));

    /* Find an empty entry in the table for this context */

    int i;
    for (i = 0;
         (i < MAX_CONTEXTS) && (EventGroups.values[i].context != NULL);
         ++i)
    {
        if (EventGroups.values[i].context == context)
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

    EventGroups.values[i].context = context;

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
    CUdevice device;
    CUDA_CHECK(cuCtxGetDevice(&device));
    
    /* Enable continuous event sampling for this context */
    CUPTI_CHECK(cuptiSetEventCollectionMode(
                    context, CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS
                    ));

    /* Create a new CUPTI event group for this context */
    CUPTI_CHECK(cuptiEventGroupCreate(
                    context, &EventGroups.values[i].event_group, 0
                    ));

    /* Iterate over each event in our event sampling configuration */
    uint e;
    for (e = 0; e < TheSamplingConfig.events.events_len; ++e)
    {
        CUDA_EventDescription* event = &TheSamplingConfig.events.events_val[e];
        
        /*
         * Look up the event id for this event. Note that an unidentified
         * event is NOT treated as fatal since it may simply be an event
         * that will be handled by PAPI. Just continue to the next event.
         */

        CUpti_EventID* id = 
            &EventGroups.values[i].event_ids[EventGroups.values[i].event_count];
        
        if (cuptiEventGetIdFromName(device, event->name, id) != CUPTI_SUCCESS)
        {
            continue;
        }

        /* Add this event to the event group */

        CUPTI_CHECK(cuptiEventGroupAddEvent(
                        EventGroups.values[i].event_group, *id
                        ));
        
        EventGroups.values[i].event_to_periodic[
            EventGroups.values[i].event_count
            ] = e;
        
        /* Increment the number of events in this event group */
        ++EventGroups.values[i].event_count;

#if !defined(NDEBUG)
        if (IsDebugEnabled)
        {
            printf("[CBTF/CUDA] recording GPU event \"%s\" for context %p\n",
                   event->name, context);
        }
#endif
    }

    /* Are there any events to collect? */
    if (EventGroups.values[i].event_count > 0)
    {
        /* Enable collection of this event group */
        CUPTI_CHECK(cuptiEventGroupEnable(EventGroups.values[i].event_group));
    }

    /* Restore (if necessary) the previous value of the current context */
    if (current != context)
    {
        CUDA_CHECK(cuCtxPopCurrent(&context));
        CUDA_CHECK(cuCtxPushCurrent(current));
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&EventGroups.mutex));
}



/**
 * Sample the CUPTI events for all active CUDA contexts.
 *
 * @param tls       Thread-local storage of the current thread.
 * @param sample    Periodic sample to hold the CUPTI event counts.
 */
void CUPTI_events_sample(TLS* tls, PeriodicSample* sample)
{
    PTHREAD_CHECK(pthread_mutex_lock(&EventGroups.mutex));

    /* Iterate over each context in the table. */
    int i;
    for (i = 0;
         (i < MAX_CONTEXTS) && (EventGroups.values[i].context != NULL);
         ++i)
    {
        /* Are any events being collected? */
        if (EventGroups.values[i].event_count > 0)
        {
            /* Read the counters for this context's event group */

            uint64_t counts[MAX_EVENTS];
            size_t counts_size = sizeof(counts);
            CUpti_EventID ids[MAX_EVENTS];
            size_t ids_size = sizeof(ids);
            size_t event_count = 0;

            CUPTI_CHECK(cuptiEventGroupReadAllEvents(
                            EventGroups.values[i].event_group,
                            CUPTI_EVENT_READ_FLAG_NONE,
                            &counts_size, counts,
                            &ids_size, ids,
                            &event_count
                            ));
            
            Assert(event_count == EventGroups.values[i].event_count);

            /*
             * Insert the counter values into the sample. CUPTI doesn't
             * guarantee that the events will be read in the same order
             * they were added to the event group. That would clearly be
             * too easy. So an additional search is necessary here...
             */

            size_t e;
            for (e = 0; e < event_count; ++e)
            {
                int j;
                for (j = 0; j < MAX_EVENTS; ++j)
                {
                    if (ids[e] == EventGroups.values[i].event_ids[j])
                    {
                        sample->count[
                            EventGroups.values[i].event_to_periodic[j]
                            ] += counts[e];
                        
                        break;
                    }
                }
            }
        }
    }

    PTHREAD_CHECK(pthread_mutex_unlock(&EventGroups.mutex));
}



/**
 * Stop CUPTI events data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be stopped.
 */
void CUPTI_events_stop(CUcontext context)
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] CUPTI_events_stop(%p)\n", context);
    }
#endif

    PTHREAD_CHECK(pthread_mutex_lock(&EventGroups.mutex));

    /* Find the specified context in the table */

    int i;
    for (i = 0;
         (i < MAX_CONTEXTS) && (EventGroups.values[i].context != NULL);
         ++i)
    {
        if (EventGroups.values[i].context == context)
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
    if (EventGroups.values[i].event_count > 0)
    {
        /* Disable collection of this event group */
        CUPTI_CHECK(cuptiEventGroupDisable(EventGroups.values[i].event_group));
    }

    /* Destroy this event group */
    CUPTI_CHECK(cuptiEventGroupDestroy(EventGroups.values[i].event_group));
    EventGroups.values[i].event_count = 0;
    
    PTHREAD_CHECK(pthread_mutex_unlock(&EventGroups.mutex));
}
