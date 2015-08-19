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

/** @file Declaration of the CUPTI context support functions.
 *
 * When referring to a particular CUDA context, CUPTI will sometimes use the
 * CUDA context pointer, and sometimes a unique CUPTI context ID. Frequently
 * one is provided but not the other. And of course the other value is often
 * required to implement a particular operation. Recent versions of CUPTI now
 * provide a cuptiGetContextId() function. But this is not provided in every
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
 * Add the specified mapping of CUPTI context ID to CUDA context pointer.
 *
 * @param id     CUPTI context ID.
 * @param ptr    Corresponding CUDA context pointer.
 */
extern void CUPTI_context_add(uint32_t id, CUcontext ptr);

/**
 * Find the CUDA context pointer corresponding to the given CUPTI context ID.
 *
 * @param id    CUPTI context ID.
 * @return      Corresponding CUDA context pointer.
 */
extern CUcontext CUPTI_context_ptr_from_id(uint32_t id);

/**
 * Find the CUPTI context ID corresponding to the given CUDA context pointer.
 *
 * @param ptr    CUDA context pointer.
 * @return       Corresponding CUPTI context ID.
 */
extern uint32_t CUPTI_context_id_from_ptr(CUcontext ptr);
