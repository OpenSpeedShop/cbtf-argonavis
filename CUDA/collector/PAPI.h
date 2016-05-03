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

/** @file Declaration of PAPI functions. */

#pragma once

#include "TLS.h"

/* Initialize PAPI for this process. */
void PAPI_initialize();

/* Start PAPI data collection for the current thread. */
void PAPI_start_data_collection();

/*
 * Sample the PAPI counters for the current thread.
 *
 * @param sample    Sample into which to place the PAPI counts.
 */
void PAPI_sample(PeriodicSample* sample);

/* Stop PAPI data collection for the current thread. */
void PAPI_stop_data_collection();

/* Finalize PAPI for this process. */
void PAPI_finalize();
