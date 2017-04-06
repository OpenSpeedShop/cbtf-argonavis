/*******************************************************************************
** Copyright (c) 2012-2017 Argo Navis Technologies. All Rights Reserved.
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
%#include "KrellInstitute/Messages/Time.h"



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
 * Enumeration of different sampled event kinds.
 */ 
enum CUDA_EventKind
{
    UnknownEventKind = 0,    
    Count = 1,
    Percentage = 2,
    Rate = 3
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
 * Enumeration of the different types of messages that are encapsulated within
 * this collector's blobs. See the note on CBTF_cuda_data for more information.
 */
enum CUDA_MessageTypes
{
    CompletedExec = 0,
    CompletedXfer = 1,
    ContextInfo = 2,
    DeviceInfo = 3,
    EnqueueExec = 4,
    EnqueueXfer = 5,
    OverflowSamples = 6,
    PeriodicSamples = 7,
    SamplingConfig = 8
};



/**
 * Description of a single sampled event.
 */
struct CUDA_EventDescription
{
    /**
     * Name of the event. This is PAPI's or CUPTI's ASCII name for the event.
     *
     * @sa http://icl.cs.utk.edu/papi/
     * @sa http://docs.nvidia.com/cuda/cupti/r_main.html#r_metric_api
     */
    string name<>;

    /** Kind of the event. */
    CUDA_EventKind kind;

    /**
     * Threshold for the event. This the number of events between consecutive
     * overflow samples. Zero when only periodic (in time) sampling was used.
     */
    int threshold;
};



/**
 * Message emitted when the CUDA driver completes a kernel execution.
 */
struct CUDA_CompletedExec
{
    /** Correlation ID of the kernel execution. */
    uint32_t id;

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
 * Message emitted when the CUDA driver completes a data transfer.
 */
struct CUDA_CompletedXfer
{
    /** Correlation ID of the data transfer. */
    uint32_t id;

    /** Time at which the data transfer began. */
    CBTF_Protocol_Time time_begin;
    
    /** Time at which the data transfer ended. */
    CBTF_Protocol_Time time_end;

    /** Number of bytes being transferred. */
    uint64_t size;

    /** Kind of data transfer performed. */
    CUDA_CopyKind kind;

    /** Kind of memory from which the data transfer was performed. */
    CUDA_MemoryKind source_kind;

    /** Kind of memory to which the data transfer was performed. */
    CUDA_MemoryKind destination_kind;

    /** Was the data transfer asynchronous? */
    bool asynchronous;  
};



/**
 * Message containing information about a CUDA context.
 */
struct CUDA_ContextInfo
{
    /** CUDA context for which this is information. */
    CBTF_Protocol_Address context;

    /** CUDA device for this context. */
    uint32_t device;
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
 * Message emitted when the CUDA driver enqueues a kernel execution.
 */
struct CUDA_EnqueueExec
{
    /** Correlation ID of the kernel execution. */
    uint32_t id;

    /** CUDA context for which the kernel execution was enqueued. */
    CBTF_Protocol_Address context;

    /** CUDA stream for which the kernel execution was enqueued. */
    CBTF_Protocol_Address stream;

    /** Time at which the kernel execution was enqueued. */
    CBTF_Protocol_Time time;

    /**
     * Call site of the kernel execution. This is an index into the
     * stack_traces array of the CBTF_cuda_data containing this message.
     */
    uint32_t call_site;
};



/**
 * Message emitted when the CUDA driver enqueues a data transfer.
 */
struct CUDA_EnqueueXfer
{
    /** Correlation ID of the data transfer. */
    uint32_t id;

    /** CUDA context for which the data transfer was enqueued. */
    CBTF_Protocol_Address context;

    /** CUDA stream for which the data transfer was enqueued. */
    CBTF_Protocol_Address stream;
    
    /** Time at which the data transfer was enqueued. */
    CBTF_Protocol_Time time;

    /**
     * Call site of the data transfer. This is an index into the
     * stack_traces array of the CBTF_cuda_data containing this message.
     */
    uint32_t call_site;
};



/**
 * Message containing event overflow samples.
 */
struct CUDA_OverflowSamples
{
    /** Time at which the first overflow sample was taken. */
    CBTF_Protocol_Time time_begin;
    
    /** Time at which the last overflow sample was taken. */
    CBTF_Protocol_Time time_end;
    
    /** Program counter (PC) addresses. */
    CBTF_Protocol_Address pcs<>;
    
    /** 
     * Event overflow counts at those addresses. Given the definitions:
     *
     *     N: Length of the "pcs" array above.
     *
     *     n: Index [0, N) of some PC within the "pcs" array above.
     *
     *     E: Number of events in this thread's CUDA_SamplingConfig.events
     *        array with a non-zero threshold. I.e. the number of events for
     *        which overflow sampling is enabled.
     *
     *     e: Index [0, E) of a event for which overflow sampling is enabled.
     *
     * The length of this counts array is (N * E) and the index of a particular
     * count is ((n * E) + e).
     */
    uint64_t counts<>;
};



/**
 * Message containing periodic (in time) event samples.
 */
struct CUDA_PeriodicSamples
{
    /**
     * Time and event count deltas for each sample. Each sample consists of one
     * time delta followed by an array of count deltas whose length equals the
     * length of this thread's CUDA_SamplingConfig.events array. All deltas are
     * relative to the corresponding value within the previous sample, except
     * for the first sample, which is relative to zero for both time and counts.
     *
     * Every delta is represented using a variable-length byte sequence encoding
     * a 64-bit unsigned (assumed positive) integer. Four possible encodings are
     * defined:
     *
     *
     * +--------+
     * |00aaaaaa| -->
     * +--------+
     *
     * +--------+--------+--------+--------+--------+--------+--------+--------+
     * |00000000|00000000|00000000|00000000|00000000|00000000|00000000|00aaaaaa|
     * +--------+--------+--------+--------+--------+--------+--------+--------+
     *
     *
     * +--------+--------+--------+
     * |01aaaaaa|bbbbbbbb|cccccccc| -->
     * +--------+--------+--------+
     *
     * +--------+--------+--------+--------+--------+--------+--------+--------+
     * |00000000|00000000|00000000|00000000|00000000|00aaaaaa|bbbbbbbb|cccccccc|
     * +--------+--------+--------+--------+--------+--------+--------+--------+
     *
     *
     * +--------+--------+--------+--------+
     * |10aaaaaa|bbbbbbbb|cccccccc|dddddddd| -->
     * +--------+--------+--------+--------+
     *
     * +--------+--------+--------+--------+--------+--------+--------+--------+
     * |00000000|00000000|00000000|00000000|00aaaaaa|bbbbbbbb|cccccccc|dddddddd|
     * +--------+--------+--------+--------+--------+--------+--------+--------+
     *
     *
     * +--------+--------+--------+--------+
     * |11000000|aaaaaaaa|bbbbbbbb|cccccccc|
     * +--------+--------+--------+--------+--------+
     * |dddddddd|eeeeeeee|ffffffff|gggggggg|hhhhhhhh| -->
     * +--------+--------+--------+--------+--------+
     *
     * +--------+--------+--------+--------+--------+--------+--------+--------+
     * |aaaaaaaa|bbbbbbbb|cccccccc|dddddddd|eeeeeeee|ffffffff|gggggggg|hhhhhhhh|
     * +--------+--------+--------+--------+--------+--------+--------+--------+
     *
     *
     * This encoding scheme was selected as follows... First, it must be able to
     * represent <em>all</em> possible deltas. It was also desireable to be able
     * to represent a zero delta efficiently as 1 byte. That dicated at least 2
     * encodings, but of course that alone doesn't give much compression. Adding
     * a second bit for selecting the encoding allows more flexibilty by giving
     * two additional encoding options.
     *
     * To select those, additional assumptions were made:
     *
     *     1) Modern processors operate with clock rates lower than 4 GHz.
     *
     *     2) Clock ticks are the highest frequency events to be counted.
     *
     *     3) Sampling less than 100 samples/second is likely too coarsed
     *        grained to be truly useful.
     *
     * Thus deltas can be expected to be less than 40,000,000 (26 bits) most of
     * the time, requiring 3 additional bytes, assumning 6 bits of delta in the
     * first byte. This gave only two choices for the final encoding - either 1
     * or 2 additional bytes. Using 1 byte would allow deltas up to 16,384 which
     * didn't seem as useful as using 2 bytes giving deltas up to 4,194,304.
     *
     * As an example of the overall scheme, assume the following samples are to
     * be encoded within a single message:
     *
     *     Time                Event A            Event B
     *
     *     0x1234567800000000  0x0000000001000000  0x0004000000000000
     *     0x1234567800100000  0x0000000001021000  0x000400000000034A
     *     0x1234567800200000  0x00000000010213FE  0x000400000000034C
     *
     * This would be encoded as the following byte sequence:
     *
     *     C0 12 34 56 78 00 00 00 00    Sample #0: Time
     *     81 00 00 00                              Event A
     *     C0 00 04 00 00 00 00 00 00               Event B
     *     80 10 00 00                   Sample #1: Time
     *     42 10 00                                 Event A
     *     40 03 4A                                 Event B
     *     80 10 00 00                   Sample #2: Time
     *     40 03 FE                                 Event A
     *     02                                       Event B
     *
     * This encoding requires 40 bytes instead of the 72 bytes required for the
     * unencoded original. In this case the saving is only about 50%, but keep
     * in mind that the first sample typically requires a larger number of bytes
     * because it is zero-relative, and in the above example this cost is under
     * amortized because a small number of samples is encoded.
     */
    uint8_t deltas<>;
};



/**
 * Message containing the event sampling configuration.
 */
struct CUDA_SamplingConfig
{
    /**
     * Sampling interval in nanoseconds. This is the time between consecutive
     * periodic (in time) samples.
     */
    uint64_t interval;
    
    /** Descriptions of the sampled events. */
    CUDA_EventDescription events<>;
};



/**
 * Union of the different types of messages that are encapsulated within this
 * collector's blobs. See the note on CBTF_cuda_data for more information.
 */
union CBTF_cuda_message switch (unsigned type)
{
    case   CompletedExec:   CUDA_CompletedExec completed_exec;
    case   CompletedXfer:   CUDA_CompletedXfer completed_xfer;
    case     ContextInfo:     CUDA_ContextInfo context_info;
    case      DeviceInfo:      CUDA_DeviceInfo device_info;
    case     EnqueueExec:     CUDA_EnqueueExec enqueue_exec;
    case     EnqueueXfer:     CUDA_EnqueueXfer enqueue_xfer;
    case OverflowSamples: CUDA_OverflowSamples overflow_samples;
    case PeriodicSamples: CUDA_PeriodicSamples periodic_samples;
    case  SamplingConfig:  CUDA_SamplingConfig sampling_config;

    default: void;
};



/**
 * Structure of the blob containing our performance data.
 *
 * @note    This collector generates multiple types of performance data, which
 *          leads to its blobs being significantly more complex than those of
 *          the typical collector. All of this data is "packed" into a single
 *          performance data blob type via a XDR discriminated union in order
 *          to facilitate maximum reuse of existing collector infrastructure.
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
