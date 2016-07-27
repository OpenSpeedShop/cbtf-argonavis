////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration of the Feature class. */

#pragma once

#include <boost/cstdint.hpp>
#include <cstring>
#include <string>

#include <ArgoNavis/Clustering/FeatureName.hpp>

namespace ArgoNavis { namespace Clustering {

    /**
     * Feature characterizing some aspect of a thread's performance as a single
     * 32-bit floating point value. Includes an optional name which, if present,
     * is used to ensure that only same-named features for different threads are
     * compared against each other.
     *
     * @sa https://en.wikipedia.org/wiki/Feature_(machine_learning)
     */
    class Feature
    {
        
    public:

        /** Construct an unnamed feature. */
        Feature(float value) :
            dm_name(),
            dm_value(value)
        {
        }
        
        /** Construct a feature whose name is a string. */
        Feature(const std::string& name, float value) :
            dm_name(),
            dm_value(value)
        {
            if (!name.empty())
            {
                dm_name.resize(name.size());
                memcpy(&dm_name[0], name.c_str(), dm_name.size());
            }
        }

        /** Construct a feature whose name is a 64-bit unsigned integer. */
        Feature(const boost::uint64_t& name, float value) :
            dm_name(),
            dm_value(value)
        {
            dm_name.resize(sizeof(boost::uint64_t));
            memcpy(&dm_name[0], &name, dm_name.size());
        }
        
        /** Get the name of this feature. */
        const FeatureName& name() const
        {
            return dm_name;
        }

        /** Get the value of this feature. */
        float value() const
        {
            return dm_value;
        }
        
    private:

        /** Name of this feature. */
        FeatureName dm_name;
        
        /** Value of this feature. */
        float dm_value;
        
    }; // class Feature

} } // namespace ArgoNavis::Clustering
