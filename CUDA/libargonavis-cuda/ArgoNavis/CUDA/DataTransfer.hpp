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

/** @file Declaration of the DataTransfer structure. */

#pragma once

#include <boost/cstdint.hpp>
#include <stddef.h>

#include <ArgoNavis/Base/Time.hpp>

#include <ArgoNavis/CUDA/CopyKind.hpp>
#include <ArgoNavis/CUDA/MemoryKind.hpp>

namespace ArgoNavis { namespace CUDA {
    
    /** Information about a CUDA data transfer. */
    struct DataTransfer
    {
        /** Index of the device on which the data transfer occurred. */
        size_t device;
        
        /** Index of the data transfer's call site. */
        size_t call_site;
        
        /** Time at which the data transfer was requested. */
        Base::Time time;
        
        /** Time at which the data transfer began. */
        Base::Time time_begin;
        
        /** Time at which the data transfer ended. */
        Base::Time time_end;
        
        /** Number of bytes being transferred. */
        uint64_t size;
        
        /** Kind of data transfer performed. */
        CopyKind kind;
        
        /** Kind of memory from which the data transfer was performed. */
        MemoryKind source_kind;
        
        /** Kind of memory to which the data transfer was performed. */
        MemoryKind destination_kind;
        
        /** Was the data transfer asynchronous? */
        bool asynchronous;  
    };
    
} } // namespace ArgoNavis::CUDA
