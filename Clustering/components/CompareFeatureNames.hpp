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

/** @file Declaration and definition of the CompareFeatureNames functor. */

#pragma once

#include <ArgoNavis/Clustering/FeatureName.hpp>

namespace ArgoNavis { namespace Clustering { namespace Impl {

    /**
     * Functor providing a strict weak ordering relationship for feature names.
     */
    struct CompareFeatureNames
    {
        bool operator()(const FeatureName& lhs, const FeatureName& rhs) const
        {
            if (lhs.size() < rhs.size())
            {
                return true;
            }
            else if (lhs.size() > rhs.size())
            {
                return false;
            }
            else
            {
                for (FeatureName::size_type i = 0; i < lhs.size(); ++i)
                {
                    if (lhs[i] < rhs[i])
                    {
                        return true;
                    }
                    else if (lhs[i] > rhs[i])
                    {
                        return false;
                    }
                }
            }
            
            return false;
        }
    }; // struct CompareFeatureNames

} } } // namespace ArgoNavis::Clustering::Impl
