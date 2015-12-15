////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2015 Argo Navis Technologies. All Rights Reserved.
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
#include <ArgoNavis/Base/Time.hpp>

namespace ArgoNavis { namespace CUDA { namespace Impl {

    /**
     * CUDA performance data blob generator. Encapsulates the state required to
     * aggregate individual messages, call sites, etc. into blobs.
     */
    class BlobGenerator :
        private boost::noncopyable
    {

    public:

        /** Construct an empty blob generator with the given blob visitor. */
        BlobGenerator(Base::BlobVisitor& visitor);

        /** Destroy this blob generator. */
        ~BlobGenerator();

        /** Flag indicating whether blob generation should be terminated. */
        bool terminate() const
        {
            return dm_terminate;
        }

        /** Add the specified call site to the current blob. */
        boost::uint32_t add_site(const Base::StackTrace& site);

        /** Add the specified message to the current blob. */
        CBTF_cuda_message* add_message();

        /** Update the current blob's header with the specified address. */
        void update_header(const Base::Address& address);

        /** Update the current blob's header with the specified time. */
        void update_header(const Base::Time& time);
        
    private:
        
        /** Initialize the current header and data. */
        void initialize();

        /** Generate a blob for the current header and data. */
        void generate();
        
        /** Visitor invoked for each generated blob. */
        Base::BlobVisitor& dm_visitor;
        
        /** Flag indicating whether blob generation should be terminated. */
        bool dm_terminate;

        /** Header for the current blob. */
        boost::shared_ptr<CBTF_DataHeader> dm_header;

        /** Data for the current blob. */
        boost::shared_ptr<CBTF_cuda_data> dm_data;

        /** Individual messages pointed to by the data above. */
        std::vector<CBTF_cuda_message> dm_messages;

        /** Unique, null-terminated, stack traces referenced by the messages. */
        std::vector<CBTF_Protocol_Address> dm_stack_traces;

    }; // class BlobGenerator

} } } // namespace ArgoNavis::CUDA::Impl
