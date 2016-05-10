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

#include <KrellInstitute/Messages/DataHeader.h>

#include <KrellInstitute/Services/Assert.h>

#include "CUDA_check.h"
#include "CUPTI_check.h"
#include "CUPTI_context.h"
#include "CUPTI_events.h"
#include "Pthread_check.h"
#include "TLS.h"



/**
 * Performance data header for this process. Used when constructing the fake
 * thread-local storage for each CUDA context.
 */
CBTF_DataHeader DataHeader;

/**
 * Table used to map CUDA context pointers to CUPTI event groups and fake
 * thread-local storage. This table must be protected with a mutex because
 * there is no guarantee that a CUDA context won't be created or destroyed
 * at the same time that a sample is taken.
 */
static struct {
    struct {
        CUcontext context;
        CUpti_EventGroup eventgroup;
        TLS tls;
    } values[MAX_CONTEXTS];
    pthread_mutex_t mutex;
} EventGroups = { { 0 }, PTHREAD_MUTEX_INITIALIZER };



/**
 * Initialize CUPTI events data collection for this process.
 */
void CUPTI_events_initialize()
{
    /* Access our thread-local storage */
    TLS* tls = TLS_get();
    
    /* Initialize the performance data header for this process */
    memcpy(&DataHeader, &tls->data_header, sizeof(CBTF_DataHeader));
}



/**
 * Start events data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be started.
 */
void CUPTI_events_start(CUcontext context)
{
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

    /*
     * ...
     */
    
    CUPTI_CHECK(cuptiSetEventCollectionMode(
                    context, CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS
                    ));
    
    CUPTI_CHECK(cuptiEventGroupCreate(
                    context, &EventGroups.values[i].eventgroup, 0
                    ));
    
    CUdevice device;
    CUDA_CHECK(cuCtxGetDevice(&device));

    // ...
    
    CUPTI_CHECK(cuptiEventGroupEnable(&EventGroups.values[i].eventgroup));
    
    /* Initialize this context's fake thread-local storage */
    memset(&EventGroups.values[i].tls, 0, sizeof(TLS));
    memcpy(&EventGroups.values[i].tls.data_header, &DataHeader,
           sizeof(CBTF_DataHeader));
    EventGroups.values[i].tls.data_header.posix_tid = (int64_t)context;
    TLS_initialize_data(&EventGroups.values[i].tls);
    
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
 */
void CUPTI_events_sample()
{
    PTHREAD_CHECK(pthread_mutex_lock(&EventGroups.mutex));

    // ...

    PTHREAD_CHECK(pthread_mutex_unlock(&EventGroups.mutex));
}



/**
 * Stop CUPTI events data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be stopped.
 */
void CUPTI_events_stop(CUcontext context)
{
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

    /*
     * ...
     */

    CUPTI_CHECK(cuptiEventGroupDisable(&EventGroups.values[i].eventgroup));
    CUPTI_CHECK(cuptiEventGroupDestroy(&EventGroups.values[i].eventgroup));

    PTHREAD_CHECK(pthread_mutex_unlock(&EventGroups.mutex));
}



/**
 * Finalize CUPTI events data collection for this process.
 */
void CUPTI_events_finalize()
{
    // ...
}
