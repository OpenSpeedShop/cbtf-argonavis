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

#include <KrellInstitute/Services/Assert.h>

#include "CUDA_check.h"
#include "CUPTI_check.h"
#include "CUPTI_context.h"
#include "CUPTI_events.h"
#include "TLS.h"



#if defined(DISABLE_FOR_NOW)
/**
 * Table used to map CUDA context pointers to CUPTI event groups.
 *
 * ... notes on thread safety ...
 */
static struct {
    CUcontext context;
    CUpti_EventGroup eventgroup;
} EventGroups[MAX_CONTEXTS] = { 0 };
#endif



/**
 * Initialize CUPTI events data collection for this process.
 */
void CUPTI_events_initialize()
{
    // ...
}



/**
 * Start events data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be started.
 */
void CUPTI_events_start(CUcontext context)
{
#if defined(DISABLE_FOR_NOW)
    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (EventGroups[i].context != NULL); ++i)
    {
        if (EventsGroups[i].context == context)
        {
            fprintf(stderr, "[CBTF/CUDA] CUPTI_events_start(): "
                    "Redundant call for CUPTI context pointer (%p) encountered!",
                    context);
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

    CUcontext current = NULL;
    CUDA_CHECK(cuCtxGetCurrent(&current));
    
    if (current != context)
    {
        CUDA_CHECK(cuCtxPopCurrent(&current));
        CUDA_CHECK(cuCtxPushCurrent(context));
    }
    
    CUdevice device;
    CUDA_CHECK(cuCtxGetDevice(&device));

    


    CUPTI_CHECK(cuptiSetEventCollectionMode(
                    context, CUPTI_EVENT_COLLECTION_MODE_CONTINUOUS
                    ));

    CUPTI_CHECK(cuptiEventGroupCreate(context, &EventGroups[i].eventgroup, 0));
    
    
    

    if (current != context)
    {
        CUDA_CHECK(cuCtxPopCurrent(&context));
        CUDA_CHECK(cuCtxPushCurrent(current));
    }

    EventGroups[i].context = context;
#endif
}



/**
 * Sample the CUPTI events for all active CUDA contexts.
 */
void CUPTI_events_sample()
{
#if defined(DISABLE_FOR_NOW)
    Assert(sample != NULL);

    CUcontext context = NULL;
    CUDA_CHECK(cuCtxGetCurrent(&context));
    
    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (EventGroups[i].context != NULL); ++i)
    {
        if (EventsGroups[i].context == context)
        {
            break;
        }
    }

    if (i == MAX_CONTEXTS)
    {
        fprintf(stderr, "[CBTF/CUDA] CUPTI_events_sample(): "
                "Unknown CUDA context pointer (%p) encountered!\n", context);
        fflush(stderr);
        abort();
    }

    // ...

#endif
}



/**
 * Stop CUPTI events data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be stopped.
 */
void CUPTI_events_stop(CUcontext context)
{
#if defined(DISABLE_FOR_NOW)
    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (EventGroups[i].context != NULL); ++i)
    {
        if (EventsGroups[i].context == context)
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

    CUPTI_CHECK(cuptiEventGroupDisable(&EventGroups[i].eventgroup));
    CUPTI_CHECK(cuptiEventGroupDestroy(&EventGroups[i].eventgroup));

    EventGroups[i].context = NULL;
#endif
}



/**
 * Finalize CUPTI events data collection for this process.
 */
void CUPTI_events_finalize()
{
    // ...
}
