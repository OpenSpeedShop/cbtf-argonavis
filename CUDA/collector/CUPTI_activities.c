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

/** @file Definition of CUPTI activity functions. */

#include <malloc.h>
#include <stdio.h>

#include <KrellInstitute/Messages/CUDA_data.h>

#include <KrellInstitute/Services/Assert.h>

#include "collector.h"
#include "CUPTI_activities.h"
#include "CUPTI_check.h"
#include "CUPTI_context.h"
#include "CUPTI_stream.h"
#include "CUPTI_time.h"



/**
 * Size (in bytes) of each allocated CUPTI activity buffer.
 *
 * @note    Currently the only basis for the selection of this value is that the
 *          CUPTI "activity_trace_async.cpp" example uses buffers of 32 KB each.
 */
#define BUFFER_SIZE (32 * 1024 /* 32 KB */)



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
 * Add the given context activity to the performance data blob contained
 * within the specified thread-local storage.
 *
 * @param tls             Thread-local storage to which the activity
 *                        is to be added.
 * @param raw_activity    Activity record for the activity to be added.
 */
static void add_context(TLS* tls, const CUpti_Activity* const raw_activity)
{
    const CUpti_ActivityContext* const activity =
        (const CUpti_ActivityContext*)raw_activity;
    
    CBTF_cuda_message* raw_message = TLS_add_message(tls);
    Assert(raw_message != NULL);
    raw_message->type = ContextInfo;
    
    CUDA_ContextInfo* message = &raw_message->CBTF_cuda_message_u.context_info;
    
    message->context = (CBTF_Protocol_Address)
        CUPTI_context_ptr_from_id(activity->contextId);
    
    message->device = activity->deviceId;
}



/**
 * Add the given device activity to the performance data blob contained
 * within the specified thread-local storage.
 *
 * @param tls             Thread-local storage to which the activity
 *                        is to be added.
 * @param raw_activity    Activity record for the activity to be added.
 */
static void add_device(TLS* tls, const CUpti_Activity* const raw_activity)
{
#if (CUPTI_API_VERSION < 8)
    const CUpti_ActivityDevice* const activity =
        (const CUpti_ActivityDevice*)raw_activity;
#else
    const CUpti_ActivityDevice2* const activity =
        (const CUpti_ActivityDevice2*)raw_activity;
#endif

    CBTF_cuda_message* raw_message = TLS_add_message(tls);
    Assert(raw_message != NULL);
    raw_message->type = DeviceInfo;
    
    CUDA_DeviceInfo* message = &raw_message->CBTF_cuda_message_u.device_info;

    message->device = activity->id;

    message->name = (char*)activity->name;

    message->compute_capability[0] = activity->computeCapabilityMajor;
    message->compute_capability[1] = activity->computeCapabilityMinor;
    
    message->max_grid[0] = activity->maxGridDimX;
    message->max_grid[1] = activity->maxGridDimY;
    message->max_grid[2] = activity->maxGridDimZ;
    
    message->max_block[0] = activity->maxBlockDimX;
    message->max_block[1] = activity->maxBlockDimY;
    message->max_block[2] = activity->maxBlockDimZ;
    
    message->global_memory_bandwidth = activity->globalMemoryBandwidth;    
    message->global_memory_size = activity->globalMemorySize;
    message->constant_memory_size = activity->constantMemorySize;
    message->l2_cache_size = activity->l2CacheSize;
    message->threads_per_warp = activity->numThreadsPerWarp;
    message->core_clock_rate = activity->coreClockRate;
    message->memcpy_engines = activity->numMemcpyEngines;
    message->multiprocessors = activity->numMultiprocessors;
    message->max_ipc = activity->maxIPC;
    message->max_warps_per_multiprocessor = activity->maxWarpsPerMultiprocessor;
    message->max_blocks_per_multiprocessor =
        activity->maxBlocksPerMultiprocessor;
    message->max_registers_per_block = activity->maxRegistersPerBlock;
    message->max_shared_memory_per_block = activity->maxSharedMemoryPerBlock;
    message->max_threads_per_block = activity->maxThreadsPerBlock;
}



/**
 * Add the given kernel activity to the performance data blob contained
 * within the specified thread-local storage.
 *
 * @param tls             Thread-local storage to which the activity
 *                        is to be added.
 * @param raw_activity    Activity record for the activity to be added.
 */
static void add_kernel(TLS* tls, const CUpti_Activity* const raw_activity)
{
#if (CUPTI_API_VERSION < 4)
    const CUpti_ActivityKernel* const activity =
        (const CUpti_ActivityKernel*)raw_activity;
#elif (CUPTI_API_VERSION < 8)
    const CUpti_ActivityKernel2* const activity =
        (const CUpti_ActivityKernel2*)raw_activity;
#else
    const CUpti_ActivityKernel3* const activity =
        (const CUpti_ActivityKernel3*)raw_activity;
#endif

    CBTF_cuda_message* raw_message = TLS_add_message(tls);
    Assert(raw_message != NULL);
    raw_message->type = CompletedExec;
    
    CUDA_CompletedExec* message =
        &raw_message->CBTF_cuda_message_u.completed_exec;

    message->id = activity->correlationId;
 
    message->time_begin = activity->start + TimeOffset;
    message->time_end = activity->end + TimeOffset;

    message->function = (char*)activity->name;
    
    message->grid[0] = activity->gridX;
    message->grid[1] = activity->gridY;
    message->grid[2] = activity->gridZ;
    
    message->block[0] = activity->blockX;
    message->block[1] = activity->blockY;
    message->block[2] = activity->blockZ;
    
#if (CUPTI_API_VERSION < 4)
    message->cache_preference = 
        toCachePreference(activity->cacheConfigExecuted);
#else
    message->cache_preference = 
        toCachePreference(activity->cacheConfig.config.executed);    
#endif
    
    message->registers_per_thread = activity->registersPerThread;
    message->static_shared_memory = activity->staticSharedMemory;
    message->dynamic_shared_memory = activity->dynamicSharedMemory;
    message->local_memory = activity->localMemoryTotal;
    
    TLS_update_header_with_time(tls, message->time_begin);
    TLS_update_header_with_time(tls, message->time_end);
}



/**
 * Add the given memcpy activity to the performance data blob contained
 * within the specified thread-local storage.
 *
 * @param tls             Thread-local storage to which the activity
 *                        is to be added.
 * @param raw_activity    Activity record for the activity to be added.
 */
static void add_memcpy(TLS* tls, const CUpti_Activity* const raw_activity)
{
    const CUpti_ActivityMemcpy* const activity =
        (const CUpti_ActivityMemcpy*)raw_activity;
    
    CBTF_cuda_message* raw_message = TLS_add_message(tls);
    Assert(raw_message != NULL);
    raw_message->type = CompletedXfer;
    
    CUDA_CompletedXfer* message =
        &raw_message->CBTF_cuda_message_u.completed_xfer;

    message->id = activity->correlationId;

    message->time_begin = activity->start + TimeOffset;
    message->time_end = activity->end + TimeOffset;

    message->size = activity->bytes;
    
    message->kind = toCopyKind(activity->copyKind);
    message->source_kind = toMemoryKind(activity->srcKind);
    message->destination_kind = toMemoryKind(activity->dstKind);
    
    message->asynchronous = 
        (activity->flags & CUPTI_ACTIVITY_FLAG_MEMCPY_ASYNC) ? true : false;
    
    TLS_update_header_with_time(tls, message->time_begin);
    TLS_update_header_with_time(tls, message->time_end);
}
 
 
 
/**
 * Add the activities for the specified CUDA context/stream to the performance
 * data blob contained within the given thread-local storage.
 *
 * @param tls          Thread-local storage to which activities are to be added.
 * @param context      CUDA context for the activities to be added.
 * @param stream_id    CUDA stream ID for the activities to be added.
 * @param buffer       Buffer containing the activity records.
 * @param size         Actual size of the buffer.
 */
static void add(TLS* tls, CUcontext context, uint32_t stream_id,
                uint8_t* buffer, size_t size)
{
    Assert(tls != NULL);
    
    /* Warn if any activity records were dropped */
    size_t dropped = 0;
    CUPTI_CHECK(cuptiActivityGetNumDroppedRecords(context, stream_id,
                                                  &dropped));
    if (dropped > 0)
    {
        fprintf(stderr, "[CBTF/CUDA] "
                "dropped %u activity records for stream ID %u in context %p\n",
                (unsigned int)dropped, stream_id, context);
        fflush(stderr);
    }
    
    /* Iterate over each activity record */
    
    CUpti_Activity* raw_activity = NULL;

    size_t total, ignored;
    for (total = 0, ignored = 0; true; ++total)
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
            add_context(tls, raw_activity);
            break;
            
        case CUPTI_ACTIVITY_KIND_DEVICE:
            add_device(tls, raw_activity);
            break;
            
        case CUPTI_ACTIVITY_KIND_KERNEL:
            add_kernel(tls, raw_activity);
            break;
            
        case CUPTI_ACTIVITY_KIND_MEMCPY:
            add_memcpy(tls, raw_activity);
            break;
            
        default:
            ignored++;
            break;
        }
    }
    
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] "
               "added %u activity records for stream ID %u in context %p\n",
               (unsigned int)(total - ignored), stream_id, context);
        printf("[CBTF/CUDA] "
               "ignored %u activity records for stream ID %u in context %p\n",
               (unsigned int)ignored, stream_id, context);
    }
#endif
}



#if (CUPTI_API_VERSION >= 4)
/**
 * Callback invoked by CUPTI (API versions 4 and above) each time it requires
 * a new activity buffer to be allocated.
 *
 * @retval buffer         Buffer to contain activity records.
 * @retval allocated      Allocated size of the buffer.
 * @retval max_records    Maximum number of records to put in this buffer.
 */
static void allocate(uint8_t** buffer, size_t* allocated, size_t* max_records)
{
    *buffer = memalign(ACTIVITY_RECORD_ALIGNMENT, BUFFER_SIZE);
    *allocated = BUFFER_SIZE;
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
static void callback(CUcontext context, uint32_t stream_id,
                     uint8_t* buffer, size_t allocated, size_t size)
{
    /* Access our thread-local storage */
    TLS* tls = TLS_get();
    
    /* Actually add these activities */
    add(tls, context, stream_id, buffer, size);
}
#endif



/**
 * Start CUPTI activity data collection for this process.
 */
void CUPTI_activities_start()
{
#if (CUPTI_API_VERSION < 4)
    /* Enqueue a buffer for CUPTI global activities */
    CUPTI_CHECK(cuptiActivityEnqueueBuffer(
                    NULL, 0,
                    memalign(ACTIVITY_RECORD_ALIGNMENT, BUFFER_SIZE),
                    BUFFER_SIZE
                    ));
#else
    /* Register callbacks with CUPTI for activity buffer handling */
    CUPTI_CHECK(cuptiActivityRegisterCallbacks(allocate, callback));
#endif
    
    /* Enable the CUPTI activities of interest */
    CUPTI_CHECK(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_CONTEXT));
    CUPTI_CHECK(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_DEVICE));
    CUPTI_CHECK(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_MEMCPY));
    CUPTI_CHECK(cuptiActivityEnable(CUPTI_ACTIVITY_KIND_KERNEL));
}



/**
 * Add the CUPTI activity data for the specified CUDA context/stream to the
 * performance data blob contained within the given thread-local storage.
 *
 * @param tls        Thread-local storage to which activities are to be added.
 * @param context    CUDA context for the activities to be added.
 * @param stream     CUDA stream for the activities to be added.
 */
void CUPTI_activities_add(TLS* tls, CUcontext context, CUstream stream)
{
#if (CUPTI_API_VERSION < 4)
    Assert(tls != NULL);
    
    /* Find the CUPTI stream ID corresponding to this CUDA stream pointer */
    uint32_t stream_id = 0;
    
    if (stream != NULL)
    {
        CUPTI_CHECK(cuptiGetStreamId(context, stream, &stream_id));
    }
    
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
    add(tls, context, stream_id, buffer, size);
    
    /* Re-enqueue this buffer of activities */
    CUPTI_CHECK(cuptiActivityEnqueueBuffer(
                    context, stream_id, buffer, BUFFER_SIZE
                    ));
#endif
}



/**
 * Ensure that all CUPTI activity data for this process has been flushed.
 */
void CUPTI_activities_flush()
{
#if (CUPTI_API_VERSION == 4)
    /* Wait until CUPTI flushes all activity buffers */
    CUPTI_CHECK(cuptiActivityFlushAll(0));
#elif (CUPTI_API_VERSION >= 5)
    /* Wait until CUPTI flushes all activity buffers */
    CUPTI_CHECK(cuptiActivityFlushAll(CUPTI_ACTIVITY_FLAG_FLUSH_FORCED));
#endif
}



/**
 * Stop CUPTI activity data collection for this process.
 */
void CUPTI_activities_stop()
{
    /* Disable the CUPTI activities of interest */
    CUPTI_CHECK(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_CONTEXT));
    CUPTI_CHECK(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_DEVICE));
    CUPTI_CHECK(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_MEMCPY));
    CUPTI_CHECK(cuptiActivityDisable(CUPTI_ACTIVITY_KIND_KERNEL));
}
