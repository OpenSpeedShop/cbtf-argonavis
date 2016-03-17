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

/** @file Declaration of PAPI functions. */

#pragma once

/* Called by the CUPTI callback to notify when a CUDA context is created. */
void PAPI_notify_cuda_context_created();

/* Called by the CUPTI callback to notify when a CUDA context is destroyed. */
void PAPI_notify_cuda_context_destroyed();

/* Start PAPI data collection for the current thread. */
void PAPI_start_data_collection();

/* Stop PAPI data collection for the current thread. */
void PAPI_stop_data_collection();
