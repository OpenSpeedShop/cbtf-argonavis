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

/** @file Declaration of the TLS data structure and support functions. */

#pragma once

#include <inttypes.h>
#include <stdbool.h>

#include <KrellInstitute/Messages/CUDA_data.h>
#include <KrellInstitute/Messages/DataHeader.h>

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

/*
 * The SHOC-MaxFlops benchmark was crashing on NASA Pleiades, and the cause
 * appears to be that this benchmark runs for a long time without intervening
 * CUDA calls, generating a full 32 KB buffer of deltas. And the encoding of
 * these fails, crashing CBTF_MRNet_Send(). Dropping this constant to 8 KB
 * seems to fix the issue. But is not really a great solution. So this issue
 * should be revisited in the future...
 *
 * WDH 2017-APR-4
 */
#undef MAX_DELTAS_BYTES_PER_BLOB
#define MAX_DELTAS_BYTES_PER_BLOB (8 * 1024 /* 8 KB */)


/**
 * Maximum supported number of concurrently sampled events. Controls the fixed
 * size of several of the tables related to event sampling.
 *
 * @note    This value should not be construed to be the actual number of
 *          concurrent events supported by any particular hardware. It is
 *          the maximum supported by this performance data collector.
 */
#define MAX_EVENTS 32

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

/** Type defining the data stored for each overflow event sample. */
typedef struct {
    CBTF_Protocol_Time time;     /**< Time at which the sample was taken. */
    uint64_t pc;                 /**< Program counter (PC) address of sample. */
    bool overflowed[MAX_EVENTS]; /**< Flag for each sampled event. */
} OverflowSample;

/** Type defining the data stored for each periodic event sample. */
typedef struct {
    CBTF_Protocol_Time time;    /**< Time at which the sample was taken. */
    uint64_t count[MAX_EVENTS]; /**< Count for each sampled event. */
} PeriodicSample;

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

    /** Current overflow samples for this thread. */
    struct {

        /** Message containing the overflow event samples. */
        CUDA_OverflowSamples message;

        /** Program counter (PC) addresses. Pointed to by the above message. */
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

        /** Message containing the periodic event samples. */
        CUDA_PeriodicSamples message;
        
        /** Time and event count deltas. Pointed to by the above message. */
        uint8_t deltas[MAX_DELTAS_BYTES_PER_BLOB];
        
        /** Previously taken event sample. */
        PeriodicSample previous;
        
    } periodic_samples;

#if defined(PAPI_FOUND)
    /** Number of PAPI event sets for this thread. */
    int papi_event_set_count;
    
    /** PAPI event sets for this thread. */
    struct {

        /** Handle for the component collecting this event set. */
        int component;
        
        /** Handle for this event set. */
        int event_set;

        /** Number of events in this event set. */
        int event_count;

        /** Map event indicies in this event set to periodic count indicies. */
        int event_to_periodic[MAX_EVENTS];

        /** Map event indicies in this event set to overflow count indicies. */
        int event_to_overflow[MAX_EVENTS];
        
    } papi_event_sets[MAX_EVENTS];
#endif
   
} TLS;

/*
 * Allocate and zero-initialize the thread-local storage for the current thread.
 * This function <em>must</em> be called by a thread before that thread attempts
 * to call any of this file's other functions or memory corruption will result!
 */
void TLS_initialize();

/* Destroy the thread-local storage for the current thread. */
void TLS_destroy();

/*
 * Access the thread-local storage for the current thread.
 *
 * @return    Thread-local storage for the current thread.
 */
TLS* TLS_get();

/*
 * Initialize the performance data header and blob contained within the given
 * thread-local storage. This function <em>must</em> be called before any of
 * the collection routines attempts to add a message.
 *
 * @param tls    Thread-local storage to be initialized.
 */
void TLS_initialize_data(TLS* tls);

/*
 * Send the performance data blob contained within the given thread-local
 * storage. The blob is re-initialized (cleared) after being sent. Nothing
 * is sent if the blob is empty.
 *
 * @param tls    Thread-local storage containing data to be sent.
 */
void TLS_send_data(TLS* tls);

/*
 * Add a new message to the performance data blob contained within the given
 * thread-local storage. The current blob is sent and re-initialized (cleared)
 * if it is already full.
 *
 * @param tls    Thread-local storage to which a message is to be added.
 * @return       Pointer to the new message to be filled in by the caller.
 */
CBTF_cuda_message* TLS_add_message(TLS* tls);

/*
 * Update the performance data header contained within the given thread-local
 * storage with the specified time. Insures that the time interval defined by
 * time_begin and time_end contain the specified time.
 *
 * @param tls     Thread-local storage to be updated.
 * @param time    Time with which to update.
 */
void TLS_update_header_with_time(TLS* tls, CBTF_Protocol_Time time);

/*
 * Update the performance data header contained within the given thread-local
 * storage with the specified address. Insures that the address range defined
 * by addr_begin and addr_end contain the specified address.
 *
 * @param tls     Thread-local storage to be updated.
 * @param addr    Address with which to update.
 */
void TLS_update_header_with_address(TLS* tls, CBTF_Protocol_Address addr);

/*
 * Add a new stack trace for the current call site to the performance data
 * blob contained within the given thread-local storage.
 *
 * @param tls    Thread-local storage to which the stack trace is to be added.
 * @return       Index of this call site within the performance data blob.
 *
 * @note    Call sites are always referenced by a message. And since cross-blob
 *          references aren't supported, a crash is all but certain if the call
 *          site and its referencing message were to be split across two blobs.
 *          So this function also insures there is room in the current blob for
 *          at least one more message before adding the call site.
 */
uint32_t TLS_add_current_call_site(TLS* tls);

/*
 * Add the specified overflow sample to the performance data blob contained
 * within the given thread-local storage. The current blob is sent and re-
 * initialized (cleared) if it is already full.
 *
 * @param tls       Thread-local storage to which the sample is to be added.
 * @param sample    Overflow sample to be added.
 */
void TLS_add_overflow_sample(TLS* tls, OverflowSample* sample);

/*
 * Add the specified periodic sample to the performance data blob contained
 * within the given thread-local storage. The current blob is sent and re-
 * initialized (cleared) if it is already full.
 *
 * @param tls       Thread-local storage to which the sample is to be added.
 * @param sample    Periodic sample to be added.
 */
void TLS_add_periodic_sample(TLS* tls, PeriodicSample* sample);
