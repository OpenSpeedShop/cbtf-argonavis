/*******************************************************************************
** Copyright (c) 2012-2014 Argo Navis Technologies. All Rights Reserved.
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
#include "KrellInstitute/Services/Data.h"
#include "KrellInstitute/Services/Time.h"
#include "KrellInstitute/Services/Timer.h"
#include "KrellInstitute/Services/TLS.h"
#include "KrellInstitute/Services/Unwind.h"

#include "CUDA_data.h"



/**
 * Alignment (in bytes) of each allocated CUPTI activity buffer. This value
 * is specified in the documentation for cuptiActivityEnqueueBuffer() within
 * the "cupti_activity.h" header file.
 */
#define CUPTI_ACTIVITY_BUFFER_ALIGNMENT 8

/**
 * Size (in bytes) of each allocated CUPTI activity buffer.
 *
 * @note    Currently the only basis for the selection of this value is that
 *          the CUPTI "activity_trace.cpp" example uses 2 buffers of 32 KB each.
 */
#define CUPTI_ACTIVITY_BUFFER_SIZE (2 * 32 * 1024 /* 64 KB */)

/**
 * Maximum number of (CBTF_Protocol_Address) stack trace addresses contained
 * within each (CBTF_cuda_data) performance data blob.
 *
 * @note    Currently there is no specific basis for the selection of this
 *          value other than a vague notion that it seems about right. In
 *          the future, performance testing should be done to determine an
 *          optimal value.
 */
#define MAX_ADDRESSES_PER_BLOB 1024

/** 
 * Maximum supported number of CUDA contexts. Controls the size of the table
 * used to translate CUPTI context IDs to CUDA context pointers.
 *
 * @note    Currently there is no specific basis for the selection of this
 *          value other than a vague notion that it seems about right.
 */
#define MAX_CUDA_CONTEXTS 32

/**
 * Maximum number of individual (CBTF_cuda_message) messages contained within
 * each (CBTF_cuda_data) performance data blob. 
 *
 * @note    Currently there is no specific basis for the selection of this
 *          value other than a vague notion that it seems about right. In
 *          the future, performance testing should be done to determine an
 *          optimal value.
 */
#define MAX_MESSAGES_PER_BLOB 128



#if defined(PAPI_FOUND)
/**
 * Maximum number of bytes used to store the periodic sampling deltas within
 * each (CBTF_cuda_data) performance data blob.
 *
 * @note    Assuming that 2 events are being sampled, and that deltas can
 *          typically be encoded in 3 bytes/delta, 9 bytes/sample will be
 *          needed (including the time delta). Periodic sampling occcurs
 *          at 100 samples/second by default. Thus periodic sampling will
 *          require 900 bytes/second under these assumptions, and 32 KB
 *          will store about 36 seconds worth of periodic sampling data,
 *          which seems reasonable.
 */
#define MAX_DELTAS_BYTES_PER_BLOB (32 * 1024 /* 32 KB */)

/**
 * Maximum supported number of concurrently sampled events. Controls the fixed
 * size of several of the tables related to event sampling.
 *
 * @note    This value should not be construed to be the actual number of
 *          concurrent events supported by any particular hardware. It is
 *          the maximum supported by this performance data collector.
 */
#define MAX_EVENTS 16

/**
 * Maximum number of (CBTF_Protocol_Address) unique overflow PC addresses
 * contained within each (CBTF_cuda_data) performance data blob.
 *
 * @note    Currently there is no specific basis for the selection of this
 *          value other than a vague notion that it seems about right. In
 *          the future, performance testing should be done to determine an
 *          optimal value.
 */
#define MAX_OVERFLOW_PCS_PER_BLOB 1024

/**
 * Number of entries in the overflow sampling PC addresses hash table.
 */
#define OVERFLOW_HASH_TABLE_SIZE \
    (MAX_OVERFLOW_PCS_PER_BLOB + (MAX_OVERFLOW_PCS_PER_BLOB / 4))
#endif



/**
 * Checks that the given CUPTI function call returns the value "CUPTI_SUCCESS".
 * If the call was unsuccessful, the returned error is reported on the standard
 * error stream and the application is aborted.
 *
 * @param x    CUPTI function call to be checked.
 */
#define CUPTI_CHECK(x)                                              \
    do {                                                            \
        CUptiResult RETVAL = x;                                     \
        if (RETVAL != CUPTI_SUCCESS)                                \
        {                                                           \
            const char* description = NULL;                         \
            if (cuptiGetResultString(RETVAL, &description) ==       \
                CUPTI_SUCCESS)                                      \
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

/**
 * Checks that the given pthread function call returns the value "0". If the
 * call was unsuccessful, the returned error is reported on the standard error
 * stream and the application is aborted.
 *
 * @param x    Pthread function call to be checked.
 */
#define PTHREAD_CHECK(x)                                   \
    do {                                                   \
        int RETVAL = x;                                    \
        if (RETVAL != 0)                                   \
        {                                                  \
            fprintf(stderr, "[CBTF/CUDA] %s(): %s = %d\n", \
                    __func__, #x, RETVAL);                 \
            fflush(stderr);                                \
            abort();                                       \
        }                                                  \
    } while (0)



/** String uniquely identifying this collector. */
const char* const cbtf_collector_unique_id = "cuda";

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

/** Flag indicating if debugging is enabled. */
static bool debug = FALSE;

/** CUPTI subscriber handle for this collector. */
static CUpti_SubscriberHandle cupti_subscriber_handle;

/**
 * The offset that must be added to all CUPTI-provided time values in order to
 * translate them to the same time "origin" provided by CBTF_GetTime(). Computed
 * by the process-wide initialization in cbtf_collector_start().
 */
static int64_t cupti_time_offset = 0;

/**
 * Table used to translate CUPTI context IDs to CUDA context pointers.
 *
 * For some reason CUPTI returns CUPTI_ACTIVITY_KIND_CONTEXT activity records
 * in the global, rather than the per-context, activity queue. So the "context"
 * parameter to add_activities() is NULL when this record is processed, making
 * it difficult to associate the provided information with a context pointer.
 * The associated CUpti_ActivityContext structure contains a "contextId" field,
 * but there is no direct way to map between context IDs and context pointers
 * similar to the cuptiGetStreamId() function. The only possible solution is
 * to store the mapping from context ID to pointer explicitly as encountered
 * in memcpy, memset, and kernel invocation activigy records. Then use that
 * mapping when a CUPTI_ACTIVITY_KIND_CONTEXT activity record is found.
 */
struct {
    struct {
        uint32_t id;
        CUcontext ptr;
    } values[MAX_CUDA_CONTEXTS];
    pthread_mutex_t mutex;
} context_id_to_ptr;

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
static int overflow_sampling_count = 0;

/** Number of events for which periodic sampling is enabled. */
static int periodic_sampling_count = 0;

/**
 * Map of event indicies within the PAPI event set to overflow count indicies.
 */
static int event_to_overflow_indicies[MAX_EVENTS];

/** Type defining the data stored for each event sample. */
typedef struct {
    CBTF_Protocol_Time time;    /**< Time at which the sample was taken. */
    uint64_t count[MAX_EVENTS]; /**< Count for each sampled event. */
} EventSample;
#endif


    
/** Type defining the data stored in thread-local storage. */
typedef struct {

    /** Flag indicating if data collection is paused. */
    bool paused;

    /**
     * Performance data header to be applied to this thread's performance data.
     * All of the fields except [addr|time]_[begin|end] are constant throughout
     * data collection. These exceptions are updated dynamically by the various
     * collection routines.
     */
    CBTF_DataHeader data_header;

    /**
     * Current performance data blob for this thread. Messages are added by the
     * various collection routines. It is sent when full, or upon completion of
     * data collection.
     */
    CBTF_cuda_data data;

    /**
     * Individual messages containing data gathered by this collector. Pointed
     * to by the performance data blob above.
     */
    CBTF_cuda_message messages[MAX_MESSAGES_PER_BLOB];

    /**
     * Unique, null-terminated, stack traces referenced by the messages. Pointed
     * to by the performance data blob above.
     */
    CBTF_Protocol_Address stack_traces[MAX_ADDRESSES_PER_BLOB];

#if defined(PAPI_FOUND)
    /** PAPI event set for this thread. */
    int papi_event_set;

    /** Current overflow samples for this thread. */
    struct {

        /**
         * Pointer to the message containing the overflow event samples. There
         * is always one such message within every performance data blob when
         * overflow event sampling is enabled.
         */
        CUDA_OverflowSamples* message;

        /**
         * Program counter (PC) addresses. Pointed to by the above message.
         */
        CBTF_Protocol_Address pcs[MAX_OVERFLOW_PCS_PER_BLOB];

        /**
         * Event overflow count at those addresses. Pointed to by the above
         * message.
         */
        uint64_t counts[MAX_OVERFLOW_PCS_PER_BLOB * MAX_EVENTS];

        /**
         * Hash table used to map PC addresses to their array index within
         * the "pcs" array above. The value stored in the table is actually
         * one more than the real index so that a zero value can be used to
         * indicate an empty hash table entry.
         */
        uint32_t hash_table[OVERFLOW_HASH_TABLE_SIZE];

    } overflow_samples;
    
    /** Current periodic event samples for this thread. */
    struct {
        
        /**
         * Pointer to the message containing the periodic event samples. There
         * is always one such message within every performance data blob when
         * periodic event sampling is enabled.
         */
        CUDA_PeriodicSamples* message;
        
        /**
         * Time and event count deltas. Pointed to by the above message.
         */
        uint8_t deltas[MAX_DELTAS_BYTES_PER_BLOB];
        
        /** Previously taken event sample. */
        EventSample previous;
        
    } periodic_samples;
#endif
    
} TLS;

#if defined(USE_EXPLICIT_TLS)
/**
 * Key used to look up our thread-local storage. This key <em>must</em> be
 * unique from any other key used by any of the CBTF services.
 */
static const uint32_t TLSKey = 0xBADC00DA;
#else
/** Thread-local storage. */
static __thread TLS the_tls;
#endif



/**
 * Add the specified mapping of CUPTI context ID to CUDA context pointer.
 *
 * @param id     CUPTI context ID. 
 * @param ptr    Corresponding CUDA context pointer.
 */
static void add_context_id_to_ptr(uint32_t id, CUcontext ptr)
{
    PTHREAD_CHECK(pthread_mutex_lock(&context_id_to_ptr.mutex));

    int i;
    for (i = 0;
         (i < MAX_CUDA_CONTEXTS) && (context_id_to_ptr.values[i].ptr != NULL);
         ++i)
    {
        if (context_id_to_ptr.values[i].id == id)
        {
            if (context_id_to_ptr.values[i].ptr != ptr)
            {
                fprintf(stderr, "[CBTF/CUDA] add_context_id_to_ptr(): "
                        "CUDA context pointer for CUPTI context "
                        "ID %u changed!\n", id);
                fflush(stderr);
                abort();
            }
            
            break;
        }
    }

    if (i == MAX_CUDA_CONTEXTS)
    {
        fprintf(stderr, "[CBTF/CUDA] add_context_id_to_ptr(): "
                "Maximum supported CUDA context pointers (%d) was reached!\n",
                MAX_CUDA_CONTEXTS);
        fflush(stderr);
        abort();
    }
    else if (context_id_to_ptr.values[i].ptr == NULL)
    {
        context_id_to_ptr.values[i].id = id;
        context_id_to_ptr.values[i].ptr = ptr;
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&context_id_to_ptr.mutex));
}



/**
 * Find the CUDA context pointer corresponding to the given CUPTI context ID.
 *
 * @param id    CUPTI context ID.
 * @return      Corresponding CUDA context pointer.
 */
static CUcontext find_context_ptr_from_id(uint32_t id)
{
    CUcontext ptr = NULL;

    PTHREAD_CHECK(pthread_mutex_lock(&context_id_to_ptr.mutex));
    
    int i;
    for (i = 0;
         (i < MAX_CUDA_CONTEXTS) && (context_id_to_ptr.values[i].ptr != NULL);
         ++i)
    {
        if (context_id_to_ptr.values[i].id == id)
        {
            ptr = context_id_to_ptr.values[i].ptr;
            break;
        }
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&context_id_to_ptr.mutex));

    return ptr;
}



/**
 * Initialize the performance data header and blob contained within the given
 * thread-local storage. This function <em>must</em> be called before any of
 * the collection routines attempts to add a message.
 *
 * @param tls    Thread-local storage to be initialized.
 */
static void initialize_data(TLS* tls)
{
    Assert(tls != NULL);

    tls->data_header.time_begin = ~0;
    tls->data_header.time_end = 0;
    tls->data_header.addr_begin = ~0;
    tls->data_header.addr_end = 0;
    
    tls->data.messages.messages_len = 0;
    tls->data.messages.messages_val = tls->messages;
    
    tls->data.stack_traces.stack_traces_len = 0;
    tls->data.stack_traces.stack_traces_val = tls->stack_traces;
    
    memset(tls->stack_traces, 0, sizeof(tls->stack_traces));

#if defined(PAPI_FOUND)
    if (overflow_sampling_count > 0)
    {
        CBTF_cuda_message* raw_message = 
            &(tls->messages[tls->data.messages.messages_len++]);
        raw_message->type = OverflowSamples;
        
        tls->overflow_samples.message =
            &raw_message->CBTF_cuda_message_u.overflow_samples;

        tls->overflow_samples.message->time_begin = ~0;
        tls->overflow_samples.message->time_end = 0;
        
        tls->overflow_samples.message->pcs.pcs_len = 0;
        tls->overflow_samples.message->pcs.pcs_val = tls->overflow_samples.pcs;
        
        tls->overflow_samples.message->counts.counts_len = 0;
        tls->overflow_samples.message->counts.counts_val = 
            tls->overflow_samples.counts;
        
        memset(tls->overflow_samples.hash_table, 0,
               sizeof(tls->overflow_samples.hash_table));
    }
    
    if (periodic_sampling_count > 0)
    {
        CBTF_cuda_message* raw_message = 
            &(tls->messages[tls->data.messages.messages_len++]);
        raw_message->type = PeriodicSamples;
        
        tls->periodic_samples.message =
            &raw_message->CBTF_cuda_message_u.periodic_samples;
        
        tls->periodic_samples.message->deltas.deltas_len = 0;
        tls->periodic_samples.message->deltas.deltas_val = 
            tls->periodic_samples.deltas;
        
        memset(&tls->periodic_samples.previous, 0, sizeof(EventSample));        
    }
#endif
}



/**
 * Send the performance data blob contained within the given thread-local
 * storage. The blob is re-initialized (cleared) after being sent. Nothing
 * is sent if the blob is empty.
 *
 * @param tls    Thread-local storage containing data to be sent.
 */
static void send_data(TLS* tls)
{
    Assert(tls != NULL);

    if (tls->data.messages.messages_len > 0)
    {
#if !defined(NDEBUG)
        if (debug)
        {
            printf("[CBTF/CUDA] send_data(): "
                   "sending CBTF_cuda_data message (%u msg, %u pc)\n",
                   tls->data.messages.messages_len,
                   tls->data.stack_traces.stack_traces_len);
        }
#endif

        cbtf_collector_send(
            &tls->data_header, (xdrproc_t)xdr_CBTF_cuda_data, &tls->data
            );
        initialize_data(tls);
    }
}



/**
 * Add a new message to the performance data blob contained within the given
 * thread-local storage. The current blob is sent and re-initialized (cleared)
 * if it is already full.
 *
 * @param tls    Thread-local storage to which a message is to be added.
 * @return       Pointer to the new message to be filled in by the caller.
 */
static CBTF_cuda_message* add_message(TLS* tls)
{
    Assert(tls != NULL);

    if (tls->data.messages.messages_len == MAX_MESSAGES_PER_BLOB)
    {
        send_data(tls);
    }
    
    return &(tls->messages[tls->data.messages.messages_len++]);
}



/**
 * Update the performance data header contained within the given thread-local
 * storage with the specified time. Insures that the time interval defined by
 * time_begin and time_end contain the specified time.
 *
 * @param tls     Thread-local storage to be updated.
 * @param time    Time with which to update.
 */
inline void update_header_with_time(TLS* tls, CBTF_Protocol_Time time)
{
    Assert(tls != NULL);

    if (time < tls->data_header.time_begin)
    {
        tls->data_header.time_begin = time;
    }
    if (time >= tls->data_header.time_end)
    {
        tls->data_header.time_end = time + 1;
    }
}



/**
 * Update the performance data header contained within the given thread-local
 * storage with the specified address. Insures that the address range defined
 * by addr_begin and addr_end contain the specified address.
 *
 * @param tls     Thread-local storage to be updated.
 * @param addr    Address with which to update.
 */
inline void update_header_with_address(TLS* tls, CBTF_Protocol_Address addr)
{
    Assert(tls != NULL);

    if (addr < tls->data_header.addr_begin)
    {
        tls->data_header.addr_begin = addr;
    }
    if (addr >= tls->data_header.addr_end)
    {
        tls->data_header.addr_end = addr + 1;
    }
}



/**
 * Add a new stack trace for the current call site to the performance data
 * blob contained within the given thread-local storage.
 *
 * @param tls    Thread-local storage to which the stack trace is to be added.
 * @return       Index of this call site within the performance data blob.
 */
static uint32_t add_current_call_site(TLS* tls)
{
    Assert(tls != NULL);

    /* Get the stack trace for the current call site */

    int frame_count = 0;
    uint64_t frame_buffer[CBTF_ST_MAXFRAMES];
    
    CBTF_GetStackTraceFromContext(
        NULL, FALSE, 0, CBTF_ST_MAXFRAMES, &frame_count, frame_buffer
        );

    /* Search for this stack trace amongst the existing stack traces */
    
    int i, j;
    
    /* Iterate over the addresses in the existing stack traces */
    for (i = 0, j = 0; i < MAX_ADDRESSES_PER_BLOB; ++i)
    {
        /* Is this the terminating null of an existing stack trace? */
        if (tls->stack_traces[i] == 0)
        {
            /*
             * Terminate the search if a complete match has been found between
             * this stack trace and the existing stack trace.
             */
            if (j == frame_count)
            {
                break;
            }

            /*
             * Otherwise check for a null in the first or last entry, or
             * for consecutive nulls, all of which indicate the end of the
             * existing stack traces, and the need to add this stack trace
             * to the existing stack traces.
             */
            else if ((i == 0) || 
                     (i == (MAX_ADDRESSES_PER_BLOB - 1)) ||
                     (tls->stack_traces[i - 1] == 0))
            {
                /*
                 * Send performance data for this thread if there isn't enough
                 * room in the existing stack traces to add this stack trace.
                 * Doing so frees up enough space for this stack trace.
                 */
                if ((i + frame_count) >= MAX_ADDRESSES_PER_BLOB)
                {
                    send_data(tls);
                    i = 0;
                }

                /* Add this stack trace to the existing stack traces */
                for (j = 0; j < frame_count; ++j, ++i)
                {
                    tls->stack_traces[i] = frame_buffer[j];
                    update_header_with_address(tls, tls->stack_traces[i]);
                }
                tls->stack_traces[i] = 0;
                tls->data.stack_traces.stack_traces_len = i + 1;
              
                break;
            }
            
            /* Otherwise reset the pointer within this stack trace to zero. */
            else
            {
                j = 0;
            }
        }
        else
        {
            /*
             * Advance the pointer within this stack trace if the current
             * address within this stack trace matches the current address
             * within the existing stack traces. Otherwise reset the pointer
             * to zero.
             */
            j = (frame_buffer[j] == tls->stack_traces[i]) ? (j + 1) : 0;
        }
    }

    /* Return the index of this stack trace within the existing stack traces */
    return i - frame_count;
}



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
 */
static void add_activities(TLS* tls, CUcontext context, 
                           CUstream stream, uint32_t stream_id)
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

    /* Iterate over each activity record */
    
    CUpti_Activity* raw_activity = NULL;

    size_t added;
    for (added = 0; true; ++added)
    {
        retval = cuptiActivityGetNextRecord(buffer, size, &raw_activity);

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

                CBTF_cuda_message* raw_message = add_message(tls);
                Assert(raw_message != NULL);
                raw_message->type = ContextInfo;
                
                CUDA_ContextInfo* message =
                    &raw_message->CBTF_cuda_message_u.context_info;

                message->context = (CBTF_Protocol_Address)
                    find_context_ptr_from_id(activity->contextId);

                message->device = activity->deviceId;
                
                switch (activity->computeApiKind)
                {

                case CUPTI_ACTIVITY_COMPUTE_API_UNKNOWN:
                    message->compute_api = "Unknown";
                    break;

                case CUPTI_ACTIVITY_COMPUTE_API_CUDA:
                    message->compute_api = "CUDA";
                    break;

#if CUPTI_API_VERSION == 2
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

                CBTF_cuda_message* raw_message = add_message(tls);
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

                CBTF_cuda_message* raw_message = add_message(tls);
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
                
                update_header_with_time(tls, message->time_begin);
                update_header_with_time(tls, message->time_end);

                /* Add the context ID to pointer mapping from this activity */
                add_context_id_to_ptr(activity->contextId, context);
            }
            break;



        case CUPTI_ACTIVITY_KIND_MEMSET:
            {
                const CUpti_ActivityMemset* const activity =
                    (CUpti_ActivityMemset*)raw_activity;

                /* Add a message for this activity */

                CBTF_cuda_message* raw_message = add_message(tls);
                Assert(raw_message != NULL);
                raw_message->type = SetMemory;
                
                CUDA_SetMemory* message =
                    &raw_message->CBTF_cuda_message_u.set_memory;

                message->context = (CBTF_Protocol_Address)context;
                message->stream = (CBTF_Protocol_Address)stream;

                message->time_begin = activity->start + cupti_time_offset;
                message->time_end = activity->end + cupti_time_offset;

                message->size = activity->bytes;
                
                update_header_with_time(tls, message->time_begin);
                update_header_with_time(tls, message->time_end);

                /* Add the context ID to pointer mapping from this activity */
                add_context_id_to_ptr(activity->contextId, context);
            }
            break;



        case CUPTI_ACTIVITY_KIND_KERNEL:
            {
                const CUpti_ActivityKernel* const activity =
                    (CUpti_ActivityKernel*)raw_activity;

                /* Add a message for this activity */

                CBTF_cuda_message* raw_message = add_message(tls);
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

                update_header_with_time(tls, message->time_begin);
                update_header_with_time(tls, message->time_end);

                /* Add the context ID to pointer mapping from this activity */
                add_context_id_to_ptr(activity->contextId, context);
            }
            break;



        default:
            break;
        }
    }

#if !defined(NDEBUG)
    if (debug)
    {
        printf("[CBTF/CUDA] add_activities(): "
               "added %u activity records for stream %p in context %p\n",
               (unsigned int)added, stream, context);
    }
#endif
    
    /* Re-enqueue this buffer of activities */
    CUPTI_CHECK(cuptiActivityEnqueueBuffer(
                    context, stream_id, buffer, CUPTI_ACTIVITY_BUFFER_SIZE
                    ));
}



#if defined(PAPI_FOUND)
/**
 * Callback invoked by PAPI every time an event counter overflows. I.e. reaches
 * the threshold value previously specified in parse_configuration().
 *
 * @param event_set          Event set of the overflowing event. This should
 *                           always be papi_event_set since that is the only
 *                           event set being used by this collector.
 * @param address            Program counter address when the overflow occurred.
 * @param overflow_vector    Vector indicating the particular event(s) that have
 *                           overflowed. Call PAPI_get_overflow_event_index() to
 *                           decompose this vector into the individual events.
 * @param context            Thread context when the overflow occurred.
 */
static void papi_callback(int event_set, void* address,
                          long long overflow_vector, void* context)
{
    Assert(overflow_sampling_count > 0);

    /* Access our thread-local storage */
#if defined(USE_EXPLICIT_TLS)
    TLS* tls = CBTF_GetTLS(TLSKey);
#else
    TLS* tls = &the_tls;
#endif
    Assert(tls != NULL);

    /* Do nothing if data collection is paused for this thread */
    if (tls->paused)
    {
        return;
    }

    /* Get the time at which the overflow occurred */
    CBTF_Protocol_Time time = CBTF_GetTime();

    /* Get the PC at which the overflow occurred */
    CBTF_Protocol_Address pc = (CBTF_Protocol_Address)address;
    
    /* Determine which events overflowed */
    int events[MAX_EVENTS];
    memset(events, 0, sizeof(events));
    int num_events = sampling_config.events.events_len;
    PAPI_CHECK(PAPI_get_overflow_event_index(tls->papi_event_set,
                                             overflow_vector, events,
                                             &num_events));

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
                send_data(tls);
                continue;
            }

            /* Add an entry for this sample to the existing overflow samples */
            pcs[tls->overflow_samples.message->pcs.pcs_len] = pc;
            memset(&counts[tls->overflow_samples.message->counts.counts_len],
                   0, overflow_sampling_count * sizeof(uint64_t));            
            hash_table[bucket] = ++tls->overflow_samples.message->pcs.pcs_len;
            tls->overflow_samples.message->counts.counts_len += 
                overflow_sampling_count;
                                    
            /* Update the header with this sample's address */
            update_header_with_address(tls, pc);            
        }

        /*
         * Increment the counts for the events that actually overflowed for
         * this sample. The funky triple-nested array indexing selects each
         * event that overflowed, maps its PAPI event index to its index in
         * the counts array, and then adds that result to the base index of
         * this PC address within the counts array.
         */
        int e, base = (hash_table[bucket] - 1) * overflow_sampling_count;
        for (e = 0; e < num_events; ++e)
        {
            counts[base + event_to_overflow_indicies[events[e]]]++;            
        }

        /* This sample has now been successfully added */
        break;
    }

    /* Update the header (and overflow samples message) with this sample time */

    update_header_with_time(tls, time);

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
    Assert(periodic_sampling_count > 0);

    /* Access our thread-local storage */
#if defined(USE_EXPLICIT_TLS)
    TLS* tls = CBTF_GetTLS(TLSKey);
#else
    TLS* tls = &the_tls;
#endif
    Assert(tls != NULL);

    /* Do nothing if data collection is paused for this thread */
    if (tls->paused)
    {
        return;
    }

    /* Collect a new event sample */
    EventSample sample;
    memset(&sample, 0, sizeof(EventSample));
    sample.time = CBTF_GetTime();
    PAPI_CHECK(PAPI_read(tls->papi_event_set, (long long*)&sample.count));

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
    int i, iEnd = periodic_sampling_count + 1;
    for (i = 0; i < iEnd; ++i, ++previous, ++current)
    {
        /*
         * Compute the delta between the previous and current samples for this
         * value. The previous event sample is zeroed by initialize_data(), so
         * there is no need to treat the first sample specially here.
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
            send_data(tls);
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
    update_header_with_time(tls, sample.time);
    
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
    if (tls->papi_event_set != PAPI_NULL)
    {
        return;
    }

#if !defined(NDEBUG)
    if (debug)
    {
        printf("[CBTF/CUDA] start_papi_data_collection()\n");
    }

#endif

    /* Atomically test whether PAPI has been started */
    
    PTHREAD_CHECK(pthread_mutex_lock(&context_count.mutex));
    
    if (context_count.value > 0)
    {
        /* Create our PAPI event set */
        PAPI_CHECK(PAPI_create_eventset(&tls->papi_event_set));
        
        /* Initialize our PAPI event set */
        u_int i;
        for (i = 0; i < sampling_config.events.events_len; ++i)
        {
            CUDA_EventDescription* event = 
                &sampling_config.events.events_val[i];
            
            /* Add this event to our PAPI event set */
            int event_code = PAPI_NULL;
            PAPI_CHECK(PAPI_event_name_to_code(event->name, &event_code));
            PAPI_CHECK(PAPI_add_event(tls->papi_event_set, event_code));
            
            /* Setup overflow for this event if a threshold was specified */
            if (event->threshold > 0)
            {
                PAPI_CHECK(PAPI_overflow(tls->papi_event_set,
                                         event_code,
                                         event->threshold,
                                         PAPI_OVERFLOW_FORCE_SW,
                                         papi_callback));
            }
        }
        
        /* Start the sampling of our PAPI event set */
        if (periodic_sampling_count > 0)
        {  
            PAPI_CHECK(PAPI_start(tls->papi_event_set));
            CBTF_Timer(sampling_config.interval, timer_callback);
        }
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
    if (tls->papi_event_set == PAPI_NULL)
    {
        return;
    }

#if !defined(NDEBUG)
    if (debug)
    {
        printf("[CBTF/CUDA] stop_papi_data_collection()\n");
    }
#endif

    /* Atomically test whether PAPI has been started */
    
    PTHREAD_CHECK(pthread_mutex_lock(&context_count.mutex));
    
    if (context_count.value > 0)
    {
        /* Stop the sampling of our PAPI event set */
        if (periodic_sampling_count > 0)
        {
            PAPI_CHECK(PAPI_stop(tls->papi_event_set, NULL));
            CBTF_Timer(0, NULL);
        }
        
        /* Cleanup and destroy our PAPI event set */
        PAPI_CHECK(PAPI_cleanup_eventset(tls->papi_event_set));
        PAPI_CHECK(PAPI_destroy_eventset(&tls->papi_event_set));
        tls->papi_event_set = PAPI_NULL;
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
    if (debug)
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
    if (debug)
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
#if defined(USE_EXPLICIT_TLS)
    TLS* tls = CBTF_GetTLS(TLSKey);
#else
    TLS* tls = &the_tls;
#endif
    Assert(tls != NULL);

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
                    if (debug)
                    {
                        printf("[CBTF/CUDA] enter cuLaunchKernel()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = EnqueueRequest;
                    
                    CUDA_EnqueueRequest* message = 
                        &raw_message->CBTF_cuda_message_u.enqueue_request;
                    
                    message->type = LaunchKernel;
                    message->time = CBTF_GetTime();
                    message->context = (CBTF_Protocol_Address)cbdata->context;
                    message->stream = (CBTF_Protocol_Address)params->hStream;
                    message->call_site = add_current_call_site(tls);
                    
                    update_header_with_time(tls, message->time);
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
                    if (debug)
                    {
                        printf("[CBTF/CUDA] enter cuMemcpy*()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = add_message(tls);
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

                    message->call_site = add_current_call_site(tls);
                    
                    update_header_with_time(tls, message->time);
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
                    if (debug)
                    {
                        printf("[CBTF/CUDA] enter cuMemset*()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = add_message(tls);
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

                    message->call_site = add_current_call_site(tls);
                    
                    update_header_with_time(tls, message->time);
                }
                break;



            case CUPTI_DRIVER_TRACE_CBID_cuModuleGetFunction :
                if (cbdata->callbackSite == CUPTI_API_EXIT)
                {
                    const cuModuleGetFunction_params* const params =
                        (cuModuleGetFunction_params*)cbdata->functionParams;
                    
#if !defined(NDEBUG)
                    if (debug)
                    {
                        printf("[CBTF/CUDA] exit cuModuleGetFunction()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = ResolvedFunction;
                    
                    CUDA_ResolvedFunction* message = 
                        &raw_message->CBTF_cuda_message_u.resolved_function;
                    
                    message->time = CBTF_GetTime();
                    message->module_handle = 
                        (CBTF_Protocol_Address)params->hmod;
                    message->function = (char*)params->name;
                    message->handle = (CBTF_Protocol_Address)*(params->hfunc);
                    
                    update_header_with_time(tls, message->time);
                }
                break;



            case CUPTI_DRIVER_TRACE_CBID_cuModuleLoad :
                if (cbdata->callbackSite == CUPTI_API_EXIT)
                {
                    const cuModuleLoad_params* const params =
                        (cuModuleLoad_params*)cbdata->functionParams;
                    
#if !defined(NDEBUG)
                    if (debug)
                    {
                        printf("[CBTF/CUDA] exit cuModuleLoad()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = LoadedModule;
                    
                    CUDA_LoadedModule* message =  
                        &raw_message->CBTF_cuda_message_u.loaded_module;
                    
                    message->time = CBTF_GetTime();
                    message->module.path = (char*)(params->fname);
                    message->module.checksum = 0;
                    message->handle = (CBTF_Protocol_Address)*(params->module);
                    
                    update_header_with_time(tls, message->time);
                }
                break;



            case CUPTI_DRIVER_TRACE_CBID_cuModuleLoadData :
                if (cbdata->callbackSite == CUPTI_API_EXIT)
                {
                    const cuModuleLoadData_params* const params =
                        (cuModuleLoadData_params*)cbdata->functionParams;
                    
#if !defined(NDEBUG)
                    if (debug)
                    {
                        printf("[CBTF/CUDA] exit cuModuleLoadData()\n");
                    }
#endif
                        
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = add_message(tls);
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
                        
                    update_header_with_time(tls, message->time);
                }
                break;
                
                
                
            case CUPTI_DRIVER_TRACE_CBID_cuModuleLoadDataEx :
                if (cbdata->callbackSite == CUPTI_API_EXIT)
                {
                    const cuModuleLoadDataEx_params* const params =
                        (cuModuleLoadDataEx_params*)cbdata->functionParams;
                    
#if !defined(NDEBUG)
                    if (debug)
                    {
                        printf("[CBTF/CUDA] exit cuModuleLoadDataEx()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = add_message(tls);
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
                        
                    update_header_with_time(tls, message->time);
                }
                break;
                
                

            case CUPTI_DRIVER_TRACE_CBID_cuModuleLoadFatBinary :
                if (cbdata->callbackSite == CUPTI_API_EXIT)
                {
                    const cuModuleLoadFatBinary_params* const params =
                        (cuModuleLoadFatBinary_params*)cbdata->functionParams;
                    
#if !defined(NDEBUG)
                    if (debug)
                    {
                        printf("[CBTF/CUDA] exit cuModuleLoadFatBinary()\n");
                    }
#endif
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = LoadedModule;
                    
                    CUDA_LoadedModule* message =  
                        &raw_message->CBTF_cuda_message_u.loaded_module;
                        
                    static char* const kModulePath = "<Module from Fat Binary>";
                        
                    message->time = CBTF_GetTime();
                    message->module.path = kModulePath;
                    message->module.checksum = 0;
                    message->handle = (CBTF_Protocol_Address)*(params->module);
                        
                    update_header_with_time(tls, message->time);
                }
                break;
                
                
                
            case CUPTI_DRIVER_TRACE_CBID_cuModuleUnload :
                {
                    if (cbdata->callbackSite == CUPTI_API_EXIT)
                    {
                        const cuModuleUnload_params* const params =
                            (cuModuleUnload_params*)cbdata->functionParams;
                        
#if !defined(NDEBUG)
                        if (debug)
                        {
                            printf("[CBTF/CUDA] exit cuModuleUnload()\n");
                        }
#endif
                        
                        /* Add a message for this event */
                        
                        CBTF_cuda_message* raw_message = add_message(tls);
                        Assert(raw_message != NULL);
                        raw_message->type = UnloadedModule;
                        
                        CUDA_UnloadedModule* message =  
                            &raw_message->CBTF_cuda_message_u.unloaded_module;
                        
                        message->time = CBTF_GetTime();
                        message->handle = (CBTF_Protocol_Address)params->hmod;
                        
                        update_header_with_time(tls, message->time);                                 }
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
                    if (debug)
                    {
                        printf("[CBTF/CUDA] created context %p\n",
                               rdata->context);
                    }
#endif

                    /* Enqueue a buffer for context-specific activities */
                    CUPTI_CHECK(cuptiActivityEnqueueBuffer(
                                    rdata->context, 0,
                                    memalign(CUPTI_ACTIVITY_BUFFER_ALIGNMENT,
                                             CUPTI_ACTIVITY_BUFFER_SIZE),
                                    CUPTI_ACTIVITY_BUFFER_SIZE
                                    ));
                }
                break;



            case CUPTI_CBID_RESOURCE_CONTEXT_DESTROY_STARTING:
                {
#if !defined(NDEBUG)
                    if (debug)
                    {
                        printf("[CBTF/CUDA] destroying context %p\n",
                               rdata->context);
                    }
#endif

                    /* Add messages for this context's activities */
                    add_activities(tls, rdata->context, NULL, 0);

                    /* Add messages for global activities */
                    add_activities(tls, NULL, NULL, 0);
                }
                break;



            case CUPTI_CBID_RESOURCE_STREAM_CREATED:
                {
                    uint32_t stream_id = 0;
                    CUPTI_CHECK(cuptiGetStreamId(rdata->context,
                                                 rdata->resourceHandle.stream,
                                                 &stream_id));
                    
#if !defined(NDEBUG)
                    if (debug)
                    {
                        printf("[CBTF/CUDA] created stream %p in context %p\n",
                               rdata->resourceHandle.stream, rdata->context);
                    }
#endif
                    
                    /* Enqueue a buffer for stream-specific activities */
                    CUPTI_CHECK(cuptiActivityEnqueueBuffer(
                                    rdata->context, stream_id,
                                    memalign(CUPTI_ACTIVITY_BUFFER_ALIGNMENT,
                                             CUPTI_ACTIVITY_BUFFER_SIZE),
                                    CUPTI_ACTIVITY_BUFFER_SIZE
                                    ));
                }
                break;
                


            case CUPTI_CBID_RESOURCE_STREAM_DESTROY_STARTING:
                {
                    uint32_t stream_id = 0;
                    CUPTI_CHECK(cuptiGetStreamId(rdata->context,
                                                 rdata->resourceHandle.stream,
                                                 &stream_id));

#if !defined(NDEBUG)
                    if (debug)
                    {
                        printf("[CBTF/CUDA] "
                               "destroying stream %p in context %p\n",
                               rdata->resourceHandle.stream, rdata->context);
                    }
#endif

                    /* Add messages for this stream's activities */
                    add_activities(tls, rdata->context,
                                   rdata->resourceHandle.stream, stream_id);
                    
                    /* Add messages for global activities */
                    add_activities(tls, NULL, NULL, 0);                    
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
                    if (debug)
                    {
                        printf("[CBTF/CUDA] synchronized context %p\n",
                               sdata->context);
                    }
#endif

                    /* Add messages for this context's activities */
                    add_activities(tls, sdata->context, NULL, 0);

                    /* Add messages for global activities */
                    add_activities(tls, NULL, NULL, 0);
                }
                break;



            case CUPTI_CBID_SYNCHRONIZE_STREAM_SYNCHRONIZED:
                {
                    uint32_t stream_id = 0;
                    CUPTI_CHECK(cuptiGetStreamId(sdata->context,
                                                 sdata->stream,
                                                 &stream_id));
                    
#if !defined(NDEBUG)
                    if (debug)
                    {
                        printf("[CBTF/CUDA] "
                               "synchronized stream %p in context %p\n",
                               sdata->stream, sdata->context);
                    }
#endif
                    
                    /* Add messages for this stream's activities */
                    add_activities(tls, sdata->context,
                                   sdata->stream, stream_id);

                    /* Add messages for global activities */
                    add_activities(tls, NULL, NULL, 0);                    
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
    if (debug)
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
            if (debug)
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
                if (debug)
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
                if (debug)
                {
                    printf("[CBTF/CUDA] parse_configuration(): "
                           "event name = \"%s\"\n", event->name);
                }
#endif
            }

            /* Increment the number of periodically sampled events */
            ++periodic_sampling_count;
            
            /* Setup overflow for this event if a threshold was specified */
            if (event->threshold > 0)
            {
                event_to_overflow_indicies[sampling_config.events.events_len] =
                    overflow_sampling_count++;
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
    if (debug)
    {
        printf("[CBTF/CUDA] cbtf_collector_start()\n");
    }
#endif

    /* Atomically increment the active thread count */

    PTHREAD_CHECK(pthread_mutex_lock(&thread_count.mutex));
    
#if !defined(NDEBUG)
    if (debug)
    {
        printf("[CBTF/CUDA] cbtf_collector_start(): "
               "thread_count.value = %d --> %d\n",
               thread_count.value, thread_count.value + 1);
    }
#endif
    
    if (thread_count.value == 0)
    {
        /* Should debugging be enabled? */
        debug = (getenv("CBTF_DEBUG_COLLECTOR") != NULL);
        
#if !defined(NDEBUG)
        if (debug)
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
        
        /* Enqueue a buffer for CUPTI global activities */
        CUPTI_CHECK(cuptiActivityEnqueueBuffer(
                        NULL, 0,
                        memalign(CUPTI_ACTIVITY_BUFFER_ALIGNMENT,
                                 CUPTI_ACTIVITY_BUFFER_SIZE),
                        CUPTI_ACTIVITY_BUFFER_SIZE
                        ));
        
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

    /* Create, zero-initialize, and access our thread-local storage */
#if defined(USE_EXPLICIT_TLS)
    TLS* tls = malloc(sizeof(TLS));
    Assert(tls != NULL);
    OpenSS_SetTLS(TLSKey, tls);
#else
    TLS* tls = &the_tls;
#endif
    Assert(tls != NULL);
    memset(tls, 0, sizeof(TLS));

    /* Copy the header into our thread-local storage for future use */
    memcpy(&tls->data_header, header, sizeof(CBTF_DataHeader));

    /* Initialize our performance data header and blob */
    initialize_data(tls);

#if defined(PAPI_FOUND)
    /* Initially PAPI data collection is not started */
    tls->papi_event_set = PAPI_NULL;
    
    /* Append the event sampling configuration to our performance data blob */
    if (sampling_config.events.events_len > 0)
    {
        CBTF_cuda_message* raw_message = add_message(tls);
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
    if (debug)
    {
        printf("[CBTF/CUDA] cbtf_collector_pause()\n");
    }
#endif
    
    /* Access our thread-local storage */
#if defined(USE_EXPLICIT_TLS)
    TLS* tls = CBTF_GetTLS(TLSKey);
#else
    TLS* tls = &the_tls;
#endif
    Assert(tls != NULL);

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
    if (debug)
    {
        printf("[CBTF/CUDA] cbtf_collector_resume()\n");
    }
#endif

    /* Access our thread-local storage */
#if defined(USE_EXPLICIT_TLS)
    TLS* tls = CBTF_GetTLS(TLSKey);
#else
    TLS* tls = &the_tls;
#endif
    Assert(tls != NULL);

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
    if (debug)
    {
        printf("[CBTF/CUDA] cbtf_collector_stop()\n");
    }
#endif
    
    /* Access our thread-local storage */
#if defined(USE_EXPLICIT_TLS)
    TLS* tls = CBTF_GetTLS(TLSKey);
#else
    TLS* tls = &the_tls;
#endif
    Assert(tls != NULL);

#if defined(PAPI_FOUND)
    /* Stop PAPI data collection for this thread */
    stop_papi_data_collection(tls);
#endif

    /* Pause (stop) data collection for this thread */
    tls->paused = TRUE;
    
    /* Send any remaining performance data for this thread */
    send_data(tls);
    
    /* Destroy our thread-local storage */
#if defined(USE_EXPLICIT_TLS)
    free(tls);
    OpenSS_SetTLS(TLSKey, NULL);
#endif

    /* Atomically decrement the active thread count */
    
    PTHREAD_CHECK(pthread_mutex_lock(&thread_count.mutex));
    
#if !defined(NDEBUG)
    if (debug)
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
