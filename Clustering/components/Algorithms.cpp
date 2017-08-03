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

/** @file Definition of the cluster analysis algorithms. */

#include <boost/numeric/ublas/vector_expression.hpp>

#include <KrellInstitute/CBTF/Impl/MRNet.hpp>

#include "Algorithms.hpp"

using namespace ArgoNavis::Clustering;
using namespace ArgoNavis::Clustering::Impl;



/** Anonymous namespace hiding implementation details. */
namespace {

    // ...
    
} // namespace <anonymous>



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void Impl::defaultClusteringAlgorithm(State& state)
{
    using namespace boost::numeric::ublas;

    //
    // Near the leaf nodes it is common to have a relatively small number of
    // single-threaded "clusters". When employing a distributed, hierarchical,
    // algorithm such as this one, it can be deterimental to begin clustering
    // before a sufficiently broad cross section of data has been accumulated.
    // Therefore don't begin clustering until the input State contains 64 or
    // more single-threaded clusters. Unless the frontend is reached without
    // having reached that number, in which case the total number of threads
    // is small and clustering analysis should be applied anyway.
    // 
    
    if (!KrellInstitute::CBTF::Impl::TheTopologyInfo.IsFrontend &&
        (state.sizes().size() < 64) && (norm_inf(state.sizes()) <= 1.0f))
    {
        return;
    }

    //
    // The input State may be the result of aggregating the State objects
    // from multiple nodes below this one. So it possible that one or more
    // of the existing clusters completely contain other existing clusters.
    // Find all occurences of this condition and join any such clusters.
    //
    
    for (bool finished = false; !finished;)
    {
        const Matrix& centroids = state.centroids();
        const Vector& radii = state.radii();       
        
        const size_t N = state.sizes().size();
        
        bool found = false;
        
        for (size_t r = 0; !found && (r < N); ++r)
        {
            //
            // Compute the Euclidean distance from the centroid of each
            // cluster to the centroid of the current reference cluster
            // "r". Then add the radii of each cluster to this distance,
            // giving the maximum possible distance of each cluster from
            // the centroid of the current reference cluster.
            //
            
            Vector distances(N);

            for (size_t i = 0; i < N; ++i)
            {
                distances(i) = norm_2(row(centroids, i) - row(centroids, r));
            }

            distances += radii;

            //
            // Now search for clusters whose maximum possible distance
            // from the centroid of the current reference cluster "r" is
            // less than or equal to the radius of the current reference
            // cluster. Such clusters are entirely contained within the
            // current reference cluster and should be joined with it.
            //
            
            std::set<size_t> joinable;

            for (size_t i = 0; i < N; ++i)
            {
                if (distances(i) < radii(r))
                {
                    joinable.insert(i);
                }
            }

            //
            // Perform the join if there is more than one cluster within
            // the "joinable" set (the current reference cluster "r" must
            // always be a member of the set). The centroid and radius of
            // the new, joined, cluster, are simply those of the current
            // reference cluster because all of the others are contained
            // within it.
            //
            
            if (joinable.size() > 1)
            {
                state.join(joinable, row(centroids, r), radii(r));
                found = true;
            }
        }
        
        finished = !found;
    }
    
    //
    // ...
    //

    for (bool finished = false; !finished;)
    {
        // ...
    }
}



bool Impl::defaultFitnessAlgorithm(const State& state)
{
    // ...

    return false;
}
