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

/** @file Definition of the CUPTI context support functions. */

#include <pthread.h>

#include "CUPTI_context.h"
#include "Pthread_check.h"



/** 
 * Maximum supported number of CUDA contexts. Controls the size of the table
 * used to translate between CUPTI context IDs and CUDA context pointers.
 *
 * @note    Currently there is no specific basis for the selection of this
 *          value other than testing indicates it is usually sufficient.
 */
#define MAX_CONTEXTS 32



/** Table used to translate CUPTI context IDs to CUDA context pointers. */
struct {
    uint32_t id;
    CUcontext ptr;
} Table[MAX_CONTEXTS];

/** Mutex controlling access to Table. */
pthread_mutex_t Mutex;



/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
void CUPTI_context_add(uint32_t id, CUcontext ptr)
{
    PTHREAD_CHECK(pthread_mutex_lock(&Mutex));

    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Table[i].ptr != NULL); ++i)
    {
        if (Table[i].id == id)
        {
            if (Table[i].ptr != ptr)
            {
                fprintf(stderr, "[CBTF/CUDA] CUPTI_context_add(): "
                        "CUDA context pointer for CUPTI context "
                        "ID %u changed!\n", id);
                fflush(stderr);
                abort();
            }

            break;
        }
    }

    if (i == MAX_CONTEXTS)
    {
        fprintf(stderr, "[CBTF/CUDA] CUPTI_context_add(): "
                "Maximum supported CUDA context pointers (%d) was reached!\n",
                MAX_CONTEXTS);
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
CUcontext CUPTI_context_ptr_from_id(uint32_t id)
{
    CUcontext ptr = NULL;

    PTHREAD_CHECK(pthread_mutex_lock(&Mutex));
    
    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Table[i].ptr != NULL); ++i)
    {
        if (Table[i].id == id)
        {
            ptr = Table[i].ptr;
            break;
        }
    }

    if (i == MAX_CONTEXTS)
    {
        fprintf(stderr, "[CBTF/CUDA] CUPTI_context_ptr_from_id(): "
                "Unknown CUPTI context ID (%u) encountered!\n", id);
        fflush(stderr);
        abort();
    }

    PTHREAD_CHECK(pthread_mutex_unlock(&Mutex));
    
    return ptr;
}



/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
uint32_t CUPTI_context_id_from_ptr(CUcontext ptr)
{
    uint32_t id = 0;

    PTHREAD_CHECK(pthread_mutex_lock(&Mutex));
    
    int i;
    for (i = 0; (i < MAX_CONTEXTS) && (Table[i].ptr != NULL); ++i)
    {
        if (Table[i].ptr == ptr)
        {
            id = Table[i].id;
            break;
        }
    }

    if (i == MAX_CONTEXTS)
    {
        fprintf(stderr, "[CBTF/CUDA] CUPTI_context_id_from_ptr(): "
                "Unknown CUDA context pointer (%p) encountered!\n", ptr);
        fflush(stderr);
        abort();
    }

    PTHREAD_CHECK(pthread_mutex_unlock(&Mutex));
    
    return id;
}
