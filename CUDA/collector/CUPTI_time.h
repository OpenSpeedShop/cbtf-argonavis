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

/** @file Declaration of CUPTI time support functions and globals. */

#pragma once

#include <inttypes.h>

/*
 * The offset that must be added to all CUPTI-provided time values in order to
 * translate them to the same time "origin" provided by CBTF_GetTime().
 */
extern int64_t TimeOffset;

/* Estimate the offset from CUPTI-provided times to CBTF_GetTime(). */
void CUPTI_time_synchronize();
