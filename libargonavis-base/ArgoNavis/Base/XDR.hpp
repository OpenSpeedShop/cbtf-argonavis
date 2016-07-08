////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Argo Navis Technologies. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////

/** @file Declaration and definition of XDR helper functions. */

#pragma once

#include <algorithm>
#include <cstdlib>
#include <rpc/types.h>

namespace ArgoNavis { namespace Base {

    /**
     * Function making it more convenient to allocate XDR variable-length
     * ("counted") arrays. XDR gets cranky (i.e. it crashes) when you try
     * to pass a null data pointer for a zero-length array. This function
     * insures that at least one element is always allocated. Also insures
     * that malloc() is used instead of new[] because XDR calls free() to
     * release the memory within xdr_free().
     *
     * @tparam T     Type of elements in the array.
     *
     * @param len    Length of the array to be allocated.
     * @return       Pointer to the allocated array.
     */
    template <typename T>
    T* allocateXDRCountedArray(u_int len)
    {
        return reinterpret_cast<T*>(malloc(std::max(1U, len) * sizeof(T)));
    }

} } // namespace ArgoNavis::Base
