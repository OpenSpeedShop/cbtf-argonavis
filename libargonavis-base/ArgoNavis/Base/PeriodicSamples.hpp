////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016,2017 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration of the PeriodicSamples class. */

#pragma once

#include <boost/cstdint.hpp>
#include <map>
#include <string>

#include <ArgoNavis/Base/PeriodicSampleVisitor.hpp>
#include <ArgoNavis/Base/Time.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

namespace ArgoNavis { namespace Base {

    /**
     * Zero or more samples taken at specific (usually periodic) points in time.
     */
    class PeriodicSamples
    {
        
    public:

        /** Enumeration of the supported kinds of sampled values. */
        enum Kind
        {
            Count,      /**< Each sampled value is an event count. */
            Percentage, /**< Each sampled value is a percentage. */
            Rate        /**< Each sampled value is a rate. */
        };
        
        /**
         * Construct an empty container.
         *
         * @param name    Name of these samples.
         * @param kind    Kind of sampled values.
         */
        PeriodicSamples(const std::string& name, Kind kind = Count);

        /** Get the name of these samples. */
        const std::string& name() const
        {
            return dm_name;
        }

        /** Get the kind of sampled values. */
        Kind kind() const
        {
            return dm_kind;
        }
        
        /** Smallest time interval containing all of these samples. */
        TimeInterval interval() const;

        /** Add a new sample. */
        void add(const Time& time, boost::uint64_t value);

        /**
         * Visit those samples within the specified time interval.
         *
         * @note    The visitation is terminated immediately if "false"
         *          is returned by the visitor.
         *
         * @param interval    Time interval for the visitation.
         * @param visitor     Visitor invoked for each sample.
         */
        void visit(const TimeInterval& interval,
                   const PeriodicSampleVisitor& visitor) const;
        
    private:

        /** Name of these samples. */
        std::string dm_name;

        /** Kind of sampled values. */
        Kind dm_kind;

        /** Individual samples indexed by time. */
        std::map<Time, boost::uint64_t> dm_samples;
        
    }; // class PeriodicSamples

} } // namespace ArgoNavis::Base
