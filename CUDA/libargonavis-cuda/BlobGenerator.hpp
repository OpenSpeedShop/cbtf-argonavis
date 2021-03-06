////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015-2017 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration of the BlobGenerator class. */

#pragma once

#include <boost/cstdint.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <utility>
#include <vector>

#include <KrellInstitute/Messages/Address.h>
#include <KrellInstitute/Messages/CUDA_data.h>
#include <KrellInstitute/Messages/DataHeader.h>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/BlobVisitor.hpp>
#include <ArgoNavis/Base/StackTrace.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>
#include <ArgoNavis/Base/Time.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

namespace ArgoNavis { namespace CUDA { namespace Impl {

    /**
     * CUDA performance data blob generator. Encapsulates the state required to
     * aggregate individual messages, call sites, etc. into blobs.
     */
    class BlobGenerator :
        private boost::noncopyable
    {

    public:

        /** Construct an empty blob generator. */
        BlobGenerator(const Base::ThreadName& thread,
                      const Base::BlobVisitor& visitor,
                      const Base::TimeInterval& interval);

        /** Destroy this blob generator. */
        ~BlobGenerator();

        /** Flag indicating whether blob generation should be terminated. */
        bool terminate() const
        {
            return dm_terminate;
        }

        /**
         * Add the specified call site to the current blob.
         *
         * @note    Call sites are always referenced by a message. And since
         *          cross-blob references aren't supported, a crash is all but
         *          certain if the call site and its referencing message were
         *          to be split across two blobs. So this method also insures
         *          there is room in the current blob for at least one more
         *          message before adding the call site.
         */
        boost::uint32_t addSite(const Base::StackTrace& site);

        /** Add a new message to the current blob. */
        CBTF_cuda_message* addMessage();
        
        /** Add the specified periodic sample to the current blob. */
        void addPeriodicSample(boost::uint64_t time,
                               const std::vector<boost::uint64_t>& counts);
        
    private:
        
        /** Initialize the current header and data. */
        void initialize();

        /** Is the current blob already full? */
        bool full() const;
        
        /** Generate a blob for the current header and data. */
        void generate();

        /** Name of the thread for which blobs are being generated. */
        const Base::ThreadName& dm_thread;

        /** Visitor invoked for each generated blob. */
        const Base::BlobVisitor& dm_visitor;

        /** Time interval to be applied to each generated blob. */
        const Base::TimeInterval dm_interval;

        /** Flag indicating whether blob generation should be terminated. */
        bool dm_terminate;

        /** Header for the current blob. */
        boost::shared_ptr<CBTF_DataHeader> dm_header;

        /** Data for the current blob. */
        boost::shared_ptr<CBTF_cuda_data> dm_data;
        
        /** Message containing the periodic samples. */
        CUDA_PeriodicSamples dm_periodic_samples;
        
        /** Previously taken periodic event sample (including its time). */
        std::vector<boost::uint64_t> dm_periodic_samples_previous;

    }; // class BlobGenerator

} } } // namespace ArgoNavis::CUDA::Impl
