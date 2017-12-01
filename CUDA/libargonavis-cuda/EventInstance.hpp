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

/** @file Declaration of the EventInstance structure. */

#pragma once

#include <boost/cstdint.hpp>

#include <ArgoNavis/Base/Time.hpp>

namespace ArgoNavis { namespace CUDA { namespace Impl {
  
    /** Information about an event instance. */
    struct EventInstance
    {
        /** Unique ID of the event class being instanced. */
        boost::uint32_t clas;
    
        /** Correlation ID of the event. */
        boost::uint32_t id;
        
        /** Time at which the event was requested. */
        Base::Time time;
        
        /** Time at which the event began. */
        Base::Time time_begin;
        
        /** Time at which the event ended. */
        Base::Time time_end;
    };

} } } // namespace ArgoNavis::CUDA::Impl
