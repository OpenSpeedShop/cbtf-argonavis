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

/** @file Definition of Mutex functions. */

#include "Mutex.h"



/**
 * Acquire the specified mutex, busy waiting until it is available.
 *
 * @param mutex    Mutex to be acquired.
 */
void Mutex_acquire(Mutex* mutex)
{
    Mutex expected = 0, desired = 1;

    while (!__atomic_compare_exchange(
               mutex, &expected, &desired,
               false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
}



/**
 * Release the specified mutex, busy waiting until successful.
 *
 * @param mutex    Mutex to be released.
 */
void Mutex_release(Mutex* mutex)
{
    Mutex expected = 1, desired = 0;

    while (!__atomic_compare_exchange(
               mutex, &expected, &desired,
               false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST));
}



/**
 * Try to acquire the specifeid mutex without waiting for it.
 *
 * @param mutex    Mutex to be acquired.
 * @return         Boolean "true" if the mutex was
 *                 acquired, or "false" otherwise.
 */
bool Mutex_try(Mutex* mutex)
{
    Mutex expected = 0, desired = 1;

    return __atomic_compare_exchange(
        mutex, &expected, &desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
        );
}
