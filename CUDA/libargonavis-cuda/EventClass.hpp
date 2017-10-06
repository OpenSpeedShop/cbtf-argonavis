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

#include <ArgoNavis/CUDA/DataTransfer.hpp>
#include <ArgoNavis/CUDA/KernelExecution.hpp>

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
            return (lhs.device == rhs.device) &&
                   (lhs.call_site == rhs.call_site) &&
                   (lhs.context == rhs.context) &&
                   (lhs.stream == rhs.stream) &&
                   (lhs.size == rhs.size) &&
                   (lhs.kind == rhs.kind) &&
                   (lhs.source_kind == rhs.source_kind) &&
                   (lhs.destination_kind == rhs.destination_kind) &&
                   (lhs.asynchronous == rhs.asynchronous);
        }
    };

    /** Specialization of EventClass for KernelExecution. */
    template <>
    struct EventClass<KernelExecution>
    {
        bool operator()(const KernelExecution& lhs,
                        const KernelExecution& rhs) const
        {
            return (lhs.device == rhs.device) &&
                   (lhs.call_site == rhs.call_site) &&
                   (lhs.context == rhs.context) &&
                   (lhs.stream == rhs.stream) &&
                   (lhs.function == rhs.function) &&
                   (lhs.grid.get<0>() == rhs.grid.get<0>()) &&
                   (lhs.grid.get<1>() == rhs.grid.get<1>()) &&
                   (lhs.grid.get<2>() == rhs.grid.get<2>()) &&
                   (lhs.block.get<0>() == rhs.block.get<0>()) &&
                   (lhs.block.get<1>() == rhs.block.get<1>()) &&
                   (lhs.block.get<2>() == rhs.block.get<2>()) &&
                   (lhs.cache_preference == rhs.cache_preference) &&
                   (lhs.registers_per_thread == rhs.registers_per_thread) &&
                   (lhs.static_shared_memory == rhs.static_shared_memory) &&
                   (lhs.dynamic_shared_memory == rhs.dynamic_shared_memory) &&
                   (lhs.local_memory == rhs.local_memory);
        }
    };

} } } // namespace ArgoNavis::CUDA::Impl
