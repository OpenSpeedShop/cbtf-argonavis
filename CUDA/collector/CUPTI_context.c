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

/** @file Definition of the CUPTI context support functions. */

#include <monitor.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "collector.h"
#include "CUPTI_context.h"
#include "Pthread_check.h"



/** Table used to translate CUPTI context IDs to CUDA context pointers. */
static struct {
    struct {
        uint32_t id;
        CUcontext ptr;
    } values[MAX_CONTEXTS];
    pthread_mutex_t mutex;
} Contexts = { { 0 }, PTHREAD_MUTEX_INITIALIZER };



/**
 * Add the specified mapping of CUPTI context ID to CUDA context pointer.
 *
 * @param id     CUPTI context ID.
 * @param ptr    Corresponding CUDA context pointer.
 */
void CUPTI_context_add(uint32_t id, CUcontext ptr)
{
    PTHREAD_CHECK(pthread_mutex_lock(&Contexts.mutex));

    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Contexts.values[i].ptr != NULL); ++i)
    {
        if (Contexts.values[i].id == id)
        {
            if (Contexts.values[i].ptr != ptr)
            {
                fprintf(stderr, "[CUDA %d:%d] CUPTI_context_add(): "
                        "CUDA context pointer for CUPTI context "
                        "ID %u changed!\n",
                        getpid(), monitor_get_thread_num(), id);
                fflush(stderr);
                abort();
            }

            break;
        }
    }

    if (i == MAX_CONTEXTS)
    {
        fprintf(stderr, "[CUDA %d:%d] CUPTI_context_add(): "
                "Maximum supported CUDA context pointers (%d) was reached!\n",
                getpid(), monitor_get_thread_num(), MAX_CONTEXTS);
        fflush(stderr);
        abort();
    }
    else if (Contexts.values[i].ptr == NULL)
    {
#if !defined(NDEBUG)
        if (IsDebugEnabled)
        {
            printf("[CUDA %d:%d] CUPTI_context_add(%u, %p)\n",
                   getpid(), monitor_get_thread_num(), id, ptr);
        }
#endif
        
        Contexts.values[i].id = id;
        Contexts.values[i].ptr = ptr;
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&Contexts.mutex));
}



/**
 * Find the CUDA context pointer corresponding to the given CUPTI context ID.
 *
 * @param id    CUPTI context ID.
 * @return      Corresponding CUDA context pointer.
 */
CUcontext CUPTI_context_ptr_from_id(uint32_t id)
{
    CUcontext ptr = NULL;

    PTHREAD_CHECK(pthread_mutex_lock(&Contexts.mutex));
    
    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Contexts.values[i].ptr != NULL); ++i)
    {
        if (Contexts.values[i].id == id)
        {
            ptr = Contexts.values[i].ptr;
            break;
        }
    }

    if (i == MAX_CONTEXTS)
    {
        fprintf(stderr, "[CUDA %d:%d] CUPTI_context_ptr_from_id(): "
                "Unknown CUPTI context ID (%u) encountered!\n",
                getpid(), monitor_get_thread_num(), id);
        fflush(stderr);
        abort();
    }

    PTHREAD_CHECK(pthread_mutex_unlock(&Contexts.mutex));
    
    return ptr;
}



/**
 * Find the CUPTI context ID corresponding to the given CUDA context pointer.
 *
 * @param ptr    CUDA context pointer.
 * @return       Corresponding CUPTI context ID.
 */
uint32_t CUPTI_context_id_from_ptr(CUcontext ptr)
{
    uint32_t id = 0;

    PTHREAD_CHECK(pthread_mutex_lock(&Contexts.mutex));
    
    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Contexts.values[i].ptr != NULL); ++i)
    {
        if (Contexts.values[i].ptr == ptr)
        {
            id = Contexts.values[i].id;
            break;
        }
    }

    if (i == MAX_CONTEXTS)
    {
        fprintf(stderr, "[CUDA %d:%d] CUPTI_context_id_from_ptr(): "
                "Unknown CUDA context pointer (%p) encountered!\n",
                getpid(), monitor_get_thread_num(), ptr);
        fflush(stderr);
        abort();
    }

    PTHREAD_CHECK(pthread_mutex_unlock(&Contexts.mutex));
    
    return id;
}
