////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration of the PeriodicSamplesGroup type and functions. */

#pragma once

#include <boost/cstdint.hpp>
#include <boost/optional.hpp>
#include <vector>

#include <ArgoNavis/Base/PeriodicSamples.hpp>
#include <ArgoNavis/Base/Time.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

namespace ArgoNavis { namespace Base {

    /** Type of container used to store multiple PeriodicSamples. */
    typedef std::vector<PeriodicSamples> PeriodicSamplesGroup;

    /** Get the total number of samples in a group. */
    boost::uint64_t getTotalSampleCount(const PeriodicSamplesGroup& group);
    
    /** Get the smallest time interval containing all samples in a group. */
    TimeInterval getSmallestTimeInterval(const PeriodicSamplesGroup& group);
    
    /** Get the average sampling rate of all samples in a group. */
    Time getAverageSamplingRate(const PeriodicSamplesGroup& group);

    /**
     * Resample a group at a fixed sampling rate. If no sampling rate is
     * provided, the average sampling rate of the group (to the nearest
     * mS) is used.
     */
    PeriodicSamplesGroup getResampled(
        const PeriodicSamplesGroup& group,
        const boost::optional<Time>& rate = boost::none
        );
    
} } // namespace ArgoNavis::Base
