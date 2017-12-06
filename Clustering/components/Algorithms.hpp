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

/** @file Declaration of the cluster analysis algorithms. */

#pragma once

#include "State.hpp"

namespace ArgoNavis { namespace Clustering { namespace Impl {

    /**
     * Default clustering algorithm.
     *
     * @param state    State to which clustering algorithm is to be applied.
     * 
     * @sa http://en.wikipedia.org/wiki/Hierarchical_clustering
     */
    void defaultClusteringAlgorithm(State& state);

    /**
     * Default fitness algorithm.
     *
     * @param state    State to which the fitness algorithm is to be applied.
     * @return         Boolean "true" if this State is considered to have
     *                 produced an interesting clustering, or "false" otherwise.
     */
    bool defaultFitnessAlgorithm(const State& state);

} } } // namespace ArgoNavis::Clustering::Impl
