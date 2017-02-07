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

/** @file Declaration and definition of the PTHREAD_CHECK macro. */

#pragma once

#include <monitor.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * Checks that the given pthread function call returns the value "0". If the
 * call was unsuccessful, the returned error is reported on the standard error
 * stream and the application is aborted.
 *
 * @param x    Pthread function call to be checked.
 */
#define PTHREAD_CHECK(x)                                    \
    do {                                                    \
        int RETVAL = x;                                     \
        if (RETVAL != 0)                                    \
        {                                                   \
            fprintf(stderr, "[CUDA %d:%d] %s(): %s = %d\n", \
                    getpid(), monitor_get_thread_num(),     \
                    __func__, #x, RETVAL);                  \
            fflush(stderr);                                 \
            abort();                                        \
        }                                                   \
    } while (0)
