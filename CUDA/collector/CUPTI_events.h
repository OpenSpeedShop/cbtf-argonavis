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

/** @file Declaration of CUPTI events functions. */

#pragma once

#include <cupti.h>

#include "TLS.h"

/*
 * Start CUPTI events data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be started.
 */
void CUPTI_events_start(CUcontext context);

/*
 * Sample the CUPTI events for all active CUDA contexts.
 *
 * @param tls       Thread-local storage of the current thread.
 * @param sample    Periodic sample to hold the CUPTI event counts.
 */
void CUPTI_events_sample(TLS* tls, PeriodicSample* sample);

/*
 * Stop CUPTI events data collection for the specified CUDA context.
 *
 * @param context    CUDA context for which data collection is to be stopped.
 */
void CUPTI_events_stop(CUcontext context);
