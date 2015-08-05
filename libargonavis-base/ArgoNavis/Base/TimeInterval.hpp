////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013 Krell Institute. All Rights Reserved.
// Copyright (c) 2015 Argo Navis Technologies. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation; either version 2 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
// details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////

/** @file Declaration and definition of the TimeInterval type. */

#pragma once

#include <KrellInstitute/Messages/Time.h>

#include <ArgoNavis/Base/Interval.hpp>
#include <ArgoNavis/Base/Time.hpp>

namespace ArgoNavis { namespace Base {

    /**
     * A closed-ended time interval. Used to represent a contiguous period of
     * time.
     */
    typedef Interval<Time, CBTF_Protocol_TimeInterval> TimeInterval;

} } // namespace ArgoNavis::Base
