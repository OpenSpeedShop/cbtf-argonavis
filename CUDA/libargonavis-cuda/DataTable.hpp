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

/** @file Declaration of the DataTable class. */

#pragma once

#include <boost/shared_ptr.hpp>
#include <string>
#include <vector>

#include <ArgoNavis/Base/TimeInterval.hpp>

#include <ArgoNavis/CUDA/Device.hpp>
#include <ArgoNavis/CUDA/StackTrace.hpp>

namespace ArgoNavis { namespace CUDA { namespace Impl {

    /**
     * Table containing the performance data for one or more threads. This class
     * provides the underlying implementation for the PerformanceData class.
     */
    class DataTable
    {

    public:

        /** Type of handle (smart pointer) to a data table. */
        typedef boost::shared_ptr<DataTable> Handle;

        /** Construct an empty data table. */
        DataTable();

        /** Names of all sampled hardware performance counters. */
        const std::vector<std::string>& counters() const;

        /** Information about all known CUDA devices. */
        const std::vector<Device>& devices() const;

        /** Smallest time interval containing this performance data. */
        const Base::TimeInterval& interval() const;

        /** Call sites of all known CUDA requests. */
        const std::vector<StackTrace>& sites() const;

    private:

        /** Names of all sampled hardware performance counters. */
        std::vector<std::string> dm_counters;

        /** Information about all known CUDA devices. */
        std::vector<Device> dm_devices;

        /** Smallest time interval containing this performance data. */
        Base::TimeInterval dm_interval;
        
        /** Call sites of all known CUDA requests. */
        std::vector<StackTrace> dm_sites;

    }; // class DataTable

} } } // namespace ArgoNavis::CUDA::Impl
