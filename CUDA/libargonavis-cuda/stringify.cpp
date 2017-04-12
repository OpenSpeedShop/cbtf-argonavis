////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016,2017 Argo Navis Technologies. All Rights Reserved.
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
const std::map<
    std::string, std::string
    > ArgoNavis::CUDA::Impl::kLongCounterNames = boost::assign::map_list_of

    // CUPTI Metric Names
    ("inst_executed", "GPU All Instructions")
    ("inst_control", "GPU Branch Instructions")
    ("inst_integer", "GPU Integer Instructions")
    ("flop_count_sp", "GPU (32-Bit) Float Instructions")
    ("flop_count_dp", "GPU (64-Bit) Float Instructions")
    ("ldst_executed", "GPU Load/Store Instructions")
    
    // PAPI Preset Event Names
    ("PAPI_TOT_INS", "CPU All Instructions")
    ("PAPI_BR_INS", "CPU Branches Instructions")
    ("PAPI_INT_INS", "CPU Integer Instructions")
    ("PAPI_SP_OPS", "CPU (32-Bit) Float Instructions")
    ("PAPI_DP_OPS", "CPU (64-Bit) Float Instructions")
    ("PAPI_LST_INS", "CPU Load/Store Instructions")

    // Additional CUPTI Metric Names

    ("achieved_occupancy", "GPU Achieved Occupancy (%)")
    ("branch_efficiency", "GPU Branch Efficiency (%)")

    ("stall_constant_memory_dependency",
     "GPU Stalls (%) on Constant Cache Miss)")
    ("stall_data_request", "GPU Stalls (%) on Memory Busy")
    ("stall_exec_dependency", " GPU Stalls (%) on Instruction Dependency")
    ("stall_inst_fetch", "GPU Stalls (%) on Instruction Fetch")
    ("stall_memory_dependency", "GPU Stalls (%) on Memory Busy")
    ("stall_memory_throttle", "GPU Stalls (%) on Memory Throttle")
    ("stall_not_selected", "GPU Stalls (%) on Warp Not Selected")
    ("stall_other", "GPU Stalls (%) on Other")
    ("stall_pipe_busy", "GPU Stalls (%) on Pipeline Busy")
    ("stall_sync", "GPU Stalls (%) on Warp Blocked on Sync")
    ("stall_texture", "GPU Stalls (%) on Texture Units Busy")

    ;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const std::map<
    std::string, std::string
    > ArgoNavis::CUDA::Impl::kShortCounterNames = boost::assign::map_list_of

    // CUPTI Metric Names
    ("inst_executed", "GPU All")
    ("inst_control", "GPU Branches")
    ("inst_integer", "GPU Integer")
    ("flop_count_sp", "GPU 32-Bit Float")
    ("flop_count_dp", "GPU 64-Bit Float")
    ("ldst_executed", "GPU Load/Store")
    
    // PAPI Preset Event Names
    ("PAPI_TOT_INS", "CPU All")
    ("PAPI_BR_INS", "CPU Branches")
    ("PAPI_INT_INS", "CPU Integer")
    ("PAPI_SP_OPS", "CPU 32-Bit Float")
    ("PAPI_DP_OPS", "CPU 64-Bit Float")
    ("PAPI_LST_INS", "CPU Load/Store")

    // Additional CUPTI Metric Names

    ("achieved_occupancy", "GPU Achieved Occupancy")
    ("branch_efficiency", "GPU Branch Efficiency")

    ("stall_constant_memory_dependency", "GPU Stalls (Const Miss)")
    ("stall_data_request", "GPU Stalls (Mem Busy)")
    ("stall_exec_dependency", " GPU Stalls (Inst Dep)")
    ("stall_inst_fetch", "GPU Stalls (Inst Fetch)")
    ("stall_memory_dependency", "GPU Stalls (Mem Busy)")
    ("stall_memory_throttle", "GPU Stalls (Mem Throt)")
    ("stall_not_selected", "GPU Stalls (Warp Inactive)")
    ("stall_other", "GPU Stalls (Other)")
    ("stall_pipe_busy", "GPU Stalls (Pipe Busy)")
    ("stall_sync", "GPU Stalls (Warp Sync)")
    ("stall_texture", "GPU Stalls (Tex Busy)")

    ;
