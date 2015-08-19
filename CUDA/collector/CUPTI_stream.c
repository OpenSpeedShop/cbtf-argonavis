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

/** @file Definition of the CUPTI stream support functions. */

#include <pthread.h>

#include "CUPTI_stream.h"
#include "Pthread_check.h"



/** 
 * Maximum supported number of CUDA streams. Controls the size of the table
 * used to translate between CUPTI stream IDs and CUDA stream pointers.
 *
 * @note    Currently there is no specific basis for the selection of this
 *          value other than testing indicates it is usually sufficient.
 */
#define MAX_STREAMS 32



/** Table used to translate CUPTI stream IDs to CUDA stream pointers. */
struct {
    uint32_t id;
    CUstream ptr;
} Table[MAX_STREAMS];

/** Mutex controlling access to Table. */
pthread_mutex_t Mutex;



/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void CUPTI_stream_add(uint32_t id, CUstream ptr)
{
    PTHREAD_CHECK(pthread_mutex_lock(&Mutex));

    int i;
    for (i = 0; (i < MAX_STREAMS) && (Table[i].ptr != NULL); ++i)
    {
        if (Table[i].id == id)
        {
            if (Table[i].ptr != ptr)
            {
                fprintf(stderr, "[CBTF/CUDA] CUPTI_stream_add(): "
                        "CUDA stream pointer for CUPTI stream "
                        "ID %u changed!\n", id);
                fflush(stderr);
                abort();
            }

            break;
        }
    }

    if (i == MAX_STREAMS)
    {
        fprintf(stderr, "[CBTF/CUDA] CUPTI_stream_add(): "
                "Maximum supported CUDA stream pointers (%d) was reached!\n",
                MAX_STREAMS);
        fflush(stderr);
        abort();
    }
    else if (Table[i].ptr == NULL)
    {
        Table[i].id = id;
        Table[i].ptr = ptr;
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&Mutex));
}



/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
CUstream CUPTI_stream_ptr_from_id(uint32_t id)
{
    CUstream ptr = NULL;

    PTHREAD_CHECK(pthread_mutex_lock(&Mutex));
    
    int i;
    for (i = 0; (i < MAX_STREAMS) && (Table[i].ptr != NULL); ++i)
    {
        if (Table[i].id == id)
        {
            ptr = Table[i].ptr;
            break;
        }
    }

    if (i == MAX_STREAMS)
    {
        fprintf(stderr, "[CBTF/CUDA] CUPTI_stream_ptr_from_id(): "
                "Unknown CUPTI stream ID (%u) encountered!\n", id);
        fflush(stderr);
        abort();
    }

    PTHREAD_CHECK(pthread_mutex_unlock(&Mutex));
    
    return ptr;
}



/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
uint32_t CUPTI_stream_id_from_ptr(CUstream ptr)
{
    uint32_t id = 0;

    PTHREAD_CHECK(pthread_mutex_lock(&Mutex));
    
    int i;
    for (i = 0; (i < MAX_STREAMS) && (Table[i].ptr != NULL); ++i)
    {
        if (Table[i].ptr == ptr)
        {
            id = Table[i].id;
            break;
        }
    }

    if (i == MAX_STREAMS)
    {
        fprintf(stderr, "[CBTF/CUDA] CUPTI_stream_id_from_ptr(): "
                "Unknown CUDA stream pointer (%p) encountered!\n", ptr);
        fflush(stderr);
        abort();
    }

    PTHREAD_CHECK(pthread_mutex_unlock(&Mutex));
    
    return id;
}
