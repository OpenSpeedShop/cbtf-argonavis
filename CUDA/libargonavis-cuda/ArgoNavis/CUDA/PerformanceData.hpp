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

/** @file Declaration of the PerformanceData class. */

#pragma once

#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <stddef.h>
#include <string>
#include <vector>

#include <KrellInstitute/Messages/CUDA_data.h>

#include <ArgoNavis/Base/AddressVisitor.hpp>
#include <ArgoNavis/Base/BlobVisitor.hpp>
#include <ArgoNavis/Base/PeriodicSamples.hpp>
#include <ArgoNavis/Base/PeriodicSampleVisitor.hpp>
#include <ArgoNavis/Base/StackTrace.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>
#include <ArgoNavis/Base/ThreadVisitor.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

#include <ArgoNavis/CUDA/CounterDescription.hpp>
#include <ArgoNavis/CUDA/DataTransferVisitor.hpp>
#include <ArgoNavis/CUDA/Device.hpp>
#include <ArgoNavis/CUDA/KernelExecutionVisitor.hpp>

namespace ArgoNavis { namespace CUDA {

    namespace Impl {
        class DataTable;
    }

    /**
     * CUDA performance data for one or more threads.
     */
    class PerformanceData
    {

    public:

        /**
         * Visit the PC (program counter) addresses within the given message.
         *
         * @param message    Message containing the performance data.
         * @param visitor    Visitor invoked for each PC address.
         */
        static void visitPCs(const CBTF_cuda_data& message,
                             const Base::AddressVisitor& visitor);

        /** Construct empty performance data. */
        PerformanceData();

        /**
         * Apply (add) the performance data contained within the given message.
         *
         * @param thread     Name of the thread containing this data.
         * @param message    Message containing the performance data.
         */
        void apply(const Base::ThreadName& thread,
                   const CBTF_cuda_data& message);
        
        /** Name and kind of all sampled hardware performance counters. */
        const std::vector<CounterDescription>& counters() const;
        
        /**
         * Counts for all sampled hardware performance counters for the given
         * thread between the specified time interval.
         *
         * @note    The name of the counter corresponding to any particular
         *          count in the returned vector can be found by using that
         *          count's index within the vector returned by counters().
         *
         * @note    When the specified time interval does not lie exactly on
         *          the hardware performance counter sample times, the counts
         *          provided are estimates based on the nearest samples.
         *
         * @param thread      Name of thread for which to get counts.
         * @param interval    Time interval over which to get counts.
         * @return            Counts over the specified time interval.
         */
        std::vector<boost::uint64_t> counts(
            const Base::ThreadName& thread,
            const Base::TimeInterval& interval
            ) const;
        
        /**
         * Index, within devices(), of the device for which the given thread is
         * a GPU hardware performance counter sampling thread. Returns "none" if
         * the thread isn't a GPU hardware performance counter sampling thread.
         *
         * @param thread    Name of thread for which to get the device index.
         * @return          Index of the device within devices() or "none".
         */
        boost::optional<std::size_t> device(
            const Base::ThreadName& thread
            ) const;
        
        /** Information about all known CUDA devices. */
        const std::vector<Device>& devices() const;

        /** Smallest time interval containing this performance data. */
        const Base::TimeInterval& interval() const;

        /**
         * Periodic hardware performance counter samples within the given
         * thread whose sample time is within the specified time interval.
         *
         * @param thread      Name of the thread for which to get samples.
         * @param interval    Time interval over which to get samples.
         * @param counter     Index, within counters(), of the counter
         *                    for which to get samples.
         * @return            Samples over the specified time interval.
         *
         * @throw std::invalid_argument    The given counter index is not valid.
         */
        Base::PeriodicSamples periodic(
            const Base::ThreadName& thread,
            const Base::TimeInterval& interval,
            std::size_t counter
            ) const;
        
        /** Call sites of all known CUDA requests. */
        const std::vector<Base::StackTrace>& sites() const;

        /**
         * Visit the (raw) performance data blobs for the given thread.
         *
         * @param thread      Name of thread for the visitation.
         * @param visitor     Visitor invoked for each performance data blob.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        void visitBlobs(const Base::ThreadName& thread,
                        const Base::BlobVisitor& visitor) const;

        /**
         * Visit those data transfers within the given thread whose request-
         * to-completion time interval intersects the specified time interval.
         *
         * @param thread      Name of thread for the visitation.
         * @param interval    Time interval for the visitation.
         * @param visitor     Visitor invoked for each data transfer.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        void visitDataTransfers(const Base::ThreadName& thread,
                                const Base::TimeInterval& interval,
                                const DataTransferVisitor& visitor) const;

        /**
         * Visit those kernel executions within the given thread whose request-
         * to-completion time interval intersects the specified time interval.
         *
         * @param thread      Name of thread for the visitation.
         * @param interval    Time interval for the visitation.
         * @param visitor     Visitor invoked for each kernel execution.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        void visitKernelExecutions(const Base::ThreadName& thread,
                                   const Base::TimeInterval& interval,
                                   const KernelExecutionVisitor& visitor) const;

        /**
         * Visit those hardware performance counter periodic samples within the
         * given thread whose sample time is within the specified time interval.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         *
         * @note    The name of the counter corresponding to any particular
         *          count in the vector passed into the visitor can be found
         *          by using that count's index within the vector returned by
         *          counters().
         *
         * @param thread      Name of thread for the visitation.
         * @param interval    Time interval for the visitation.
         * @param visitor     Visitor invoked for each periodic sample.
         */
        void visitPeriodicSamples(
            const Base::ThreadName& thread,
            const Base::TimeInterval& interval,
            const Base::PeriodicSampleVisitor& visitor
            ) const;

        /**
         * Visit the threads containing performance data.
         *
         * @param visitor    Visitor invoked for each thread containing
         *                   performance data.
         *
         * @note    The visitation is terminated immediately if "false" is
         *          returned by the visitor.
         */
        void visitThreads(const Base::ThreadVisitor& visitor) const;

    private:

        /** Data table containing this performance data. */
        boost::shared_ptr<Impl::DataTable> dm_data_table;

    }; // class PerformanceData

} } // namespace ArgoNavis::CUDA
