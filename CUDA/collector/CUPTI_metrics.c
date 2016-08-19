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

/** @file Definition of CUPTI metrics functions. */

#include <cuda.h>
#include <pthread.h>
#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

#include <KrellInstitute/Services/Assert.h>

#include "collector.h"
#include "CUDA_check.h"
#include "CUPTI_check.h"
#include "CUPTI_context.h"
#include "CUPTI_metrics.h"
#include "Pthread_check.h"



/**
 * Start metrics data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be started.
 */
void CUPTI_metrics_start(CUcontext context)
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] CUPTI_metrics_start(%p)\n", context);
    }
#endif

    // ...
}



/**
 * Sample the CUPTI metrics for all active CUDA contexts.
 *
 * @param tls       Thread-local storage of the current thread.
 * @param sample    Periodic sample to hold the CUPTI metric counts.
 */
void CUPTI_metrics_sample(TLS* tls, PeriodicSample* sample)
{
    // ...
}



/**
 * Stop CUPTI metrics data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be stopped.
 */
void CUPTI_metrics_stop(CUcontext context)
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CBTF/CUDA] CUPTI_metrics_stop(%p)\n", context);
    }
#endif

    // ...
}
