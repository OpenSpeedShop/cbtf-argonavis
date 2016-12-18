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

/** @file Definition of the CUPTI stream support functions. */

#include <monitor.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "collector.h"
#include "CUPTI_stream.h"
#include "Pthread_check.h"



/** Table used to translate CUPTI stream IDs to CUDA stream pointers. */
static struct {
    struct {
        uint32_t id;
        CUstream ptr;
    } values[MAX_STREAMS];
    pthread_mutex_t mutex;
} Streams = { { 0 }, PTHREAD_MUTEX_INITIALIZER };



/**
 * Add the specified mapping of CUPTI stream ID to CUDA stream pointer.
 *
 * @param id     CUPTI stream ID.
 * @param ptr    Corresponding CUDA stream pointer.
 */
void CUPTI_stream_add(uint32_t id, CUstream ptr)
{
    PTHREAD_CHECK(pthread_mutex_lock(&Streams.mutex));

    int i;
    for (i = 0; (i < MAX_STREAMS) && (Streams.values[i].ptr != NULL); ++i)
    {
        if (Streams.values[i].id == id)
        {
            if (Streams.values[i].ptr != ptr)
            {
                fprintf(stderr, "[CUDA %d:%d] CUPTI_stream_add(): "
                        "CUDA stream pointer for CUPTI stream "
                        "ID %u changed!\n",
                        getpid(), monitor_get_thread_num(), id);
                fflush(stderr);
                abort();
            }

            break;
        }
    }

    if (i == MAX_STREAMS)
    {
        fprintf(stderr, "[CUDA %d:%d] CUPTI_stream_add(): "
                "Maximum supported CUDA stream pointers (%d) was reached!\n",
                getpid(), monitor_get_thread_num(), MAX_STREAMS);
        fflush(stderr);
        abort();
    }
    else if (Streams.values[i].ptr == NULL)
    {
#if !defined(NDEBUG)
        if (IsDebugEnabled)
        {
            printf("[CUDA %d:%d] CUPTI_stream_add(%u, %p)\n",
                   getpid(), monitor_get_thread_num(), id, ptr);
        }
#endif
        
        Streams.values[i].id = id;
        Streams.values[i].ptr = ptr;
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&Streams.mutex));
}



/**
 * Find the CUDA stream pointer corresponding to the given CUPTI stream ID.
 *
 * @param id    CUPTI stream ID.
 * @return      Corresponding CUDA stream pointer.
 */
CUstream CUPTI_stream_ptr_from_id(uint32_t id)
{
    CUstream ptr = NULL;

    PTHREAD_CHECK(pthread_mutex_lock(&Streams.mutex));
    
    int i;
    for (i = 0; (i < MAX_STREAMS) && (Streams.values[i].ptr != NULL); ++i)
    {
        if (Streams.values[i].id == id)
        {
            ptr = Streams.values[i].ptr;
            break;
        }
    }

    if (i == MAX_STREAMS)
    {
        fprintf(stderr, "[CUDA %d:%d] CUPTI_stream_ptr_from_id(): "
                "Unknown CUPTI stream ID (%u) encountered!\n",
                getpid(), monitor_get_thread_num(), id);
        fflush(stderr);
        abort();
    }

    PTHREAD_CHECK(pthread_mutex_unlock(&Streams.mutex));
    
    return ptr;
}



/**
 * Find the CUPTI stream ID corresponding to the given CUDA stream pointer.
 *
 * @param ptr    CUDA stream pointer.
 * @return       Corresponding CUPTI stream ID.
 */
uint32_t CUPTI_stream_id_from_ptr(CUstream ptr)
{
    uint32_t id = 0;

    PTHREAD_CHECK(pthread_mutex_lock(&Streams.mutex));
    
    int i;
    for (i = 0; (i < MAX_STREAMS) && (Streams.values[i].ptr != NULL); ++i)
    {
        if (Streams.values[i].ptr == ptr)
        {
            id = Streams.values[i].id;
            break;
        }
    }

    if (i == MAX_STREAMS)
    {
        fprintf(stderr, "[CUDA %d:%d] CUPTI_stream_id_from_ptr(): "
                "Unknown CUDA stream pointer (%p) encountered!\n",
                getpid(), monitor_get_thread_num(), ptr);
        fflush(stderr);
        abort();
    }

    PTHREAD_CHECK(pthread_mutex_unlock(&Streams.mutex));
    
    return id;
}
