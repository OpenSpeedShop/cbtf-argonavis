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

/** @file Declaration of CUPTI activity functions. */

#pragma once

#include <cupti.h>

#include "TLS.h"

/* Start CUPTI activity data collection for this process. */
void CUPTI_activities_start();

/*
 * Add the CUPTI activity data for the specified CUDA context/stream to the
 * performance data blob contained within the given thread-local storage.
 *
 * @param tls        Thread-local storage to which activities are to be added.
 * @param context    CUDA context for the activities to be added.
 * @param stream     CUDA stream for the activities to be added.
 */
void CUPTI_activities_add(TLS* tls, CUcontext context, CUstream stream);

/* Ensure all CUPTI activity data for this process has been flushed. */
void CUPTI_activities_flush();

/* Stop CUPTI activity data collection for this process. */
void CUPTI_activities_stop();
