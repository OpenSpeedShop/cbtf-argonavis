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

/** @file Declaration of the CounterDescription structure. */

#pragma once

#include <string>

#include <ArgoNavis/CUDA/CounterKind.hpp>

namespace ArgoNavis { namespace CUDA {

    /** Description of a hardware performance counter. */
    struct CounterDescription
    {
        /**
         * Name of the counter. This is PAPI's or CUPTI's ASCII name for
         * the event or metric.
         *
         * @sa http://icl.cs.utk.edu/papi/
         * @sa http://docs.nvidia.com/cuda/cupti/r_main.html#r_metric_api
         */
        std::string name;

        /** Kind of the counter. */
        CounterKind kind;
        
        /**
         * Threshold for the counter. This is the number of counts between
         * consecutive overflow samples. Zero when only periodic (in time)
         * sampling was used.
         */
        int threshold;
    };
    
} } // namespace ArgoNavis::CUDA
