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

/** @file Declaration of CUDA collector globals. */

#pragma once

#include <stdbool.h>

#include <KrellInstitute/Messages/CUDA_data.h>

/* Flag indicating if debugging is enabled. */
extern bool IsDebugEnabled;

/* Event sampling configuration. */
extern CUDA_SamplingConfig TheSamplingConfig;

/* Number of events for which overflow sampling is enabled. */
extern int OverflowSamplingCount;