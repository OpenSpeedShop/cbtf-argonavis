////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Argo Navis Technologies. All Rights Reserved.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
// details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////

/** @file Declaration and definition of the EventClass functor. */

#pragma once

#include <boost/tuple/tuple_comparison.hpp>

#include <ArgoNavis/CUDA/DataTransfer.hpp>
#include <ArgoNavis/CUDA/KernelExecution.hpp>

/** Convenience macro for compactly generating EventClass tests. */
#define COMPARE_NEXT_FIELD(x)      \
    do                          \
    {                           \
        if (lhs.x < rhs.x)      \
        {                       \
            return true;        \
        }                       \
        else if (lhs.x > rhs.x) \
        {                       \
            return false;       \
        }                       \
    } while (false)

/** Convenience macro for compactly generating EventClass tests. */
#define COMPARE_LAST_FIELD(x) \
    return lhs.x < rhs.x

namespace ArgoNavis { namespace CUDA { namespace Impl {
    
    /**
     * Functor defining when events (kernel executions, data transfers, etc.)
     * are considered to be of the same class.
     *
     * @tparam T    Type of events for which event classes are being defined.
     */
    template <typename T>
    struct EventClass
    {
    };

    /** Specialization of EventClass for DataTransfer. */
    template <>
    struct EventClass<DataTransfer>
    {
        bool operator()(const DataTransfer& lhs,
                        const DataTransfer& rhs) const
        {
            COMPARE_NEXT_FIELD(device);
            COMPARE_NEXT_FIELD(call_site);
            COMPARE_NEXT_FIELD(context);
            COMPARE_NEXT_FIELD(stream);
            COMPARE_NEXT_FIELD(size);
            COMPARE_NEXT_FIELD(kind);
            COMPARE_NEXT_FIELD(source_kind);
            COMPARE_NEXT_FIELD(destination_kind);
            COMPARE_LAST_FIELD(asynchronous);
        }
    };

    /** Specialization of EventClass for KernelExecution. */
    template <>
    struct EventClass<KernelExecution>
    {
        bool operator()(const KernelExecution& lhs,
                        const KernelExecution& rhs) const
        {
            COMPARE_NEXT_FIELD(device);
            COMPARE_NEXT_FIELD(call_site);
            COMPARE_NEXT_FIELD(context);
            COMPARE_NEXT_FIELD(stream);
            COMPARE_NEXT_FIELD(function);
            COMPARE_NEXT_FIELD(grid);
            COMPARE_NEXT_FIELD(block);
            COMPARE_NEXT_FIELD(cache_preference);
            COMPARE_NEXT_FIELD(registers_per_thread);
            COMPARE_NEXT_FIELD(static_shared_memory);
            COMPARE_NEXT_FIELD(dynamic_shared_memory);
            COMPARE_LAST_FIELD(local_memory);
        }
    };

} } } // namespace ArgoNavis::CUDA::Impl

#undef COMPARE_NEXT_FIELD
#undef COMPARE_LAST_FIELD
