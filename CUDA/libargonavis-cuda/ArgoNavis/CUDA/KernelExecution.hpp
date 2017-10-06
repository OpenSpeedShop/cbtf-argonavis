////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2017 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration of the KernelExecution structure. */

#pragma once

#include <boost/cstdint.hpp>
#include <stddef.h>
#include <string>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/Time.hpp>

#include <ArgoNavis/CUDA/CachePreference.hpp>
#include <ArgoNavis/CUDA/Vector.hpp>

namespace ArgoNavis { namespace CUDA {
    
    /** Information about a CUDA kernel execution. */
    struct KernelExecution
    {
        /** Unique ID of the kernel execution class. */
        boost::uint32_t clas;
    
        /** Index of the device on which the kernel execution occurred. */
        size_t device;

        /** Index of the kernel execution's call site. */
        size_t call_site;

        /** Correlation ID of the kernel execution. */
        boost::uint32_t id;

        /** CUDA context for which the kernel was executed. */
        Base::Address context;

        /** CUDA stream for which the kernel was executed. */
        Base::Address stream;

        /** Time at which the kernel execution was requested. */
        Base::Time time;
        
        /** Time at which the kernel execution began. */
        Base::Time time_begin;
        
        /** Time at which the kernel execution ended. */
        Base::Time time_end;
        
        /** Name of the kernel function being executed. */
        std::string function;
        
        /** Dimensions of the grid. */
        Vector3u grid;
        
        /** Dimensions of each block. */
        Vector3u block;
        
        /** Cache preference used. */
        CachePreference cache_preference;
        
        /** Registers required for each thread. */
        boost::uint32_t registers_per_thread;
        
        /** Total amount (in bytes) of static shared memory reserved. */
        boost::uint32_t static_shared_memory;
        
        /** Total amount (in bytes) of dynamic shared memory reserved. */
        boost::uint32_t dynamic_shared_memory;
        
        /** Total amount (in bytes) of local memory reserved. */
        boost::uint32_t local_memory;
    };
    
} } // namespace ArgoNavis::CUDA
