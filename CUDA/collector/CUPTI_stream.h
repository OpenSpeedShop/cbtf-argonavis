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

/** @file Declaration of the CUPTI stream support functions.
 *
 * When referring to a particular CUDA stream, CUPTI will sometimes use the
 * CUDA stream pointer, and sometimes a unique CUPTI stream ID. Frequently
 * one is provided but not the other. And of course the other value is often
 * required to implement a particular operation. Recent versions of CUPTI now
 * provide a cuptiGetStreamId() function. But this is not provided in every
 * version and the opposite is not available in any version. These functions
 * provide a manual mechanism for tracking this correspondence.
 *
 * @note    The functions defined in this header can be safely called
 *          concurrently by multiple threads.
 */

#pragma once

#include <cupti.h>
#include <inttypes.h>

/** 
 * Maximum supported number of CUDA streams. Controls the size of the table
 * used to translate between CUPTI stream IDs and CUDA stream pointers.
 *
 * @note    Currently there is no specific basis for the selection of this
 *          value other than testing indicates it is usually sufficient.
 */
#define MAX_STREAMS 1024

/*
 * Add the specified mapping of CUPTI stream ID to CUDA stream pointer.
 *
 * @param id     CUPTI stream ID.
 * @param ptr    Corresponding CUDA stream pointer.
 */
void CUPTI_stream_add(uint32_t id, CUstream ptr);

/*
 * Find the CUDA stream pointer corresponding to the given CUPTI stream ID.
 *
 * @param id    CUPTI stream ID.
 * @return      Corresponding CUDA stream pointer.
 */
CUstream CUPTI_stream_ptr_from_id(uint32_t id);

/*
 * Find the CUPTI stream ID corresponding to the given CUDA stream pointer.
 *
 * @param ptr    CUDA stream pointer.
 * @return       Corresponding CUPTI stream ID.
 */
uint32_t CUPTI_stream_id_from_ptr(CUstream ptr);
