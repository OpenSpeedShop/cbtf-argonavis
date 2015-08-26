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

/** @file Definition of CUPTI time support functions and globals. */

#include <KrellInstitute/Services/Time.h>

#include "CUPTI_check.h"
#include "CUPTI_time.h"



/**
 * The offset that must be added to all CUPTI-provided time values in order to
 * translate them to the same time "origin" provided by CBTF_GetTime(). Computed
 * by the process-wide initialization in cbtf_collector_start().
 */
int64_t TimeOffset = 0;



/**
 * Estimate the offset that must be added to all CUPTI-provided time values in
 * order to translate them to the same time "origin" provided by CBTF_GetTime().
 */
void CUPTI_time_synchronize()
{
    uint64_t cbtf_now = CBTF_GetTime();
    
    uint64_t cupti_now = 0;
    CUPTI_CHECK(cuptiGetTimestamp(&cupti_now));
    
    if (cbtf_now > cupti_now)
    {
        TimeOffset = cbtf_now - cupti_now;
    }
    else if (cbtf_now < cupti_now)
    {
        TimeOffset = -(cupti_now - cbtf_now);
    }
    else
    {
        TimeOffset = 0;
    }
}
