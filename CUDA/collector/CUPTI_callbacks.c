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

/** @file Definition of CUPTI callback functions. */

#include <cupti.h>
#include <malloc.h>
#include <stdio.h>

#include <KrellInstitute/Services/Assert.h>
#include <KrellInstitute/Services/Time.h>

#include "CUDA_data.h"

#include "collector.h"
#include "CUPTI_activities.h"
#include "CUPTI_callbacks.h"
#include "CUPTI_check.h"
#include "CUPTI_context.h"
#include "CUPTI_stream.h"
#include "PAPI.h"
#include "TLS.h"



/** CUPTI subscriber handle for this collector. */
static CUpti_SubscriberHandle Handle;



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



            case CUPTI_DRIVER_TRACE_CBID_cuLaunchKernel :
                if (cbdata->callbackSite == CUPTI_API_ENTER)
                {
                    const cuLaunchKernel_params* const params =
                        (cuLaunchKernel_params*)cbdata->functionParams;

#if !defined(NDEBUG)
                    if (IsDebugEnabled)
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
                    if (IsDebugEnabled)
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
                    if (IsDebugEnabled)
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
                    if (IsDebugEnabled)
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
                    if (IsDebugEnabled)
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
                    if (IsDebugEnabled)
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
                    if (IsDebugEnabled)
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
                    if (IsDebugEnabled)
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
                        if (IsDebugEnabled)
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
