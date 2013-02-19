/*******************************************************************************
** Copyright (c) 2012 Argo Navis Technologies. All Rights Reserved.
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

/** @file Specification of the CUDA collector's blobs. */

%#include "KrellInstitute/Messages/Address.h"
%#include "KrellInstitute/Messages/File.h"
%#include "KrellInstitute/Messages/Thread.h"
%#include "KrellInstitute/Messages/Time.h"



/**
 * Message containing information about a CUDA context.
 */
struct CUDA_ContextInfo
{
    /** CUDA context for which this is information. */
    CBTF_Protocol_Address context;

    /** CUDA device for this context. */
    uint32_t device;

    /** Compute API (CUDA or OpenCL) being used in this context. */
    string compute_api<>;
};



/**
 * Enumeration of the different memory copy kinds.
 */
enum CUDA_CopyKind
{
    InvalidCopyKind = 0,
    UnknownCopyKind = 1,
    HostToDevice = 2,
    DeviceToHost = 3,
    HostToArray = 4,
    ArrayToHost = 5,
    ArrayToArray = 6,
    ArrayToDevice = 7,
    DeviceToArray = 8,
    DeviceToDevice = 9,
    HostToHost = 10
};

/**
 * Enumeration of the different memory kinds.
 */
enum CUDA_MemoryKind
{     
    InvalidMemoryKind = 0,
    UnknownMemoryKind = 1,
    Pageable = 2,
    Pinned = 3,
    Device = 4,
    Array = 5
};

/**
 * Message emitted when the CUDA driver copied memory.
 */
struct CUDA_CopiedMemory
{
    /** CUDA context for which the request was enqueued. */
    CBTF_Protocol_Address context;

    /** CUDA stream for which memory was copied. */
    CBTF_Protocol_Address stream;

    /** Time at which the memory copy began. */
    CBTF_Protocol_Time time_begin;
    
    /** Time at which the memory copy ended. */
    CBTF_Protocol_Time time_end;
    
    /** Number of bytes being copied. */
    uint64_t size;

    /** Kind of copy performed. */
    CUDA_CopyKind kind;

    /** Kind of memory from which the copy was performed. */
    CUDA_MemoryKind source_kind;

    /** Kind of memory to which the copy was performed. */
    CUDA_MemoryKind destination_kind;

    /** Was the copy asynchronous? */
    bool asynchronous;  
};



/**
 * Message containing information about a CUDA device.
 */
struct CUDA_DeviceInfo
{
    /** CUDA device for which this is information. */
    uint32_t device;

    /** Name of this device. */
    string name<>;

    /** Compute capability (major/minor) of this device. */
    uint32_t compute_capability[2];

    /** Maximum allowed dimensions of grids with this device. */
    uint32_t max_grid[3];

    /** Maximum allowed dimensions of blocks with this device. */
    uint32_t max_block[3];

    /** Global memory bandwidth of this device (in KBytes/sec). */
    uint64_t global_memory_bandwidth;
    
    /** Global memory size of this device (in bytes). */
    uint64_t global_memory_size;

    /** Constant memory size of this device (in bytes). */
    uint32_t constant_memory_size;

    /** L2 cache size of this device (in bytes). */
    uint32_t l2_cache_size;

    /** Number of threads per warp for this device. */
    uint32_t threads_per_warp;

    /** Core clock rate of this device (in KHz). */
    uint32_t core_clock_rate;

    /** Number of memory copy engines on this device. */
    uint32_t memcpy_engines;

    /** Number of multiprocessors on this device. */
    uint32_t multiprocessors;

    /** Maximum instructions/cycle possible on this device's multiprocessors. */
    uint32_t max_ipc;

    /** Maximum warps/multiprocessor for this device. */
    uint32_t max_warps_per_multiprocessor;

    /** Maximum blocks/multiprocessor for this device. */
    uint32_t max_blocks_per_multiprocessor;

    /** Maximum registers/block for this device. */
    uint32_t max_registers_per_block;
    
    /** Maximium shared memory / block for this device. */
    uint32_t max_shared_memory_per_block;

    /** Maximum threads/block for this device. */
    uint32_t max_threads_per_block;
};



/**
 * Enumeration of the different request types enqueue by the CUDA driver.
 */
enum CUDA_RequestTypes
{
    LaunchKernel = 0,
    MemoryCopy = 1,
    MemorySet = 2
};

/**
 * Message emitted when the CUDA driver enqueues a request.
 */
struct CUDA_EnqueueRequest
{
    /** Type of request that was enqueued. */
    CUDA_RequestTypes type;

    /** Time at which the request was enqueued. */
    CBTF_Protocol_Time time;

    /** CUDA context for which the request was enqueued. */
    CBTF_Protocol_Address context;

    /** CUDA stream for which the request was enqueued. */
    CBTF_Protocol_Address stream;

    /**
     * Call site of the request. This is an index into the stack_traces array
     * of the CBTF_cuda_data containing this message.
     */
    uint32_t call_site;
};



/**
 * Enumeration of the different cache preferences.
 */
enum CUDA_CachePreference
{
    InvalidCachePreference = 0,
    NoPreference = 1,
    PreferShared = 2,
    PreferCache = 3,
    PreferEqual = 4
};

/**
 * Message emitted when the CUDA driver executed a kernel.
 */
struct CUDA_ExecutedKernel
{
    /** CUDA context for which the request was enqueued. */
    CBTF_Protocol_Address context;

    /** CUDA stream for which a kernel was executed. */
    CBTF_Protocol_Address stream;

    /** Time at which the kernel execution began. */
    CBTF_Protocol_Time time_begin;

    /** Time at which the kernel execution ended. */
    CBTF_Protocol_Time time_end;

    /** Name of the kernel function being executed. */
    string function<>;

    /** Dimensions of the grid. */
    int32_t grid[3];
    
    /** Dimensions of each block. */
    int32_t block[3];

    /** Cache preference used. */
    CUDA_CachePreference cache_preference;

    /** Registers required for each thread. */
    uint16_t registers_per_thread;

    /** Total amount (in bytes) of static shared memory reserved. */
    int32_t static_shared_memory;

    /** Total amount (in bytes) of dynamic shared memory reserved. */
    int32_t dynamic_shared_memory;

    /** Total amount (in bytes) of local memory reserved. */
    int32_t local_memory;
};



/**
 * Message emitted when the CUDA driver loads a module.
 */
struct CUDA_LoadedModule
{
    /** Time at which the module was unloaded. */
    CBTF_Protocol_Time time;

    /** Name of the file containing the module that was loaded. */
    CBTF_Protocol_FileName module;
    
    /** Handle within the CUDA driver of the loaded module .*/
    CBTF_Protocol_Address handle;
};



/**
 * Message emitted when the CUDA driver resolves a function.
 */
struct CUDA_ResolvedFunction
{
    /** Time at which the function was resolved. */
    CBTF_Protocol_Time time;

    /** Handle within the CUDA driver of the module containing the function. */
    CBTF_Protocol_Address module_handle;
    
    /** Name of the function being resolved. */
    string function<>;
    
    /** Handle within the CUDA driver of the resolved function. */
    CBTF_Protocol_Address handle;    
};



/**
 * Message emitted when the CUDA driver set memory.
 */
struct CUDA_SetMemory
{
    /** CUDA context for which the request was enqueued. */
    CBTF_Protocol_Address context;

    /** CUDA stream for which memory was set. */
    CBTF_Protocol_Address stream;

    /** Time at which the memory set began. */
    CBTF_Protocol_Time time_begin;

    /** Time at which the memory set ended. */
    CBTF_Protocol_Time time_end;

    /** Number of bytes being set. */
    uint64_t size;
};



/**
 * Message emitted when the CUDA driver unloads a module.
 */
struct CUDA_UnloadedModule
{
    /** Time at which the module was unloaded. */
    CBTF_Protocol_Time time;

    /** Handle within the CUDA driver of the unloaded module .*/
    CBTF_Protocol_Address handle;
};



/**
 * Enumeration of the different types of messages that are encapsulated within
 * this collector's blobs. See the note on CBTF_cuda_data for more information.
 */
enum CUDA_MessageTypes
{
    ContextInfo = 0,
    CopiedMemory = 1,
    DeviceInfo = 2,
    EnqueueRequest = 3,
    ExecutedKernel = 4,
    LoadedModule = 5,
    ResolvedFunction = 6,
    SetMemory = 7,
    UnloadedModule = 8
};



/**
 * Union of the different types of messages that are encapsulated within this
 * collector's blobs. See the note on CBTF_cuda_data for more information.
 */
union CBTF_cuda_message switch (unsigned type)
{
    case      ContextInfo:      CUDA_ContextInfo context_info;
    case     CopiedMemory:     CUDA_CopiedMemory copied_memory;
    case       DeviceInfo:       CUDA_DeviceInfo device_info;
    case   EnqueueRequest:   CUDA_EnqueueRequest enqueue_request;
    case   ExecutedKernel:   CUDA_ExecutedKernel executed_kernel;
    case     LoadedModule:     CUDA_LoadedModule loaded_module;
    case ResolvedFunction: CUDA_ResolvedFunction resolved_function;
    case        SetMemory:        CUDA_SetMemory set_memory;
    case   UnloadedModule:   CUDA_UnloadedModule unloaded_module;

    default: void;
};



/**
 * Structure of the blob containing our performance data.
 *
 * @note    The CUDA driver contains a separate loader that is used to load
 *          binary code modules onto the GPU, resolve symbols, and invoke
 *          functions (kernels). Additionally, there are multiple types of
 *          performance data generated by this collector. These issues lead
 *          to this collector's performance data blobs being significantly
 *          more complex than those of the typical collector. To facilitate
 *          maximum reuse of existing collector infrastructure, all of the
 *          different data generated by this collector is "packed" into one
 *          performance data blob type using a XDR discriminated union.
 *
 * @sa http://en.wikipedia.org/wiki/Tagged_union
 */
struct CBTF_cuda_data
{
    /** Individual messages containing data gathered by this collector. */
    CBTF_cuda_message messages<>;

    /**
     * Unique, null-terminated, stack traces referenced by the messages.
     *
     * @note    Because calls to the CUDA driver typically occur from a limited
     *          set of call sites, grouping them together in this way can result
     *          in significant data compression.
     */
    CBTF_Protocol_Address stack_traces<>;
};
