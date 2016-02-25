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

/** @file Definition of CUPTI callback functions. */

#include <cupti.h>
#include <malloc.h>
#include <stdio.h>

#include <KrellInstitute/Messages/CUDA_data.h>

#include <KrellInstitute/Services/Assert.h>
#include <KrellInstitute/Services/Time.h>

#include "collector.h"
#include "CUPTI_activities.h"
#include "CUPTI_callbacks.h"
#include "CUPTI_check.h"
#include "CUPTI_context.h"
#include "CUPTI_stream.h"
#include "PAPI.h"
#include "TLS.h"



#if (CUPTI_API_VERSION < 4)
/**
 * Size (in bytes) of each allocated CUPTI activity buffer.
 *
 * @note    Currently the only basis for the selection of this value is that the
 *          CUPTI "activity_trace_async.cpp" example uses buffers of 32 KB each.
 */
#define CUPTI_ACTIVITY_BUFFER_SIZE (32 * 1024 /* 32 KB */)
#endif



/** CUPTI subscriber handle for this collector. */
static CUpti_SubscriberHandle Handle;



/**
 * Get the CUDA stream pointer from the parameters passed into a driver API
 * domain callback. A null pointer is returned if the specified parameters
 * do not refer to a CUDA stream pointer.
 *
 * @param id            ID of the callback.
 * @param raw_params    Parameters passed to the callback.
 * @return              CUDA stream pointer referenced by those parameters.
 */
static CUstream get_stream(CUpti_CallbackId id, const void* raw_params)
{
    switch (id)
    {

    case CUPTI_DRIVER_TRACE_CBID_cuLaunchGridAsync:
        return ((cuLaunchGridAsync_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel:
        return ((cuLaunchKernel_params*)raw_params)->hStream;
#if (CUPTI_API_VERSION >= 8)
    case CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel_ptsz:
        return ((cuLaunchKernel_ptsz_params*)raw_params)->hStream;        
#endif

    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoDAsync:
        return ((cuMemcpyHtoDAsync_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoHAsync:
        return ((cuMemcpyDtoHAsync_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoDAsync:
        return ((cuMemcpyDtoDAsync_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoAAsync:
        return ((cuMemcpyHtoAAsync_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoHAsync:
        return ((cuMemcpyAtoHAsync_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DAsync:
        return ((cuMemcpy2DAsync_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DAsync:
        return ((cuMemcpy3DAsync_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoDAsync_v2:
        return ((cuMemcpyHtoDAsync_v2_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoHAsync_v2:
        return ((cuMemcpyDtoHAsync_v2_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoDAsync_v2:
        return ((cuMemcpyDtoDAsync_v2_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoHAsync_v2:
        return ((cuMemcpyAtoHAsync_v2_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DAsync_v2:
        return ((cuMemcpy2DAsync_v2_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DAsync_v2:
        return ((cuMemcpy3DAsync_v2_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoAAsync_v2:
        return ((cuMemcpyHtoAAsync_v2_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAsync:
        return ((cuMemcpyAsync_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyPeerAsync:
        return ((cuMemcpyPeerAsync_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DPeerAsync:
        return ((cuMemcpy3DPeerAsync_params*)raw_params)->hStream;
#if (CUPTI_API_VERSION >= 8)
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAsync_ptsz:
        return ((cuMemcpyAsync_ptsz_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoAAsync_v2_ptsz:
        return ((cuMemcpyHtoAAsync_v2_ptsz_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoHAsync_v2_ptsz:
        return ((cuMemcpyAtoHAsync_v2_ptsz_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoDAsync_v2_ptsz:
        return ((cuMemcpyHtoDAsync_v2_ptsz_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoHAsync_v2_ptsz:
        return ((cuMemcpyDtoHAsync_v2_ptsz_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoDAsync_v2_ptsz:
        return ((cuMemcpyDtoDAsync_v2_ptsz_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DAsync_v2_ptsz:
        return ((cuMemcpy2DAsync_v2_ptsz_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DAsync_v2_ptsz:
        return ((cuMemcpy3DAsync_v2_ptsz_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpyPeerAsync_ptsz:
        return ((cuMemcpyPeerAsync_ptsz_params*)raw_params)->hStream;
    case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DPeerAsync_ptsz:
        return ((cuMemcpy3DPeerAsync_ptsz_params*)raw_params)->hStream;
#endif
        
    default:
        return NULL;
    }
}



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
static void callback(void* userdata,
                     CUpti_CallbackDomain domain,
                     CUpti_CallbackId id,
                     const void* data)
{
    /* Is a CUDA context being created? */
    if ((domain == CUPTI_CB_DOMAIN_RESOURCE) &&
        (id == CUPTI_CBID_RESOURCE_CONTEXT_CREATED))
    {
        /* Start (initialize) PAPI */
        PAPI_notify_cuda_context_created();

        /* Start PAPI data collection for this thread */
        PAPI_start_data_collection();
    }
    
    /* Is a CUDA context being destroyed? */
    if ((domain == CUPTI_CB_DOMAIN_RESOURCE) &&
        (id == CUPTI_CBID_RESOURCE_CONTEXT_DESTROY_STARTING))
    {
        /* Stop (shutdown) PAPI */
        PAPI_notify_cuda_context_destroyed();
    }

    /* Access our thread-local storage */
    TLS* tls = TLS_get();

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


            case CUPTI_DRIVER_TRACE_CBID_cuLaunch:
            case CUPTI_DRIVER_TRACE_CBID_cuLaunchGrid:
            case CUPTI_DRIVER_TRACE_CBID_cuLaunchGridAsync:                
            case CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel:
#if (CUPTI_API_VERSION >= 8)
            case CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel_ptsz:
#endif
                if (cbdata->callbackSite == CUPTI_API_ENTER)
                {
#if !defined(NDEBUG)
                    if (IsDebugEnabled)
                    {
                        printf("[CBTF/CUDA] enter cuLaunchKernel()\n");
                    }
#endif

                    /* Add the context ID to pointer mapping */
                    CUPTI_context_add(cbdata->contextUid, cbdata->context);
                    
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = TLS_add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = EnqueueExec;
                    
                    CUDA_EnqueueExec* message = 
                        &raw_message->CBTF_cuda_message_u.enqueue_exec;

                    message->id = cbdata->correlationId;
                    message->context = (CBTF_Protocol_Address)cbdata->context;
                    message->stream = (CBTF_Protocol_Address)get_stream(
                        id, cbdata->functionParams
                        );
                    message->time = CBTF_GetTime();
                    message->call_site = TLS_add_current_call_site(tls);
                    
                    TLS_update_header_with_time(tls, message->time);
                }
                break;



            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoD:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoH:                
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoD:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoA:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoD:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoA:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoH:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoA:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2D:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DUnaligned:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3D:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoDAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoHAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoDAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoAAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoHAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoD_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoDAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoH_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoHAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoD_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoDAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoH_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoHAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoD_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoA_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoA_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2D_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DUnaligned_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3D_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoA_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoAAsync_v2:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyPeer:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyPeerAsync:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DPeer:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DPeerAsync:
#if (CUPTI_API_VERSION >= 8)
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoD_v2_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoH_v2_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoD_v2_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoA_v2_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoD_v2_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoA_v2_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoH_v2_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoA_v2_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2D_v2_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DUnaligned_v2_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3D_v2_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyPeer_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DPeer_ptds:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAsync_ptsz:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoAAsync_v2_ptsz:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyAtoHAsync_v2_ptsz:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyHtoDAsync_v2_ptsz:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoHAsync_v2_ptsz:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyDtoDAsync_v2_ptsz:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy2DAsync_v2_ptsz:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DAsync_v2_ptsz:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpyPeerAsync_ptsz:
            case CUPTI_DRIVER_TRACE_CBID_cuMemcpy3DPeerAsync_ptsz:
#endif
                if (cbdata->callbackSite == CUPTI_API_ENTER)
                {
#if !defined(NDEBUG)
                    if (IsDebugEnabled)
                    {
                        printf("[CBTF/CUDA] enter cuMemcpy*()\n");
                    }
#endif

                    /* Add the context ID to pointer mapping */
                    CUPTI_context_add(cbdata->contextUid, cbdata->context);
                                        
                    /* Add a message for this event */
                    
                    CBTF_cuda_message* raw_message = TLS_add_message(tls);
                    Assert(raw_message != NULL);
                    raw_message->type = EnqueueXfer;
                    
                    CUDA_EnqueueXfer* message = 
                        &raw_message->CBTF_cuda_message_u.enqueue_xfer;
                    
                    message->id = cbdata->correlationId;
                    message->context = (CBTF_Protocol_Address)cbdata->context;
                    message->stream = (CBTF_Protocol_Address)get_stream(
                        id, cbdata->functionParams
                        );
                    message->time = CBTF_GetTime();
                    message->call_site = TLS_add_current_call_site(tls);
                    
                    TLS_update_header_with_time(tls, message->time);
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
                    if (IsDebugEnabled)
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
#endif
                }
                break;



            case CUPTI_CBID_RESOURCE_CONTEXT_DESTROY_STARTING:
                {
#if !defined(NDEBUG)
                    if (IsDebugEnabled)
                    {
                        printf("[CBTF/CUDA] destroying context %p\n",
                               rdata->context);
                    }
#endif

#if (CUPTI_API_VERSION < 4)
                    /* Add this context's activities */
                    CUPTI_activities_add(tls, rdata->context, NULL);
                    
                    /* Add the global activities */
                    CUPTI_activities_add(tls, NULL, NULL);
#else
                    /* Flush all activities */
                    CUPTI_activities_flush();
#endif
                }
                break;



            case CUPTI_CBID_RESOURCE_STREAM_CREATED:
                {
#if !defined(NDEBUG)
                    if (IsDebugEnabled)
                    {
                        printf("[CBTF/CUDA] created stream %p in context %p\n",
                               rdata->resourceHandle.stream, rdata->context);
                    }
#endif

                    /* Add the stream ID to pointer mapping */

                    uint32_t stream_id = 0;
                    CUPTI_CHECK(cuptiGetStreamId(rdata->context,
                                                 rdata->resourceHandle.stream,
                                                 &stream_id));

                    CUPTI_stream_add(stream_id, rdata->resourceHandle.stream);
                                        
#if (CUPTI_API_VERSION < 4)
                    /* Enqueue a buffer for stream-specific activities */
                    CUPTI_CHECK(cuptiActivityEnqueueBuffer(
                                    rdata->context, stream_id,
                                    memalign(ACTIVITY_RECORD_ALIGNMENT,
                                             CUPTI_ACTIVITY_BUFFER_SIZE),
                                    CUPTI_ACTIVITY_BUFFER_SIZE
                                    ));
#endif
                }
                break;
                


            case CUPTI_CBID_RESOURCE_STREAM_DESTROY_STARTING:
                {
#if !defined(NDEBUG)
                    if (IsDebugEnabled)
                    {
                        printf("[CBTF/CUDA] "
                               "destroying stream %p in context %p\n",
                               rdata->resourceHandle.stream, rdata->context);
                    }
#endif

#if (CUPTI_API_VERSION < 4)
                    /* Add this stream's activities */
                    CUPTI_activities_add(tls, rdata->context,
                                         rdata->resourceHandle.stream);
                    
                    /* Add the global activities */
                    CUPTI_activities_add(tls, NULL, NULL);
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
                    if (IsDebugEnabled)
                    {
                        printf("[CBTF/CUDA] synchronized context %p\n",
                               sdata->context);
                    }
#endif

#if (CUPTI_API_VERSION < 4)
                    /* Add this context's activities */
                    CUPTI_activities_add(tls, sdata->context, NULL);
                    
                    /* Add the global activities */
                    CUPTI_activities_add(tls, NULL, NULL);
#endif
                }
                break;



            case CUPTI_CBID_SYNCHRONIZE_STREAM_SYNCHRONIZED:
                {
#if !defined(NDEBUG)
                    if (IsDebugEnabled)
                    {
                        printf("[CBTF/CUDA] "
                               "synchronized stream %p in context %p\n",
                               sdata->stream, sdata->context);
                    }
#endif
                    
#if (CUPTI_API_VERSION < 4)
                    /* Add this stream's activities */
                    CUPTI_activities_add(tls, sdata->context, sdata->stream);
                    
                    /* Add the global activities */
                    CUPTI_activities_add(tls, NULL, NULL);
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



/**
 * Subscribe to CUPTI callbacks for this process.
 */
void CUPTI_callbacks_subscribe()
{
    CUPTI_CHECK(cuptiSubscribe(&Handle, callback, NULL));    
    CUPTI_CHECK(cuptiEnableDomain(1, Handle, CUPTI_CB_DOMAIN_DRIVER_API));    
    CUPTI_CHECK(cuptiEnableDomain(1, Handle, CUPTI_CB_DOMAIN_RESOURCE));    
    CUPTI_CHECK(cuptiEnableDomain(1, Handle, CUPTI_CB_DOMAIN_SYNCHRONIZE));
}



/**
 * Unsubscribe to CUPTI callbacks for this process.
 */
void CUPTI_callbacks_unsubscribe()
{
    CUPTI_CHECK(cuptiUnsubscribe(Handle));
}
