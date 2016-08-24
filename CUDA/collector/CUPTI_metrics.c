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

/** @file Definition of CUPTI metrics functions. */

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
#include "CUPTI_metrics.h"
#include "Pthread_check.h"

/** Macro definition enabling the use of the CUPTI metric API. */
//#define ENABLE_CUPTI_METRICS



/**
 * Table used to map CUDA context pointers to the information necessary to
 * collect CUPTI metrics. This table must be protected with a mutex because
 * there is no guarantee that a CUDA context won't be created or destroyed
 * at the same time a sample is taken.
 */
static struct {
    struct {

        /** CUDA context pointer. */
        CUcontext context;

        /** CUDA device for this context. */
        CUdevice device;

        /** Number of metrics. */
        int count;
        
        /** Identifiers for the metrics. */
        CUpti_MetricID ids[MAX_EVENTS];

        /** Map metric indicies to periodic count indicies. */
        int to_periodic[MAX_EVENTS];

        /** Handle to the CUPTI event group sets for this context. */
        CUpti_EventGroupSets* sets;

    } values[MAX_CONTEXTS];
    pthread_mutex_t mutex;
} Metrics = { { 0 }, PTHREAD_MUTEX_INITIALIZER };

/**
 * Current CUPTI metric origins.
 *
 * Each time a CUPTI event is read via cuptiEventGroupReadAllEvents() it
 * is automatically reset. No choice. The event counts in PeriodicSample,
 * however, are expected to be monotonically increasing absolute counts.
 * So these origins are used to convert the event deltas returned by CUPTI
 * into absolute counts.
 *
 * @note    It may seem obvious to use the TLS "periodic_samples.previous"
 *          field for this purpose. But that object is reset to all zeroes
 *          every time a performance data blob is emitted. Thus it doesn't
 *          serve sufficiently for our purpose here...
 *
 * @note    No locking is required here because this object is only accessed
 *          by CUPTI_metrics_sample(). And that function is only ever called
 *          from the main thread.
 */
PeriodicSample MetricOrigins = { 0 };



/**
 * Start metrics data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be started.
 */
void CUPTI_metrics_start(CUcontext context)
{
#if !defined(ENBALE_CUPTI_METRICS)
    return;
#endif

#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] CUPTI_metrics_start(%p)\n", context);
    }
#endif

    PTHREAD_CHECK(pthread_mutex_lock(&Metrics.mutex));

    /* Find an empty entry in the table for this context */

    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Metrics.values[i].context != NULL); ++i)
    {
        if (Metrics.values[i].context == context)
        {
            fprintf(stderr, "[CBTF/CUDA] CUPTI_metrics_start(): "
                    "Redundant call for CUPTI context pointer (%p)!", context);
            fflush(stderr);
            abort();
        }
    }
    
    if (i == MAX_CONTEXTS)
    {
        fprintf(stderr, "[CBTF/CUDA] CUPTI_metrics_start(): "
                "Maximum supported CUDA context pointers (%d) was reached!\n",
                MAX_CONTEXTS);
        fflush(stderr);
        abort();
    }

    Metrics.values[i].context = context;

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
    CUDA_CHECK(cuCtxGetDevice(&Metrics.values[i].device));
    
    /* Iterate over each event in our event sampling configuration */
    uint e;
    for (e = 0; e < TheSamplingConfig.events.events_len; ++e)
    {
        CUDA_EventDescription* event = &TheSamplingConfig.events.events_val[e];
        
        /*
         * Look up the metric id for this event. Note that an unidentified
         * event is NOT treated as fatal since it may simply be an event
         * that will be handled elsewhere. Just continue to the next event.
         */
        
        CUpti_MetricID id;
        
        if (cuptiMetricGetIdFromName(
                Metrics.values[i].device, event->name, &id
                ) != CUPTI_SUCCESS)
        {
            continue;
        }
        
        /*
         * Only support uint64_t metrics for now. If a non-uint64_t metric
         * is specified, warn the user and continue to the next event.
         */

        CUpti_MetricValueKind kind;
        size_t size = sizeof(kind);

        CUPTI_CHECK(cuptiMetricGetAttribute(
                        id, CUPTI_METRIC_ATTR_VALUE_KIND, &size, &kind
                        ));

        if (kind != CUPTI_METRIC_VALUE_KIND_UINT64)
        {
            fprintf(stderr, "[CBTF/CUDA] CUPTI_metrics_start(): "
                    "Valid GPU event \"%s\" is of an unsupported value kind "
                    "(%d). Ignoring this event.\n", event->name, kind);
            fflush(stderr);
            
            continue;
        }

        /* Add this metric */
        Metrics.values[i].ids[Metrics.values[i].count] = id;
        Metrics.values[i].to_periodic[Metrics.values[i].count] = e;

        /* Increment the number of metrics */
        ++Metrics.values[i].count;

#if !defined(NDEBUG)
        if (IsDebugEnabled)
        {
            printf("[CBTF/CUDA] recording GPU metric \"%s\" for context %p\n",
                   event->name, context);
        }
#endif
    }

    /* Are there any metrics to collect? */
    if (Metrics.values[i].count > 0)
    {
        /* Create the event groups to collect these metrics for this context */
        CUPTI_CHECK(cuptiMetricCreateEventGroupSets(
                        context,
                        Metrics.values[i].count * sizeof(CUpti_MetricID),
                        Metrics.values[i].ids,
                        &Metrics.values[i].sets
                        ));

        /*
         * There is no way to support multi-pass metrics using the current
         * exection model. Warn the user and ignore all GPU events.
         */
        if (Metrics.values[i].sets->numSets > 1)
        {
            fprintf(stderr, "[CBTF/CUDA] CUPTI_metrics_start(): "
                    "The specified GPU events cannot be collected in a "
                    "single pass. Ignoring all GPU events.\n");
            fflush(stderr);
            
            /* Destroy all of the event group sets. */
            CUPTI_CHECK(cuptiEventGroupSetsDestroy(Metrics.values[i].sets));
            
            /* Insure CUPTI_metrics_[sample|stop] don't do anything */
            Metrics.values[i].count = 0;
        }
        else
        {
            /* Enable continuous event sampling for this context */
            CUPTI_CHECK(cuptiSetEventCollectionMode(
                            context, CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS
                            ));
            
            /* Enable collection of the event group set */
            CUPTI_CHECK(cuptiEventGroupSetEnable(
                            &Metrics.values[i].sets->sets[0]
                            ));
        }
    }

    /* Restore (if necessary) the previous value of the current context */
    if (current != context)
    {
        CUDA_CHECK(cuCtxPopCurrent(&context));
        CUDA_CHECK(cuCtxPushCurrent(current));
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&Metrics.mutex));
}



/**
 * Sample the CUPTI metrics for all active CUDA contexts.
 *
 * @param tls       Thread-local storage of the current thread.
 * @param sample    Periodic sample to hold the CUPTI metric counts.
 */
void CUPTI_metrics_sample(TLS* tls, PeriodicSample* sample)
{
#if !defined(ENBALE_CUPTI_METRICS)
    return;
#endif

    PTHREAD_CHECK(pthread_mutex_lock(&Metrics.mutex));

    /* Initialize a new periodic sample to hold the metric deltas */
    PeriodicSample delta;
    memset(&delta, 0, sizeof(PeriodicSample));

    /** Compute the duration over which events were collected */
    uint64_t dt = sample->time - MetricOrigins.time;

    /* Iterate over each context in the table */
    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Metrics.values[i].context != NULL); ++i)
    {
        /* Skip this context if no metrics are being collected */
        if (Metrics.values[i].count == 0)
        {
            continue;
        }

        /* Read the counters for each event group for this context */

        uint64_t counts[MAX_EVENTS];
        CUpti_EventID ids[MAX_EVENTS];
        size_t counts_size, ids_size, event_count;

        size_t n = 0;
        
        int g;
        for (g = 0; g < Metrics.values[i].sets->sets[0].numEventGroups; ++g)
        {
            counts_size = (MAX_EVENTS - n) * sizeof(uint64_t);
            ids_size = (MAX_EVENTS - n) * sizeof(CUpti_EventID);
            event_count = 0;
            
            CUPTI_CHECK(cuptiEventGroupReadAllEvents(
                            Metrics.values[i].sets->sets[0].eventGroups[g],
                            CUPTI_EVENT_READ_FLAG_NONE,
                            &counts_size, &counts[n],
                            &ids_size, &ids[n],
                            &event_count
                            ));

            n += event_count;
        }

        /* Compute each of the metrics for this context */
        int m = 0;
        for (m = 0; m < Metrics.values[i].count; ++m)
        {
            CUpti_MetricValue metric;
            
            CUPTI_CHECK(cuptiMetricGetValue(
                            Metrics.values[i].device,
                            Metrics.values[i].ids[m],
                            n * sizeof(CUpti_EventID), ids,
                            n * sizeof(uint64_t), counts,
                            dt,
                            &metric
                            ));
            
            delta.count[Metrics.values[i].to_periodic[m]] +=
                metric.metricValueUint64;
        }
    }

    /*
     * Compute absolute metrics by adding the deltas to the metric origins
     * and copy these absolute counts into the provided sample. But only do
     * this for non-zero deltas to avoid overwriting samples from the other
     * data collection methods.
     */

    size_t e;
    for (e = 0; e < MAX_EVENTS; ++e)
    {
        if (delta.count[e] > 0)
        {
            MetricOrigins.count[e] += delta.count[e];
            sample->count[e] = MetricOrigins.count[e];
        }
    }

    /** Update the time origin for the next metric computation */
    MetricOrigins.time = sample->time;
    
    PTHREAD_CHECK(pthread_mutex_unlock(&Metrics.mutex));
}



/**
 * Stop CUPTI metrics data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be stopped.
 */
void CUPTI_metrics_stop(CUcontext context)
{
#if !defined(ENBALE_CUPTI_METRICS)
    return;
#endif

#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] CUPTI_metrics_stop(%p)\n", context);
    }
#endif

    PTHREAD_CHECK(pthread_mutex_lock(&Metrics.mutex));

    /* Find the specified context in the table */

    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Metrics.values[i].context != NULL); ++i)
    {
        if (Metrics.values[i].context == context)
        {
            break;
        }
    }
    
    if (i == MAX_CONTEXTS)
    {
        fprintf(stderr, "[CBTF/CUDA] CUPTI_metrics_stop(): "
                "Unknown CUDA context pointer (%p) encountered!\n", context);
        fflush(stderr);
        abort();
    }

    /* Are any metrics being collected? */
    if (Metrics.values[i].count > 0)
    {
        /* Disable collection of the event group set */
        CUPTI_CHECK(cuptiEventGroupSetDisable(
                        &Metrics.values[i].sets->sets[0]
                        ));
        
        /* Destroy all of the event group sets. */
        CUPTI_CHECK(cuptiEventGroupSetsDestroy(Metrics.values[i].sets));
    }

    PTHREAD_CHECK(pthread_mutex_unlock(&Metrics.mutex));
}
