/*******************************************************************************
** Copyright (c) 2016-2017 Argo Navis Technologies. All Rights Reserved.
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

#include <cupti.h>
#include <errno.h>
#include <monitor.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <KrellInstitute/Messages/CUDA_data.h>
#include <KrellInstitute/Messages/LinkedObjectEvents.h>
#include <KrellInstitute/Messages/Thread.h>
#include <KrellInstitute/Messages/ThreadEvents.h>
#include <KrellInstitute/Messages/ToolMessageTags.h>

#include <KrellInstitute/Services/Assert.h>
#include <KrellInstitute/Services/Send.h>

#include "collector.h"
#include "CUDA_check.h"
#include "CUPTI_check.h"
#include "CUPTI_context.h"
#include "CUPTI_metrics.h"
#include "Pthread_check.h"

extern void CBTF_MRNet_Send(const int, const xdrproc_t, const void*);



/** Flag indicating if CUDA kernel execution is to be serialized. */
bool CUPTI_metrics_do_kernel_serialization = FALSE;

/**
 * Boolean flag used by CUPTI_metrics_finalize() to request the exit of the
 * sampling thread. Marked volatile to insure the compiler doesn't optimize
 * out references to this variable.
 */
static volatile bool ExitSamplingThread = false;

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

        /** Class of the CUDA device for this context. */
        CUpti_DeviceAttributeDeviceClass class;
        
        /** Number of metrics. */
        int count;
        
        /** Identifiers for the metrics. */
        CUpti_MetricID ids[MAX_EVENTS];

        /** Map metric indicies to periodic count indicies. */
        int to_periodic[MAX_EVENTS];

        /** Handle to the CUPTI event group sets for this context. */
        CUpti_EventGroupSets* sets;

        /** Is the continuous event sampling mode used for this context? */
        bool is_continuous;
        
        /**
         * Each time a CUPTI event is read via cuptiEventGroupReadAllEvents()
         * it is automatically reset. No choice. Event counts in PeriodicSample,
         * however, are expected to be monotonically increasing absolute counts.
         * These origins are used to convert the event deltas returned by CUPTI
         * into absolute counts.
         */
        PeriodicSample origins;

        /**
         * Fake (i.e. actually per-context) thread-local storage used to store
         * and send samples for this context.
         */
        TLS tls;

    } values[MAX_CONTEXTS];
    pthread_rwlock_t mutex;
} Metrics = { { 0 }, PTHREAD_RWLOCK_INITIALIZER };

/**
 * Pthread ID of the sampling thread. Initialized by CUPTI_metrics_initialize()
 * and used by CUPTI_metrics_finalize() to wait for the thread to exit.
 */
static pthread_t SamplingThreadID;



/**
 * Sample the CUPTI metrics for the specified CUDA context.
 *
 * @param i    Index of the CUDA context's entry in the Metrics table.
 *
 * @note    This function does <em>not</em> acquire the Metrics
 *          mutex itself. The caller must have already done this.
 */
static void take_sample(int i)
{
    /* Initialize a new periodic sample */
    PeriodicSample sample;
    memset(&sample, 0, sizeof(PeriodicSample));
    CUPTI_CHECK(cuptiGetTimestamp(&sample.time));
    
    /* Read the counters for each event group for this context */
    
    static const size_t kMaxCounters = MAX_EVENTS * 16;
    
    uint64_t counts[kMaxCounters];
    CUpti_EventID ids[kMaxCounters];
    size_t counts_size, ids_size, event_count;
    
    size_t n = 0;

    uint32_t s;
    for (s = 0; s < Metrics.values[i].sets->numSets; ++s)
    {
        uint32_t g;
        for (g = 0; g < Metrics.values[i].sets->sets[0].numEventGroups; ++g)
        {
            counts_size = (kMaxCounters - n) * sizeof(uint64_t);
            ids_size = (kMaxCounters - n) * sizeof(CUpti_EventID);
            event_count = 0;
            
            CUPTI_CHECK(cuptiEventGroupReadAllEvents(
                            Metrics.values[i].sets->sets[s].eventGroups[g],
                            CUPTI_EVENT_READ_FLAG_NONE,
                            &counts_size, &counts[n],
                            &ids_size, &ids[n],
                            &event_count
                            ));
            
            n += event_count;
        }
    }
    
    /** Compute the duration over which events were collected */
    uint64_t dt = sample.time - Metrics.values[i].origins.time;
    
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
        
        int e = Metrics.values[i].to_periodic[m];
        
        sample.count[e] = Metrics.values[i].origins.count[e] + 
            metric.metricValueUint64;
    }
    
    /* Add this sample to the performance data blob for this context */
    TLS_add_periodic_sample(&Metrics.values[i].tls, &sample);
    
    /* Update the origins */
    size_t e;
    for (e = 0; e < MAX_EVENTS; ++e)
    {
        Metrics.values[i].origins.count[e] = sample.count[e];
    }
    Metrics.values[i].origins.time = sample.time;
}



/**
 * Initialize CUPTI metrics data collection for this process.
 */
void CUPTI_metrics_initialize()
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CUDA %d:%d] CUPTI_metrics_initialize()\n",
               getpid(), monitor_get_thread_num());
    }
#endif

    /* Start the sampling thread */
    PTHREAD_CHECK(pthread_create(&SamplingThreadID, NULL,
                                 CUPTI_metrics_sampling_thread, NULL));
}



/**
 * Start metrics data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be started.
 */
void CUPTI_metrics_start(CUcontext context)
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CUDA %d:%d] CUPTI_metrics_start(%p)\n",
               getpid(), monitor_get_thread_num(), context);
    }
#endif

    PTHREAD_CHECK(pthread_rwlock_wrlock(&Metrics.mutex));

    /* Find an empty entry in the table for this context */

    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Metrics.values[i].context != NULL); ++i)
    {
        if (Metrics.values[i].context == context)
        {
            fprintf(stderr, "[CUDA %d:%d] CUPTI_metrics_start(): "
                    "Redundant call for CUPTI context pointer (%p)!",
                    getpid(), monitor_get_thread_num(), context);
            fflush(stderr);
            abort();
        }
    }
    
    if (i == MAX_CONTEXTS)
    {
        fprintf(stderr, "[CUDA %d:%d] CUPTI_metrics_start(): "
                "Maximum supported CUDA context pointers (%d) was reached!\n",
                getpid(), monitor_get_thread_num(), MAX_CONTEXTS);
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

    /* Get the class of the device for this context */
    size_t classSize = sizeof(CUpti_DeviceAttributeDeviceClass);
    CUPTI_CHECK(cuptiDeviceGetAttribute(
                    Metrics.values[i].device, CUPTI_DEVICE_ATTR_DEVICE_CLASS,
                    &classSize, &Metrics.values[i].class));

#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        static char* kClasses[4] = { "Tesla", "Quadro", "GeForce", "Tegra" };
        
        if (Metrics.values[i].class > CUPTI_DEVICE_ATTR_DEVICE_CLASS_TEGRA)
        {
            printf("[CUDA %d:%d] found Unknown-class device for context %p\n",
                   getpid(), monitor_get_thread_num(), context);
        }
        else
        {
            printf("[CUDA %d:%d] found %s-class device for context %p\n",
                   getpid(), monitor_get_thread_num(),
                   kClasses[Metrics.values[i].class], context);
        }
    }
#endif
    
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
            fprintf(stderr, "[CUDA %d:%d] CUPTI_metrics_start(): "
                    "Valid GPU event \"%s\" is of an unsupported value kind "
                    "(%d). Ignoring this event.\n", 
                    getpid(), monitor_get_thread_num(), event->name, kind);
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
            printf("[CUDA %d:%d] recording GPU metric \"%s\" for context %p\n",
                   getpid(), monitor_get_thread_num(), event->name, context);
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
         * Enable the proper event collection mode, depending on the number
         * of passes required to collect the these metrics, and the class of
         * device associated with this context,
         */
        if (Metrics.values[i].sets->numSets > 1)
        {
            CUPTI_metrics_do_kernel_serialization = TRUE;
            Metrics.values[i].is_continuous = FALSE;

            fprintf(stderr, "[CUDA %d:%d] CUPTI_metrics_start(): "
                    "The specified GPU events cannot be collected in a "
                    "single pass. Thus CUDA kernel replay mode has been "
                    "enabled. GPU events will be sampled at CUDA kernel "
                    "entry and exit only (not periodically). This also "
                    "implies CUDA kernel execution will be serialized, "
                    "possibly exhibiting different temporal behavior, "
                    "and longer execution times, than when executed "
                    "without performance monitoring.\n",
                    getpid(), monitor_get_thread_num());
            fflush(stderr);

            /*
             * Enable kernel replay mode for this context. This also implies,
             * and is only available with, kernel event sampling mode.
             */
            CUPTI_CHECK(cuptiEnableKernelReplayMode(context));
        }
        else if (Metrics.values[i].class != 
                 CUPTI_DEVICE_ATTR_DEVICE_CLASS_TESLA)
        {
            CUPTI_metrics_do_kernel_serialization = TRUE;
            Metrics.values[i].is_continuous = FALSE;

            fprintf(stderr, "[CUDA %d:%d] CUPTI_metrics_start(): "
                    "The selected CUDA device doesn't support continuous "
                    "GPU event sampling. GPU events will be sampled at CUDA "
                    "kernel entry and exit only (not peridiocally). This "
                    "also implies CUDA kernel execution will be serialized, "
                    "possibly exhibiting different temporal behavior than "
                    "when executed without performance monitoring.\n",
                    getpid(), monitor_get_thread_num());
            fflush(stderr);
            
            /* Enable kernel event sampling mode for this context */
            CUPTI_CHECK(cuptiSetEventCollectionMode(
                            context, CUPTI_EVENT_COLLECTION_MODE_KERNEL
                            ));
        }
        else
        {
            Metrics.values[i].is_continuous = TRUE;

            /* Enable continuous event sampling mode for this context */
            CUPTI_CHECK(cuptiSetEventCollectionMode(
                            context, CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS
                            ));
        }

        /* Enable collection of the event group sets */
        uint32_t j;
        for (j = 0; j < Metrics.values[i].sets->numSets; ++j)
        {
            CUPTI_CHECK(cuptiEventGroupSetEnable(
                            &Metrics.values[i].sets->sets[j]
                            ));
        }

        /* Access our thread-local storage */
        TLS* tls = TLS_get();
        
        /* Initialize the performance data header and blob for this context */
        memset(&Metrics.values[i].tls, 0, sizeof(TLS));
        memcpy(&Metrics.values[i].tls.data_header, &tls->data_header,
               sizeof(CBTF_DataHeader));
        Metrics.values[i].tls.data_header.posix_tid = (int64_t)(context);
        Metrics.values[i].tls.data_header.omp_tid = -1;
        TLS_initialize_data(&Metrics.values[i].tls);

        /* Append event sampling configuration to our performance data blob */

        CBTF_cuda_message* raw_message = 
            TLS_add_message(&Metrics.values[i].tls);
        Assert(raw_message != NULL);
        raw_message->type = SamplingConfig;
        
        CUDA_SamplingConfig* message = 
            &raw_message->CBTF_cuda_message_u.sampling_config;
        
        memcpy(message, &TheSamplingConfig, sizeof(CUDA_SamplingConfig));
        
        /** Ensure upstream processes know about this "thread" */
        
        CBTF_Protocol_ThreadName name;
        name.experiment = Metrics.values[i].tls.data_header.experiment;
        name.host = Metrics.values[i].tls.data_header.host;
        name.pid = Metrics.values[i].tls.data_header.pid;
        name.has_posix_tid = TRUE;
        name.posix_tid = Metrics.values[i].tls.data_header.posix_tid;
        name.rank = Metrics.values[i].tls.data_header.rank;
        name.omp_tid = Metrics.values[i].tls.data_header.omp_tid;
        
        CBTF_Protocol_AttachedToThreads attached;
        attached.threads.names.names_len = 1;
        attached.threads.names.names_val = &name;

        CBTF_MRNet_Send(CBTF_PROTOCOL_TAG_ATTACHED_TO_THREADS,
                        (xdrproc_t)xdr_CBTF_Protocol_AttachedToThreads,
                        &attached);

        static char* kPath = "/bin/ps";
        
        CBTF_Protocol_LinkedObject object;
        object.linked_object.path = kPath;
        object.linked_object.checksum = 0;
        object.range.begin = 0xFFFF0BADC0DABEEF;
        object.range.end = object.range.begin + 1;
        CUPTI_CHECK(cuptiGetTimestamp(&object.time_begin));
        object.time_end = -1;
        object.is_executable = false;
        
        CBTF_Protocol_LinkedObjectGroup group;
        memcpy(&group.thread, &name, sizeof(CBTF_Protocol_ThreadName));
        group.linkedobjects.linkedobjects_len = 1;
        group.linkedobjects.linkedobjects_val = &object;
        
        CBTF_MRNet_Send(CBTF_PROTOCOL_TAG_LINKED_OBJECT_GROUP,
                        (xdrproc_t)xdr_CBTF_Protocol_LinkedObjectGroup,
                        &group);
    }
    
    /* Restore (if necessary) the previous value of the current context */
    if (current != context)
    {
        CUDA_CHECK(cuCtxPopCurrent(&context));
        CUDA_CHECK(cuCtxPushCurrent(current));
    }

    PTHREAD_CHECK(pthread_rwlock_unlock(&Metrics.mutex));
}



/**
 * Sample the CUPTI metrics for the specified CUDA context.
 *
 * @param context    CUDA context for which a sample is to be taken.
 */
void CUPTI_metrics_sample(CUcontext context)
{
    PTHREAD_CHECK(pthread_rwlock_rdlock(&Metrics.mutex));
        
    /* Iterate over each context in the table */
    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Metrics.values[i].context != NULL); ++i)
    {
        /* Should this context be sampled? */
        if ((Metrics.values[i].context == context) &&
            (Metrics.values[i].count > 0) &&
            !Metrics.values[i].is_continuous)
        {
            take_sample(i);
        }
    }
    
    PTHREAD_CHECK(pthread_rwlock_unlock(&Metrics.mutex));
}



/**
 * Thread function implementing the sampling of CUPTI metrics data collection.
 *
 * @param arg    Unused.
 * @return       Always returns NULL.
 *
 * @note    The only reason this thread function isn't completely hidden
 *          inside the source file is that cbtf_collector_[start&stop]()
 *          needs the thread's address to suppress PAPI event collection
 *          for this thread.
 */
void* CUPTI_metrics_sampling_thread(void* arg)
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CUDA %d:%d] CUPTI_metrics_sampling_thread()\n",
               getpid(), monitor_get_thread_num());
    }
#endif

    /* Loop until CUPTI_metrics_finalize() tells us to exit */
    while (!ExitSamplingThread)
    {
        PTHREAD_CHECK(pthread_rwlock_rdlock(&Metrics.mutex));
        
        /* Iterate over each context in the table */
        int i;
        for (i = 0; 
             (i < MAX_CONTEXTS) && (Metrics.values[i].context != NULL);
             ++i)
        {
            /* Should this context be sampled? */
            if ((Metrics.values[i].count > 0) &&
                Metrics.values[i].is_continuous)
            {
                take_sample(i);
            }
        }
        
        PTHREAD_CHECK(pthread_rwlock_unlock(&Metrics.mutex));
        
        /* Sleep for the configuration-specified period  */
        usleep(TheSamplingConfig.interval / 1000 /* uS/nS */);
    }
    
    return NULL;
}



/**
 * Stop CUPTI metrics data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be stopped.
 */
void CUPTI_metrics_stop(CUcontext context)
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CUDA %d:%d] CUPTI_metrics_stop(%p)\n",
               getpid(), monitor_get_thread_num(), context);
    }
#endif

    PTHREAD_CHECK(pthread_rwlock_wrlock(&Metrics.mutex));

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
        fprintf(stderr, "[CUDA %d:%d] CUPTI_metrics_stop(): "
                "Unknown CUDA context pointer (%p) encountered!\n",
                getpid(), monitor_get_thread_num(), context);
        fflush(stderr);
        abort();
    }

    /* Are any metrics being collected? */
    if (Metrics.values[i].count > 0)
    {
        /* Disable kernel replay mode (if previously enabled) */
        if (Metrics.values[i].sets->numSets > 1)
        {
            CUPTI_CHECK(cuptiDisableKernelReplayMode(context));
        }

        /* Disable collection of the event group sets */
        uint32_t j;
        for (j = 0; j < Metrics.values[i].sets->numSets; ++j)
        {
            CUPTI_CHECK(cuptiEventGroupSetDisable(
                            &Metrics.values[i].sets->sets[j]
                            ));
        }
        
        /* Destroy all of the event group sets. */
        CUPTI_CHECK(cuptiEventGroupSetsDestroy(Metrics.values[i].sets));

        /* Insure CUPTI_metrics_sample doesn't do anything */
        Metrics.values[i].count = 0;

        /* Send any remaining performance data for this context */
        TLS_send_data(&Metrics.values[i].tls);

        /** Ensure upstream processes know about this "thread"'s termination */
        
        CBTF_Protocol_ThreadName name;
        name.experiment = Metrics.values[i].tls.data_header.experiment;
        name.host = Metrics.values[i].tls.data_header.host;
        name.pid = Metrics.values[i].tls.data_header.pid;
        name.has_posix_tid = TRUE;
        name.posix_tid = Metrics.values[i].tls.data_header.posix_tid;
        name.rank = Metrics.values[i].tls.data_header.rank;
        name.omp_tid = Metrics.values[i].tls.data_header.omp_tid;
        
        CBTF_Protocol_ThreadsStateChanged terminated;
        terminated.threads.names.names_len = 1;
        terminated.threads.names.names_val = &name;
        terminated.state = Terminated;

        CBTF_MRNet_Send(CBTF_PROTOCOL_TAG_THREADS_STATE_CHANGED,
                        (xdrproc_t)xdr_CBTF_Protocol_ThreadsStateChanged,
                        &terminated);
    }
    
    PTHREAD_CHECK(pthread_rwlock_unlock(&Metrics.mutex));
}



/**
 * Finalize CUPTI metrics data collection for this process.
 */
void CUPTI_metrics_finalize()
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CUDA %d:%d] CUPTI_metrics_finalize()\n",
               getpid(), monitor_get_thread_num());
    }
#endif

    /* Stop the sampling thread */
    ExitSamplingThread = true;
    PTHREAD_CHECK(pthread_join(SamplingThreadID, NULL));
}
