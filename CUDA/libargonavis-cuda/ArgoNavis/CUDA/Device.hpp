////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014,2015 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration of the Device structure. */

#pragma once

#include <boost/cstdint.hpp>
#include <string>

#include <ArgoNavis/CUDA/Vector.hpp>

namespace ArgoNavis { namespace CUDA {

    /** Information about a CUDA device. */
    struct Device
    {
        /** Name of this device. */
        std::string name;
        
        /** Compute capability (major/minor) of this device. */
        Vector2u compute_capability;
        
        /** Maximum allowed dimensions of grids with this device. */
        Vector3u max_grid;
        
        /** Maximum allowed dimensions of blocks with this device. */
        Vector3u max_block;
        
        /** Global memory bandwidth of this device (in KBytes/sec). */
        boost::uint64_t global_memory_bandwidth;
        
        /** Global memory size of this device (in bytes). */
        boost::uint64_t global_memory_size;
        
        /** Constant memory size of this device (in bytes). */
        boost::uint32_t constant_memory_size;
        
        /** L2 cache size of this device (in bytes). */
        boost::uint32_t l2_cache_size;
        
        /** Number of threads per warp for this device. */
        boost::uint32_t threads_per_warp;
        
        /** Core clock rate of this device (in KHz). */
        boost::uint32_t core_clock_rate;
        
        /** Number of memory copy engines on this device. */
        boost::uint32_t memcpy_engines;
        
        /** Number of multiprocessors on this device. */
        boost::uint32_t multiprocessors;
        
        /** 
         * Maximum instructions/cycle possible on this device's multiprocessors.
         */
        boost::uint32_t max_ipc;
        
        /** Maximum warps/multiprocessor for this device. */
        boost::uint32_t max_warps_per_multiprocessor;
        
        /** Maximum blocks/multiprocessor for this device. */
        boost::uint32_t max_blocks_per_multiprocessor;
        
        /** Maximum registers/block for this device. */
        boost::uint32_t max_registers_per_block;
        
        /** Maximium shared memory / block for this device. */
        boost::uint32_t max_shared_memory_per_block;
        
        /** Maximum threads/block for this device. */
        boost::uint32_t max_threads_per_block;
    };
    
} } // namespace ArgoNavis::CUDA
