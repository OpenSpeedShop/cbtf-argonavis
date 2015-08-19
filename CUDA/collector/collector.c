/*******************************************************************************
** Copyright (c) 2012-2015 Argo Navis Technologies. All Rights Reserved.
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

/** @file Implementation of the CUDA collector. */

#include <cupti.h>
#include <inttypes.h>
#include <malloc.h>
#if defined(PAPI_FOUND)
#include <papi.h>
#endif
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "KrellInstitute/Services/Assert.h"
#include "KrellInstitute/Services/Collector.h"
#include "KrellInstitute/Services/Time.h"
#include "KrellInstitute/Services/Timer.h"

#include "CUPTI_check.h"
#include "CUPTI_context.h"
#include "CUPTI_stream.h"
#include "Pthread_check.h"
#include "TLS.h"



/**
 * Size (in bytes) of each allocated CUPTI activity buffer.
 *
 * @note    Currently the only basis for the selection of this value is that the
 *          CUPTI "activity_trace_async.cpp" example uses buffers of 32 KB each.
 */
#define CUPTI_ACTIVITY_BUFFER_SIZE (32 * 1024 /* 32 KB */)



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



/** String uniquely identifying this collector. */
const char* const cbtf_collector_unique_id = "cuda";

/** Flag indicating if debugging is enabled. */
bool DebugEnabled = FALSE;

/**
 * The number of threads for which are are collecting data (actively or not).
 * This value is atomically incremented in cbtf_collector_start(), decremented
 * in cbtf_collector_stop(), and is used by those functions to determine when
 * to perform process-wide initialization and finalization.
 */
static struct {
    int value;
    pthread_mutex_t mutex;
} thread_count = { 0, PTHREAD_MUTEX_INITIALIZER };

/** CUPTI subscriber handle for this collector. */
static CUpti_SubscriberHandle cupti_subscriber_handle;

/**
 * The offset that must be added to all CUPTI-provided time values in order to
 * translate them to the same time "origin" provided by CBTF_GetTime(). Computed
 * by the process-wide initialization in cbtf_collector_start().
 */
static int64_t cupti_time_offset = 0;

#if defined(PAPI_FOUND)
/**
 * The number of active CUDA contexts in this process. This value is atomically
 * incremented and decremented in cupti_callback(), and is used by that function
 * to determine when to perform PAPI initialization and finalization.
 *
 * Initializing the PAPI CUDA component before the application has itself called
 * cuInit() causes the process to hang on some systems. Thus PAPI initialization
 * must be deferred until after the application calls cuInit(), and must also be
 * tracked separately from the regular process-wide initialization controlled by
 * thread_count.
 */
static struct {
    int value;
    pthread_mutex_t mutex;
} context_count = { 0, PTHREAD_MUTEX_INITIALIZER };

/**
 * Event sampling configuration. Initialized by the process-wide initialization
 * in cbtf_collector_start() through its call to parse_configuration().
 */
static CUDA_SamplingConfig sampling_config;

/**
 * Descriptions of sampled events. Also initialized by the process-wide
 * initialization in cbtf_collector_start() through parse_configuration().
 */
static CUDA_EventDescription event_descriptions[MAX_EVENTS];

/** Number of events for which overflow sampling is enabled. */
int OverflowSamplingCount = 0;
#endif


    
/**
 * Convert a CUpti_ActivityMemcpyKind enumerant to a CUDA_CopyKind enumerant.
 *
 * @param value    Value to be converted from CUpti_ActivityMemcpyKind.
 * @return         Corresponding CUDA_CopyKind enumerant.
 */
inline CUDA_CopyKind toCopyKind(CUpti_ActivityMemcpyKind value)
{
    switch (value)
    {
    case CUPTI_ACTIVITY_MEMCPY_KIND_UNKNOWN:
        return UnknownCopyKind;
    case CUPTI_ACTIVITY_MEMCPY_KIND_HTOD:
        return HostToDevice;
    case CUPTI_ACTIVITY_MEMCPY_KIND_DTOH:
        return DeviceToHost;
    case CUPTI_ACTIVITY_MEMCPY_KIND_HTOA:
        return HostToArray;
    case CUPTI_ACTIVITY_MEMCPY_KIND_ATOH:
        return ArrayToHost;
    case CUPTI_ACTIVITY_MEMCPY_KIND_ATOA:
        return ArrayToArray;
    case CUPTI_ACTIVITY_MEMCPY_KIND_ATOD:
        return ArrayToDevice;
    case CUPTI_ACTIVITY_MEMCPY_KIND_DTOA:
        return DeviceToArray;
    case CUPTI_ACTIVITY_MEMCPY_KIND_DTOD:
        return DeviceToDevice;
    case CUPTI_ACTIVITY_MEMCPY_KIND_HTOH:
        return HostToHost;
    default:
        return InvalidCopyKind;
    }
}



/**
 * Convert a CUpti_ActivityMemoryKind enumerant to a CUDA_MemoryKind enumerant.
 *
 * @param value    Value to be converted from CUpti_ActivityMemoryKind.
 * @return         Corresponding CUDA_MemoryKind enumerant.
 */
inline CUDA_MemoryKind toMemoryKind(CUpti_ActivityMemoryKind value)
{
    switch (value)
    {
    case CUPTI_ACTIVITY_MEMORY_KIND_UNKNOWN:
        return UnknownMemoryKind;
    case CUPTI_ACTIVITY_MEMORY_KIND_PAGEABLE:
        return Pageable;
    case CUPTI_ACTIVITY_MEMORY_KIND_PINNED:
        return Pinned;
    case CUPTI_ACTIVITY_MEMORY_KIND_DEVICE:
        return Device;
    case CUPTI_ACTIVITY_MEMORY_KIND_ARRAY:
        return Array;
    default:
        return InvalidMemoryKind;
    }
}



/**
 * Convert a CUfunc_cache enumerant to a CUDA_CachePerference enumerant.
 *
 * @param value    Value to be converted from CUfunc_cache.
 * @return         Corresponding CUDA_CachePreference enumerant.
 */
inline CUDA_CachePreference toCachePreference(CUfunc_cache value)
{
    switch (value)
    {
    case CU_FUNC_CACHE_PREFER_NONE:
        return NoPreference;
    case CU_FUNC_CACHE_PREFER_SHARED:
        return PreferShared;
    case CU_FUNC_CACHE_PREFER_L1:
        return PreferCache;
    case CU_FUNC_CACHE_PREFER_EQUAL:
        return PreferEqual;
    default:
        return InvalidCachePreference;
    }
}



/**
 * Add the activities for the specified CUDA context/stream to the performance
 * data blob contained within the given thread-local storage.
 *
 * @param tls          Thread-local storage to which activities are to be added.
 * @param context      CUDA context for the activities to be added.
 * @param stream       CUDA stream for the activities to be added.
 * @param stream_id    CUDA stream ID for the activities to be added.
 * @param buffer       Buffer containing the activity records.
 * @param size         Actual size of the buffer.
 */
static void add_activities(TLS* tls, CUcontext context, CUstream stream,
                           uint32_t stream_id, uint8_t* buffer, size_t size)
{
    Assert(tls != NULL);

    /* Warn if any activity records were dropped */
    size_t dropped = 0;
    CUPTI_CHECK(cuptiActivityGetNumDroppedRecords(context, stream_id,
                                                  &dropped));
    if (dropped > 0)
    {
        fprintf(stderr, "[CBTF/CUDA] add_activities(): "
                "dropped %u activity records for stream %p in context %p\n",
                (unsigned int)dropped, stream, context);
        fflush(stderr);
    }
    
    /* Iterate over each activity record */
    
    CUpti_Activity* raw_activity = NULL;

    size_t added;
    for (added = 0; true; ++added)
    {
        CUptiResult retval = cuptiActivityGetNextRecord(buffer, size,
                                                        &raw_activity);

        if (retval == CUPTI_ERROR_MAX_LIMIT_REACHED)
        {
            break;
        }
        
        CUPTI_CHECK(retval);
        
        /* Determine the activity type and handle it */
        switch (raw_activity->kind)
        {
            


        case CUPTI_ACTIVITY_KIND_CONTEXT:
            {
                const CUpti_ActivityContext* const activity =
                    (CUpti_ActivityContext*)raw_activity;

                /* Add a message for this activity */

                CBTF_cuda_message* raw_message = TLS_add_message(tls);
                Assert(raw_message != NULL);
                raw_message->type = ContextInfo;
                
                CUDA_ContextInfo* message =
                    &raw_message->CBTF_cuda_message_u.context_info;

                message->context = (CBTF_Protocol_Address)
                    CUPTI_context_ptr_from_id(activity->contextId);

                message->device = activity->deviceId;
                
                switch (activity->computeApiKind)
                {

                case CUPTI_ACTIVITY_COMPUTE_API_UNKNOWN:
                    message->compute_api = "Unknown";
                    break;

                case CUPTI_ACTIVITY_COMPUTE_API_CUDA:
                    message->compute_api = "CUDA";
                    break;

#if (CUPTI_API_VERSION == 2)
                case CUPTI_ACTIVITY_COMPUTE_API_OPENCL:
                    message->compute_api = "OpenCL";
                    break;
#endif

                default:
                    message->compute_api = "Invalid!";
                    break;

                }
            }
            break;



        case CUPTI_ACTIVITY_KIND_DEVICE:
            {
                const CUpti_ActivityDevice* const activity =
                    (CUpti_ActivityDevice*)raw_activity;

                /* Add a message for this activity */

                CBTF_cuda_message* raw_message = TLS_add_message(tls);
                Assert(raw_message != NULL);
                raw_message->type = DeviceInfo;
                
                CUDA_DeviceInfo* message =
                    &raw_message->CBTF_cuda_message_u.device_info;

                message->device = activity->id;

                message->name = (char*)activity->name;

                message->compute_capability[0] = 
                    activity->computeCapabilityMajor;
                message->compute_capability[1] = 
                    activity->computeCapabilityMinor;

                message->max_grid[0] = activity->maxGridDimX;
                message->max_grid[1] = activity->maxGridDimY;
                message->max_grid[2] = activity->maxGridDimZ;

                message->max_block[0] = activity->maxBlockDimX;
                message->max_block[1] = activity->maxBlockDimY;
                message->max_block[2] = activity->maxBlockDimZ;

                message->global_memory_bandwidth = 
                    activity->globalMemoryBandwidth;

                message->global_memory_size = activity->globalMemorySize;
                message->constant_memory_size = activity->constantMemorySize;
                message->l2_cache_size = activity->l2CacheSize;
                message->threads_per_warp = activity->numThreadsPerWarp;
                message->core_clock_rate = activity->coreClockRate;
                message->memcpy_engines = activity->numMemcpyEngines;
                message->multiprocessors = activity->numMultiprocessors;
                message->max_ipc = activity->maxIPC;
                
                message->max_warps_per_multiprocessor = 
                    activity->maxWarpsPerMultiprocessor;

                message->max_blocks_per_multiprocessor =
                    activity->maxBlocksPerMultiprocessor;

                message->max_registers_per_block =
                    activity->maxRegistersPerBlock;

                message->max_shared_memory_per_block =
                    activity->maxSharedMemoryPerBlock;

                message->max_threads_per_block = activity->maxThreadsPerBlock;
            }
            break;



        case CUPTI_ACTIVITY_KIND_MEMCPY:
            {
                const CUpti_ActivityMemcpy* const activity =
                    (CUpti_ActivityMemcpy*)raw_activity;

                /* Add a message for this activity */

                CBTF_cuda_message* raw_message = TLS_add_message(tls);
                Assert(raw_message != NULL);
                raw_message->type = CopiedMemory;
                
                CUDA_CopiedMemory* message =
                    &raw_message->CBTF_cuda_message_u.copied_memory;

                message->context = (CBTF_Protocol_Address)context;
                message->stream = (CBTF_Protocol_Address)stream;
                
                message->time_begin = activity->start + cupti_time_offset;
                message->time_end = activity->end + cupti_time_offset;

                message->size = activity->bytes;

                message->kind = toCopyKind(activity->copyKind);
                message->source_kind = toMemoryKind(activity->srcKind);
                message->destination_kind = toMemoryKind(activity->dstKind);

                message->asynchronous = 
                    (activity->flags & CUPTI_ACTIVITY_FLAG_MEMCPY_ASYNC) ?
                    true : false;
                
                TLS_update_header_with_time(tls, message->time_begin);
                TLS_update_header_with_time(tls, message->time_end);

#if (CUPTI_API_VERSION < 5)
                /* Add the context ID to pointer mapping from this activity */
                CUPTI_context_id_to_ptr(activity->contextId, context);
#endif
            }
            break;



        case CUPTI_ACTIVITY_KIND_MEMSET:
            {
                const CUpti_ActivityMemset* const activity =
                    (CUpti_ActivityMemset*)raw_activity;

                /* Add a message for this activity */

                CBTF_cuda_message* raw_message = TLS_add_message(tls);
                Assert(raw_message != NULL);
                raw_message->type = SetMemory;
                
                CUDA_SetMemory* message =
                    &raw_message->CBTF_cuda_message_u.set_memory;

                message->context = (CBTF_Protocol_Address)context;
                message->stream = (CBTF_Protocol_Address)stream;

                message->time_begin = activity->start + cupti_time_offset;
                message->time_end = activity->end + cupti_time_offset;

                message->size = activity->bytes;
                
                TLS_update_header_with_time(tls, message->time_begin);
                TLS_update_header_with_time(tls, message->time_end);

#if (CUPTI_API_VERSION < 5)
                /* Add the context ID to pointer mapping from this activity */
                CUPTI_context_id_to_ptr(activity->contextId, context);
#endif
            }
            break;



        case CUPTI_ACTIVITY_KIND_KERNEL:
            {
                const CUpti_ActivityKernel* const activity =
                    (CUpti_ActivityKernel*)raw_activity;

                /* Add a message for this activity */

                CBTF_cuda_message* raw_message = TLS_add_message(tls);
                Assert(raw_message != NULL);
                raw_message->type = ExecutedKernel;

                CUDA_ExecutedKernel* message =
                    &raw_message->CBTF_cuda_message_u.executed_kernel;

                message->context = (CBTF_Protocol_Address)context;
                message->stream = (CBTF_Protocol_Address)stream;

                message->time_begin = activity->start + cupti_time_offset;
                message->time_end = activity->end + cupti_time_offset;

                message->function = (char*)activity->name;

                message->grid[0] = activity->gridX;
                message->grid[1] = activity->gridY;
                message->grid[2] = activity->gridZ;

                message->block[0] = activity->blockX;
                message->block[1] = activity->blockY;
                message->block[2] = activity->blockZ;

                message->cache_preference = 
                    toCachePreference(activity->cacheConfigExecuted);

                message->registers_per_thread = activity->registersPerThread;
                message->static_shared_memory = activity->staticSharedMemory;
                message->dynamic_shared_memory = activity->dynamicSharedMemory;
                message->local_memory = activity->localMemoryTotal;

                TLS_update_header_with_time(tls, message->time_begin);
                TLS_update_header_with_time(tls, message->time_end);

#if (CUPTI_API_VERSION < 5)
                /* Add the context ID to pointer mapping from this activity */
                CUPTI_context_id_to_ptr(activity->contextId, context);
#endif
            }
            break;



        default:
            break;
        }
    }

#if !defined(NDEBUG)
    if (DebugEnabled)
    {
        printf("[CBTF/CUDA] add_activities(): "
               "added %u activity records for stream %p in context %p\n",
               (unsigned int)added, stream, context);
    }
#endif
}



#if (CUPTI_API_VERSION < 4)
/**
 * Wrapper for add_activities() used with CUPTI API versions 2 and 3.
 *
 * @param tls          Thread-local storage to which activities are to be added.
 * @param context      CUDA context for the activities to be added.
 * @param stream       CUDA stream for the activities to be added.
 * @param stream_id    CUDA stream ID for the activities to be added.
 */
static void wrap_add_activities(TLS* tls, CUcontext context, 
                                CUstream stream, uint32_t stream_id)
{
    Assert(tls != NULL);

    /* Dequeue the buffer of activities */
    
    uint8_t* buffer = NULL;
    size_t size = 0;
    
    CUptiResult retval = cuptiActivityDequeueBuffer(
        context, stream_id, &buffer, &size
        );
    
    if (retval == CUPTI_ERROR_QUEUE_EMPTY)
    {
        return;
    }
    
    CUPTI_CHECK(retval);

    /* Actually add these activities */
    add_activities(tls, context, stream, stream_id, buffer, size);

    /* Re-enqueue this buffer of activities */
    CUPTI_CHECK(cuptiActivityEnqueueBuffer(
                    context, stream_id, buffer, CUPTI_ACTIVITY_BUFFER_SIZE
                    ));
}
#endif



#if (CUPTI_API_VERSION >= 4)
/**
 * Callback invoked by CUPTI (API versions 4 and above) each time it requires
 * a new activity buffer to be allocated.
 *
 * @retval buffer         Buffer to contain activity records.
 * @retval allocated      Allocated size of the buffer.
 * @retval max_records    Maximum number of records to put in this buffer.
 */
static void allocate_activities(uint8_t** buffer, size_t* allocated,
                                size_t* max_records)
{
    *buffer = memalign(ACTIVITY_RECORD_ALIGNMENT, CUPTI_ACTIVITY_BUFFER_SIZE);
    *allocated = CUPTI_ACTIVITY_BUFFER_SIZE;
    *max_records = 0; /* Fill with as many records as possible */
}
#endif



#if (CUPTI_API_VERSION >= 4)
/**
 * Callback invoked by CUPTI (API versions 4 and above) each time it has filled
 * a buffer with activity records.
 *
 * @param context      CUDA context for the activities to be added.
 * @param stream_id    CUDA stream ID for the activities to be added.
 * @param buffer       Buffer containing the activity records.
 * @param allocated    Allocated size of the buffer.
 * @param size         Actual size of the buffer.
 */
static void wrap_add_activities(CUcontext context, uint32_t stream_id,
                                uint8_t* buffer, size_t allocated, size_t size)
{
    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Find the CUDA stream pointer corresponding to this CUPTI stream ID */
    CUstream stream = CUPTI_stream_ptr_from_id(stream_id);
    
    /* Actually add these activities */
    add_activities(tls, context, stream, stream_id, buffer, size);
}
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
            if (tls->overflow_samples.message->pcs.pcs_len == 
                MAX_OVERFLOW_PCS_PER_BLOB)
            {
                TLS_send_data(tls);
                continue;
            }

            /* Add an entry for this sample to the existing overflow samples */
            pcs[tls->overflow_samples.message->pcs.pcs_len] = pc;
            memset(&counts[tls->overflow_samples.message->counts.counts_len],
                   0, OverflowSamplingCount * sizeof(uint64_t));            
            hash_table[bucket] = ++tls->overflow_samples.message->pcs.pcs_len;
            tls->overflow_samples.message->counts.counts_len += 
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

    if (time < tls->overflow_samples.message->time_begin)
    {
        tls->overflow_samples.message->time_begin = time;
    }
    if (time >= tls->overflow_samples.message->time_end)
    {
        tls->overflow_samples.message->time_end = time;
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
    int index = tls->periodic_samples.message->deltas.deltas_len;

    /*
     * Get pointers to the values in the new (current) and previous event
     * samples. Note that the time and the actual event counts are treated
     * identically as 64-bit unsigned integers.
     */

    const uint64_t* previous = &tls->periodic_samples.previous.time;
    const uint64_t* current = &sample.time;

    /* Iterate over each time and event count value in this event sample */
    int i, iEnd = sampling_config.events.events_len + 1;
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
            index = tls->periodic_samples.message->deltas.deltas_len;
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
    tls->periodic_samples.message->deltas.deltas_len = index;

    /* Update the header with this sample time */
    TLS_update_header_with_time(tls, sample.time);
    
    /* Replace the previous event sample with the new event sample */
    memcpy(&tls->periodic_samples.previous, &sample, sizeof(EventSample));
}
#endif



#if defined(PAPI_FOUND)
/**
 * Start PAPI data collection for this thread.
 *
 * @param tls    Thread-local storage for this thread.
 */
static void start_papi_data_collection(TLS* tls)
{
    Assert(tls != NULL);

    /* Was PAPI data collection already started for this thread? */
    if (tls->eventset_count > 0)
    {
        return;
    }

#if !defined(NDEBUG)
    if (DebugEnabled)
    {
        printf("[CBTF/CUDA] start_papi_data_collection()\n");
    }

#endif

    /* Atomically test whether PAPI has been started */
    
    PTHREAD_CHECK(pthread_mutex_lock(&context_count.mutex));
    
    if (context_count.value > 0)
    {
        /* Iterate over each event in our event sampling configuration */
        uint i, overflow_count = 0, periodic_count = 0;
        for (i = 0; i < sampling_config.events.events_len; ++i)
        {
            CUDA_EventDescription* event = 
                &sampling_config.events.events_val[i];

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
        
        CBTF_Timer(sampling_config.interval, timer_callback);
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&context_count.mutex));
}
#endif



#if defined(PAPI_FOUND)
/**
 * Stop PAPI data collection for this thread.
 *
 * @param tls    Thread-local storage for this thread.
 */
static void stop_papi_data_collection(TLS* tls)
{
    Assert(tls != NULL);

    /* Was PAPI data collection not started for this thread? */
    if (tls->eventset_count == 0)
    {
        return;
    }

#if !defined(NDEBUG)
    if (DebugEnabled)
    {
        printf("[CBTF/CUDA] stop_papi_data_collection()\n");
    }
#endif

    /* Atomically test whether PAPI has been started */
    
    PTHREAD_CHECK(pthread_mutex_lock(&context_count.mutex));
    
    if (context_count.value > 0)
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
    
    PTHREAD_CHECK(pthread_mutex_unlock(&context_count.mutex));
}
#endif



#if defined(PAPI_FOUND)
/**
 * Called by cupti_callback() in order to start (initialize) PAPI.
 */
static void start_papi()
{
    /* Atomically increment the active CUDA context count */

    PTHREAD_CHECK(pthread_mutex_lock(&context_count.mutex));
    
#if !defined(NDEBUG)
    if (DebugEnabled)
    {
        printf("[CBTF/CUDA] start_papi(): "
               "context_count.value = %d --> %d\n",
               context_count.value, context_count.value + 1);
    }
#endif
    
    if (context_count.value == 0)
    {
        /* Initialize PAPI */
        PAPI_CHECK(PAPI_library_init(PAPI_VER_CURRENT));
        PAPI_CHECK(PAPI_thread_init((unsigned long (*)())(pthread_self)));
        PAPI_CHECK(PAPI_multiplex_init());
    }
    
    context_count.value++;
    
    PTHREAD_CHECK(pthread_mutex_unlock(&context_count.mutex));
}
#endif



#if defined(PAPI_FOUND)
/**
 * Called by cupti_callback() in order to stop (shutdown) PAPI.
 */
static void stop_papi()
{
    /* Atomically decrement the active CUDA context count */
    
    PTHREAD_CHECK(pthread_mutex_lock(&context_count.mutex));
    
#if !defined(NDEBUG)
    if (DebugEnabled)
    {
        printf("[CBTF/CUDA] stop_papi(): "
               "context_count.value = %d --> %d\n",
               context_count.value, context_count.value - 1);
    }
#endif
    
    context_count.value--;

    if (context_count.value == 0)
    {
        /* Shutdown PAPI */
        PAPI_shutdown();
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&context_count.mutex));
}
#endif



/**
 * Callback invoked by CUPTI every time a CUDA event occurs for which we have
 * a subscription. Subscriptions are setup within cbtf_collector_start(), and
 * are setup once for the entire process.
 *
 * @param userdata    User data supplied at subscription of the callback.
 * @param domain      Domain of the callback.
 * @param id          ID of the callback.
 * @param data        Data passed to the callback.
 */
static void cupti_callback(void* userdata,
                           CUpti_CallbackDomain domain,
                           CUpti_CallbackId id,
                           const void* data)
{
    /* Access our thread-local storage */
    TLS* tls = TLS_get();

#if defined(PAPI_FOUND)
    /* Is a CUDA context being created? */
    if ((domain == CUPTI_CB_DOMAIN_RESOURCE) &&
        (id == CUPTI_CBID_RESOURCE_CONTEXT_CREATED))
    {
        /* Start (initialize) PAPI */
        start_papi();

        /* Start PAPI data collection for this thread */
        start_papi_data_collection(tls);
    }
    
    /* Is a CUDA context being destroyed? */
    if ((domain == CUPTI_CB_DOMAIN_RESOURCE) &&
        (id == CUPTI_CBID_RESOURCE_CONTEXT_DESTROY_STARTING))
    {
        /* Stop (shutdown) PAPI */
        stop_papi();
    }
#endif

    /* Do nothing if data collection is paused for this thread */
    if (tls->paused)
    {
        return;
    }

    /* Determine the CUDA event that has occurred and handle it */
    switch (domain)
    {

    case CUPTI_CB_DOMAIN_DRIVER_API:
        {
            const CUpti_CallbackData* const cbdata = (CUpti_CallbackData*)data;
            
            switch (id)
            {



            case CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel :
                if (cbdata->callbackSite == CUPTI_API_ENTER)
                {
                    const cuLaunchKernel_params* const params =
                        (cuLaunchKernel_params*)cbdata->functionParams;

#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] enter cuLaunchKernel()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = TLS_add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = EnqueueRequest;
                    
                    CUDA_EnqueueRequest* message = 
                        &raw_message->CBTF_cuda_message_u.enqueue_request;
                    
                    message->type = LaunchKernel;
                    message->time = CBTF_GetTime();
                    message->context = (CBTF_Protocol_Address)cbdata->context;
                    message->stream = (CBTF_Protocol_Address)params->hStream;
                    message->call_site = TLS_add_current_call_site(tls);
                    
                    TLS_update_header_with_time(tls, message->time);
                }
                break;



            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2D:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2D_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DUnaligned:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DUnaligned_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3D:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3D_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DPeer:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DPeerAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoA:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoA_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoD:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoD_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoH:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoH_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoHAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoHAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoA:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoA_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoD:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoD_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoDAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoDAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoH:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoH_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoHAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoHAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoA:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoA_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoAAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoAAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoD:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoD_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoDAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoDAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyPeer:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyPeerAsync:
                if (cbdata->callbackSite == CUPTI_API_ENTER)
                {
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] enter cuMemcpy*()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = TLS_add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = EnqueueRequest;
                    
                    CUDA_EnqueueRequest* message = 
                        &raw_message->CBTF_cuda_message_u.enqueue_request;
                    
                    message->type = MemoryCopy;
                    message->time = CBTF_GetTime();
                    message->context = (CBTF_Protocol_Address)cbdata->context;
                    message->stream = 0;
                    
                    switch (id)
                    {

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DAsync:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpy2DAsync_params*)
                             cbdata->functionParams)->hStream;
                        break;
                        
                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DAsync_v2:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpy2DAsync_v2_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DAsync:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpy3DAsync_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DAsync_v2:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpy3DAsync_v2_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DPeerAsync:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpy3DPeerAsync_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoHAsync:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpyAtoHAsync_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoHAsync_v2:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpyAtoHAsync_v2_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoDAsync:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpyDtoDAsync_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoDAsync_v2:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpyDtoDAsync_v2_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoHAsync:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpyDtoHAsync_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoHAsync_v2:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpyDtoHAsync_v2_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoAAsync:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpyHtoAAsync_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoAAsync_v2:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpyHtoAAsync_v2_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoDAsync:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpyHtoDAsync_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoDAsync_v2:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpyHtoDAsync_v2_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyPeerAsync:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemcpyPeerAsync_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    default:
                        break;

                    }

                    message->call_site = TLS_add_current_call_site(tls);
                    
                    TLS_update_header_with_time(tls, message->time);
                }
                break;



            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD2D8:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD2D8_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD2D8Async:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD2D16:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD2D16_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD2D16Async:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD2D32:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD2D32_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD2D32Async:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD8:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD8_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD8Async:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD16:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD16_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD16Async:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD32:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD32_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemsetD32Async:
                if (cbdata->callbackSite == CUPTI_API_ENTER)
                {
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] enter cuMemset*()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = TLS_add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = EnqueueRequest;
                    
                    CUDA_EnqueueRequest* message = 
                        &raw_message->CBTF_cuda_message_u.enqueue_request;
                    
                    message->type = MemorySet;
                    message->time = CBTF_GetTime();
                    message->context = (CBTF_Protocol_Address)cbdata->context;
                    message->stream = 0;
                    
                    switch (id)
                    {
                        
                    case CUPTI_DRIVER_TRACE_CBID_cuMemsetD2D8Async:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemsetD2D8Async_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemsetD2D16Async:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemsetD2D16Async_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemsetD2D32Async: 
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemsetD2D32Async_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemsetD8Async:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemsetD8Async_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemsetD16Async:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemsetD16Async_params*)
                             cbdata->functionParams)->hStream;
                        break;

                    case CUPTI_DRIVER_TRACE_CBID_cuMemsetD32Async:
                        message->stream = (CBTF_Protocol_Address)
                            ((cuMemsetD32Async_params*)
                             cbdata->functionParams)->hStream;
                        break;
                        
                    default:
                        break;

                    }

                    message->call_site = TLS_add_current_call_site(tls);
                    
                    TLS_update_header_with_time(tls, message->time);
                }
                break;



            case CUPTI_DRIVER_TRACE_CBID_cuModuleGetFunction :
                if (cbdata->callbackSite == CUPTI_API_EXIT)
                {
                    const cuModuleGetFunction_params* const params =
                        (cuModuleGetFunction_params*)cbdata->functionParams;
                    
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] exit cuModuleGetFunction()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = TLS_add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = ResolvedFunction;
                    
                    CUDA_ResolvedFunction* message = 
                        &raw_message->CBTF_cuda_message_u.resolved_function;
                    
                    message->time = CBTF_GetTime();
                    message->module_handle = 
                        (CBTF_Protocol_Address)params->hmod;
                    message->function = (char*)params->name;
                    message->handle = (CBTF_Protocol_Address)*(params->hfunc);
                    
                    TLS_update_header_with_time(tls, message->time);
                }
                break;



            case CUPTI_DRIVER_TRACE_CBID_cuModuleLoad :
                if (cbdata->callbackSite == CUPTI_API_EXIT)
                {
                    const cuModuleLoad_params* const params =
                        (cuModuleLoad_params*)cbdata->functionParams;
                    
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] exit cuModuleLoad()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = TLS_add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = LoadedModule;
                    
                    CUDA_LoadedModule* message =  
                        &raw_message->CBTF_cuda_message_u.loaded_module;
                    
                    message->time = CBTF_GetTime();
                    message->module.path = (char*)(params->fname);
                    message->module.checksum = 0;
                    message->handle = (CBTF_Protocol_Address)*(params->module);
                    
                    TLS_update_header_with_time(tls, message->time);
                }
                break;



            case CUPTI_DRIVER_TRACE_CBID_cuModuleLoadData :
                if (cbdata->callbackSite == CUPTI_API_EXIT)
                {
                    const cuModuleLoadData_params* const params =
                        (cuModuleLoadData_params*)cbdata->functionParams;
                    
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] exit cuModuleLoadData()\n");
                    }
#endif
                        
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = TLS_add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = LoadedModule;
                    
                    CUDA_LoadedModule* message =  
                        &raw_message->CBTF_cuda_message_u.loaded_module;
                    
                    static char* const kModulePath = 
                        "<Module from Embedded Data>";
                    
                    message->time = CBTF_GetTime();
                    message->module.path = kModulePath;
                    message->module.checksum = 0;
                    message->handle = (CBTF_Protocol_Address)*(params->module);
                        
                    TLS_update_header_with_time(tls, message->time);
                }
                break;
                
                
                
            case CUPTI_DRIVER_TRACE_CBID_cuModuleLoadDataEx :
                if (cbdata->callbackSite == CUPTI_API_EXIT)
                {
                    const cuModuleLoadDataEx_params* const params =
                        (cuModuleLoadDataEx_params*)cbdata->functionParams;
                    
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] exit cuModuleLoadDataEx()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = TLS_add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = LoadedModule;
                    
                    CUDA_LoadedModule* message =  
                        &raw_message->CBTF_cuda_message_u.loaded_module;
                    
                    static char* const kModulePath = 
                        "<Module from Embedded Data>";
  
                    message->time = CBTF_GetTime();
                    message->module.path = kModulePath;
                    message->module.checksum = 0;
                    message->handle = (CBTF_Protocol_Address)*(params->module);
                        
                    TLS_update_header_with_time(tls, message->time);
                }
                break;
                
                

            case CUPTI_DRIVER_TRACE_CBID_cuModuleLoadFatBinary :
                if (cbdata->callbackSite == CUPTI_API_EXIT)
                {
                    const cuModuleLoadFatBinary_params* const params =
                        (cuModuleLoadFatBinary_params*)cbdata->functionParams;
                    
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] exit cuModuleLoadFatBinary()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = TLS_add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = LoadedModule;
                    
                    CUDA_LoadedModule* message =  
                        &raw_message->CBTF_cuda_message_u.loaded_module;
                        
                    static char* const kModulePath = "<Module from Fat Binary>";
                        
                    message->time = CBTF_GetTime();
                    message->module.path = kModulePath;
                    message->module.checksum = 0;
                    message->handle = (CBTF_Protocol_Address)*(params->module);
                        
                    TLS_update_header_with_time(tls, message->time);
                }
                break;
                
                
                
            case CUPTI_DRIVER_TRACE_CBID_cuModuleUnload :
                {
                    if (cbdata->callbackSite == CUPTI_API_EXIT)
                    {
                        const cuModuleUnload_params* const params =
                            (cuModuleUnload_params*)cbdata->functionParams;
                        
#if !defined(NDEBUG)
                        if (DebugEnabled)
                        {
                            printf("[CBTF/CUDA] exit cuModuleUnload()\n");
                        }
#endif
                        
                        /* Add a message for this event */
                        
                        CBTF_cuda_message* raw_message = TLS_add_message(tls);
                        Assert(raw_message != NULL);
                        raw_message->type = UnloadedModule;
                        
                        CUDA_UnloadedModule* message =  
                            &raw_message->CBTF_cuda_message_u.unloaded_module;
                        
                        message->time = CBTF_GetTime();
                        message->handle = (CBTF_Protocol_Address)params->hmod;
                        
                        TLS_update_header_with_time(tls, message->time);
                    }
                }
                break;
                


            }
        }
        break;

    case CUPTI_CB_DOMAIN_RESOURCE:
        {
            const CUpti_ResourceData* const rdata = (CUpti_ResourceData*)data;

            switch (id)
            {



            case CUPTI_CBID_RESOURCE_CONTEXT_CREATED:
                {
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] created context %p\n",
                               rdata->context);
                    }
#endif

#if (CUPTI_API_VERSION < 4)
                    /* Enqueue a buffer for context-specific activities */
                    CUPTI_CHECK(cuptiActivityEnqueueBuffer(
                                    rdata->context, 0,
                                    memalign(ACTIVITY_RECORD_ALIGNMENT,
                                             CUPTI_ACTIVITY_BUFFER_SIZE),
                                    CUPTI_ACTIVITY_BUFFER_SIZE
                                    ));
#elif (CUPTI_API_VERSION >= 5)
                    uint32_t context_id = 0;
                    CUPTI_CHECK(cuptiGetContextId(rdata->context, &context_id));

                    /* Add the context ID to pointer mapping */
                    CUPTI_context_id_to_ptr(context_id, rdata->context);
#endif
                }
                break;



            case CUPTI_CBID_RESOURCE_CONTEXT_DESTROY_STARTING:
                {
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] destroying context %p\n",
                               rdata->context);
                    }
#endif

#if (CUPTI_API_VERSION < 4)
                    /* Add messages for this context's activities */
                    wrap_add_activities(tls, rdata->context, NULL, 0);
                    
                    /* Add messages for global activities */
                    wrap_add_activities(tls, NULL, NULL, 0);
#endif
                }
                break;



            case CUPTI_CBID_RESOURCE_STREAM_CREATED:
                {
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] created stream %p in context %p\n",
                               rdata->resourceHandle.stream, rdata->context);
                    }
#endif

                    uint32_t stream_id = 0;
                    CUPTI_CHECK(cuptiGetStreamId(rdata->context,
                                                 rdata->resourceHandle.stream,
                                                 &stream_id));
                                        
#if (CUPTI_API_VERSION < 4)
                    /* Enqueue a buffer for stream-specific activities */
                    CUPTI_CHECK(cuptiActivityEnqueueBuffer(
                                    rdata->context, stream_id,
                                    memalign(ACTIVITY_RECORD_ALIGNMENT,
                                             CUPTI_ACTIVITY_BUFFER_SIZE),
                                    CUPTI_ACTIVITY_BUFFER_SIZE
                                    ));
#else
                    /* Add the stream ID to pointer mapping */
                    CUPTI_stream_id_to_ptr(stream_id,
                                           rdata->resourceHandle.stream);
#endif
                }
                break;
                


            case CUPTI_CBID_RESOURCE_STREAM_DESTROY_STARTING:
                {
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] "
                               "destroying stream %p in context %p\n",
                               rdata->resourceHandle.stream, rdata->context);
                    }
#endif

#if (CUPTI_API_VERSION < 4)
                    uint32_t stream_id = 0;
                    CUPTI_CHECK(cuptiGetStreamId(rdata->context,
                                                 rdata->resourceHandle.stream,
                                                 &stream_id));

                    /* Add messages for this stream's activities */
                    wrap_add_activities(tls, rdata->context,
                                        rdata->resourceHandle.stream,
                                        stream_id);
                    
                    /* Add messages for global activities */
                    wrap_add_activities(tls, NULL, NULL, 0);
#endif
                }
                break;
                


            }
        }
        break;



    case CUPTI_CB_DOMAIN_SYNCHRONIZE:
        {
            const CUpti_SynchronizeData* const sdata = 
                (CUpti_SynchronizeData*)data;

            switch (id)
            {



            case CUPTI_CBID_SYNCHRONIZE_CONTEXT_SYNCHRONIZED:
                {
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] synchronized context %p\n",
                               sdata->context);
                    }
#endif

#if (CUPTI_API_VERSION < 4)
                    /* Add messages for this context's activities */
                    wrap_add_activities(tls, sdata->context, NULL, 0);

                    /* Add messages for global activities */
                    wrap_add_activities(tls, NULL, NULL, 0);
#endif
                }
                break;



            case CUPTI_CBID_SYNCHRONIZE_STREAM_SYNCHRONIZED:
                {
                    uint32_t stream_id = 0;
                    CUPTI_CHECK(cuptiGetStreamId(sdata->context,
                                                 sdata->stream,
                                                 &stream_id));
                    
#if !defined(NDEBUG)
                    if (DebugEnabled)
                    {
                        printf("[CBTF/CUDA] "
                               "synchronized stream %p in context %p\n",
                               sdata->stream, sdata->context);
                    }
#endif
                    
#if (CUPTI_API_VERSION < 4)
                    /* Add messages for this stream's activities */
                    wrap_add_activities(tls, sdata->context,
                                        sdata->stream, stream_id);

                    /* Add messages for global activities */
                    wrap_add_activities(tls, NULL, NULL, 0);
#endif
                }
                break;



            }
        }
        break;
        
        
        
    default:
        break;        
    }
}



#if defined(PAPI_FOUND)
/**
 * Parse the configuration string that was passed into this collector.
 *
 * @param configuration    Configuration string passed into this collector.
 */
static void parse_configuration(const char* const configuration)
{
    static const char* const kIntervalPrefix = "interval=";

#if !defined(NDEBUG)
    if (DebugEnabled)
    {
        printf("[CBTF/CUDA] parse_configuration(\"%s\")\n", configuration);
    }
#endif

    /* Initialize the event sampling configuration */

    sampling_config.interval = 10 * 1000 * 1000 /* 10 mS */;
    sampling_config.events.events_len = 0;
    sampling_config.events.events_val = event_descriptions;

    memset(event_descriptions, 0, MAX_EVENTS * sizeof(CUDA_EventDescription));

    /* Copy the configuration string for parsing */
    
    static char copy[4 * 1024 /* 4 KB */];
    
    if (strlen(configuration) >= sizeof(copy))
    {
        fprintf(stderr, "[CBTF/CUDA] parse_configuration(): "
                "Configuration string \"%s\" exceeds the maximum "
                "support length (%llu)!\n",
                configuration, (unsigned long long)(sizeof(copy) - 1));
        fflush(stderr);
        abort();
    }
    
    strcpy(copy, configuration);

    /* Split the configuration string into tokens at each comma */
    char* ptr = NULL;
    for (ptr = strtok(copy, ","); ptr != NULL; ptr = strtok(NULL, ","))
    {
        /*
         * Parse this token into a sampling interval, an event name, or an
         * event name and threshold, depending on whether the token contains
         * an at ('@') character or starts with the string "interval=".
         */
        
        char* interval = strstr(ptr, kIntervalPrefix);
        char* at = strchr(ptr, '@');
        
        /* Token is a sampling interval */
        if (interval != NULL)
        {
            int64_t interval = atoll(ptr + strlen(kIntervalPrefix));

            /* Warn if the samping interval was invalid */
            if (interval <= 0)
            {
                fprintf(stderr, "[CBTF/CUDA] parse_configuration(): "
                        "An invalid sampling interval (\"%s\") was "
                        "specified!\n", ptr + strlen(kIntervalPrefix));
                fflush(stderr);
                abort();
            }
            
            sampling_config.interval = (uint64_t)interval;

#if !defined(NDEBUG)
            if (DebugEnabled)
            {
                printf("[CBTF/CUDA] parse_configuration(): "
                       "sampling interval = %llu nS\n",
                       (long long unsigned)sampling_config.interval);
            }
#endif
        }
        else
        {
            /* Warn if the maximum supported sampled events was reached */
            if (sampling_config.events.events_len == MAX_EVENTS)
            {
                fprintf(stderr, "[CBTF/CUDA] parse_configuration(): "
                        "Maximum supported number of concurrently sampled"
                        "events (%d) was reached!\n", MAX_EVENTS);
                fflush(stderr);
                abort();
            }
            
            CUDA_EventDescription* event = &sampling_config.events.events_val[
                sampling_config.events.events_len
                ];
            
            event->name = malloc(sizeof(copy));
            
            /* Token is an event name and threshold */
            if (at != NULL)
            {
                strncpy(event->name, ptr, at - ptr);
                event->threshold = atoi(at + 1);
                
#if !defined(NDEBUG)
                if (DebugEnabled)
                {
                    printf("[CBTF/CUDA] parse_configuration(): "
                           "event name = \"%s\", threshold = %d\n",
                           event->name, event->threshold);
                }
#endif
            }

            /* Token is an event name */
            else
            {
                strcpy(event->name, ptr);
            
#if !defined(NDEBUG)
                if (DebugEnabled)
                {
                    printf("[CBTF/CUDA] parse_configuration(): "
                           "event name = \"%s\"\n", event->name);
                }
#endif
            }

            /* Increment the number of overflow sampled events */
            if (event->threshold > 0)
            {
                ++OverflowSamplingCount;
            }
            
            /* Increment the number of sampled events */
            ++sampling_config.events.events_len;
        }
    }
}
#endif



/**
 * Called by the CBTF collector service in order to start data collection.
 */
void cbtf_collector_start(const CBTF_DataHeader* const header)
{
#if !defined(NDEBUG)
    if (DebugEnabled)
    {
        printf("[CBTF/CUDA] cbtf_collector_start()\n");
    }
#endif

    /* Atomically increment the active thread count */

    PTHREAD_CHECK(pthread_mutex_lock(&thread_count.mutex));
    
#if !defined(NDEBUG)
    if (DebugEnabled)
    {
        printf("[CBTF/CUDA] cbtf_collector_start(): "
               "thread_count.value = %d --> %d\n",
               thread_count.value, thread_count.value + 1);
    }
#endif
    
    if (thread_count.value == 0)
    {
        /* Should debugging be enabled? */
        DebugEnabled = (getenv("CBTF_DEBUG_COLLECTOR") != NULL);
        
#if !defined(NDEBUG)
        if (DebugEnabled)
        {
            printf("[CBTF/CUDA] cbtf_collector_start()\n");
            printf("[CBTF/CUDA] cbtf_collector_start(): "
                   "thread_count.value = %d --> %d\n",
                   thread_count.value, thread_count.value + 1);
        }
#endif
        
#if defined(PAPI_FOUND)
        /* Obtain our configuration string from the environment and parse it */
        const char* const configuration = getenv("CBTF_CUDA_CONFIG");
        if (configuration != NULL)
        {
            parse_configuration(configuration);
        }
#endif
        
#if (CUPTI_API_VERSION < 4)
        /* Enqueue a buffer for CUPTI global activities */
        CUPTI_CHECK(cuptiActivityEnqueueBuffer(
                        NULL, 0,
                        memalign(ACTIVITY_RECORD_ALIGNMENT,
                                 CUPTI_ACTIVITY_BUFFER_SIZE),
                        CUPTI_ACTIVITY_BUFFER_SIZE
                        ));
#else
        /* Register callbacks with CUPTI for activity buffer handling */
        CUPTI_CHECK(cuptiActivityRegisterCallbacks(allocate_activities,
                                                   wrap_add_activities));
#endif
        
        /* Enable the CUPTI activities of interest */
        CUPTI_CHECK(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONTEXT));
        CUPTI_CHECK(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_DEVICE));
        CUPTI_CHECK(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMCPY));
        CUPTI_CHECK(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMSET));
        CUPTI_CHECK(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_KERNEL));

        /* Subscribe to the CUPTI callbacks of interest */

        CUPTI_CHECK(cuptiSubscribe(&cupti_subscriber_handle,
                                   cupti_callback, NULL));
        
        CUPTI_CHECK(cuptiEnableDomain(1, cupti_subscriber_handle,
                                      CUPTI_CB_DOMAIN_DRIVER_API));
        
        CUPTI_CHECK(cuptiEnableDomain(1, cupti_subscriber_handle,
                                      CUPTI_CB_DOMAIN_RESOURCE));
        
        CUPTI_CHECK(cuptiEnableDomain(1, cupti_subscriber_handle,
                                      CUPTI_CB_DOMAIN_SYNCHRONIZE));
        
        /*
         * Estimate the offset that must be added to all CUPTI-provided time
         * values in order to translate them to the same time "origin" provided
         * by CBTF_GetTime().
         */
        
        uint64_t cbtf_now = CBTF_GetTime();
        
        uint64_t cupti_now = 0;
        CUPTI_CHECK(cuptiGetTimestamp(&cupti_now));
        
        if (cbtf_now > cupti_now)
        {
            cupti_time_offset = cbtf_now - cupti_now;
        }
        else if (cbtf_now < cupti_now)
        {
            cupti_time_offset = -(cupti_now - cbtf_now);
        }
        else
        {
            cupti_time_offset = 0;
        }
    }

    thread_count.value++;

    PTHREAD_CHECK(pthread_mutex_unlock(&thread_count.mutex));

    /* Create and zero-initialize our thread-local storage */
    TLS_initialize();
    
    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Copy the header into our thread-local storage for future use */
    memcpy(&tls->data_header, header, sizeof(CBTF_DataHeader));

    /* Initialize our performance data header and blob */
    TLS_initialize_data(tls);

#if defined(PAPI_FOUND)
    /* Append the event sampling configuration to our performance data blob */
    if (sampling_config.events.events_len > 0)
    {
        CBTF_cuda_message* raw_message = TLS_add_message(tls);
        Assert(raw_message != NULL);
        raw_message->type = SamplingConfig;
        
        CUDA_SamplingConfig* message = 
            &raw_message->CBTF_cuda_message_u.sampling_config;
        
        memcpy(message, &sampling_config, sizeof(CUDA_SamplingConfig));
    }
#endif

    /* Resume (start) data collection for this thread */
    tls->paused = FALSE;

#if defined(PAPI_FOUND)
    /* Start PAPI data collection for this thread */
    start_papi_data_collection(tls);
#endif
}



/**
 * Called by the CBTF collector service in order to pause data collection.
 */
void cbtf_collector_pause()
{
#if !defined(NDEBUG)
    if (DebugEnabled)
    {
        printf("[CBTF/CUDA] cbtf_collector_pause()\n");
    }
#endif
    
    /* Access our thread-local storage */
    TLS* tls = TLS_get();

#if defined(PAPI_FOUND)
    /* Stop PAPI data collection for this thread */
    stop_papi_data_collection(tls);
#endif

    /* Pause data collection for this thread */
    tls->paused = TRUE;
}



/**
 * Called by the CBTF collector service in order to resume data collection.
 */
void cbtf_collector_resume()
{
#if !defined(NDEBUG)
    if (DebugEnabled)
    {
        printf("[CBTF/CUDA] cbtf_collector_resume()\n");
    }
#endif

    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Resume data collection for this thread */
    tls->paused = FALSE;

#if defined(PAPI_FOUND)
    /* Start PAPI data collection for this thread */
    start_papi_data_collection(tls);
#endif
}



/**
 * Called by the CBTF collector service in order to stop data collection.
 */
void cbtf_collector_stop()
{
#if !defined(NDEBUG)
    if (DebugEnabled)
    {
        printf("[CBTF/CUDA] cbtf_collector_stop()\n");
    }
#endif
    
    /* Access our thread-local storage */
    TLS* tls = TLS_get();

#if defined(PAPI_FOUND)
    /* Stop PAPI data collection for this thread */
    stop_papi_data_collection(tls);
#endif

#if (CUPTI_API_VERSION == 4)
    /* Wait until CUPTI flushes all activity buffers */
    CUPTI_CHECK(cuptiActivityFlushAll(0));
#elif (CUPTI_API_VERSION >= 5)
    /* Wait until CUPTI flushes all activity buffers */
    CUPTI_CHECK(cuptiActivityFlushAll(CUPTI_ACTIVITY_FLAG_FLUSH_FORCED));
#endif

    /* Pause (stop) data collection for this thread */
    tls->paused = TRUE;
    
    /* Send any remaining performance data for this thread */
    TLS_send_data(tls);
    
    /* Destroy our thread-local storage */
    TLS_destroy();

    /* Atomically decrement the active thread count */
    
    PTHREAD_CHECK(pthread_mutex_lock(&thread_count.mutex));
    
#if !defined(NDEBUG)
    if (DebugEnabled)
    {
        printf("[CBTF/CUDA] cbtf_collector_stop(): "
               "thread_count.value = %d --> %d\n",
               thread_count.value, thread_count.value - 1);
    }
#endif
    
    thread_count.value--;

    if (thread_count.value == 0)
    {
        /* Disable all CUPTI activities */
        CUPTI_CHECK(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_CONTEXT));
        CUPTI_CHECK(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_DEVICE));
        CUPTI_CHECK(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_MEMCPY));
        CUPTI_CHECK(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_MEMSET));
        CUPTI_CHECK(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_KERNEL));
        
        /* Unsubscribe from all CUPTI callbacks */
        CUPTI_CHECK(cuptiUnsubscribe(cupti_subscriber_handle));
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&thread_count.mutex));
}
