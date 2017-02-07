/*******************************************************************************
** Copyright (c) 2016-2017 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration of CUPTI metrics functions. */

#pragma once

#include <cuda.h>

#include "TLS.h"

/* Flag indicating if CUDA kernel execution is to be serialized. */
extern bool CUPTI_metrics_do_kernel_serialization;

/* Initialize CUPTI metrics data collection for this process. */
void CUPTI_metrics_initialize();

/*
 * Start CUPTI metrics data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be started.
 */
void CUPTI_metrics_start(CUcontext context);

/*
 * Sample the CUPTI metrics for the specified CUDA context.
 *
 * @param context    CUDA context for which a sample is to be taken.
 */
void CUPTI_metrics_sample(CUcontext context);

/*
 * Thread function implementing the sampling of CUPTI metrics data collection.
 *
 * @param arg    Unused.
 * @return       Always returns NULL.
 *
 * @note    The only reason this thread function isn't completely hidden
 *          inside the source file is that cbtf_collector_[start&stop]()
 *          needs the thread's address to suppress PAPI event collection
 *          for this thread.
 */
void* CUPTI_metrics_sampling_thread(void* arg);

/*
 * Stop CUPTI metrics data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be stopped.
 */
void CUPTI_metrics_stop(CUcontext context);

/* Finalize CUPTI metrics data collection for this process. */
void CUPTI_metrics_finalize();
