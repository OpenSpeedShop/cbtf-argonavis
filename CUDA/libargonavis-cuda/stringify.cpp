////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Argo Navis Technologies. All Rights Reserved.
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

/** @file Table used by the stringify() function. */

#include <boost/assign/list_of.hpp>

#include <ArgoNavis/CUDA/stringify.hpp>



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const std::map<std::string, std::string> ArgoNavis::CUDA::Impl::kCounterNames =
    boost::assign::map_list_of

    // CUPTI Metric Names
    ("inst_executed", "GPU All")
    ("inst_control", "GPU Branches")
    ("inst_integer", "GPU Integer")
    ("flop_count_sp", "GPU Float (Single)")
    ("flop_count_dp", "GPU Float (Double)")
    ("ldst_executed", "GPU Load/Store")

    // PAPI Preset Event Names
    ("PAPI_TOT_INS", "CPU All")
    ("PAPI_BR_INS", "CPU Branches")
    ("PAPI_INT_INS", "CPU Integer")
    ("PAPI_SP_OPS", "CPU Float (Single)")
    ("PAPI_DP_OPS", "CPU Float (Double)")
    ("PAPI_LST_INS", "CPU Load/Store")

    ;
