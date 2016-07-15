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

/** @file Definition of the State class. */

#include <algorithm>
#include <cstring>
#include <iterator>
#include <stdexcept>
#include <utility>

#include <ArgoNavis/Base/Raise.hpp>
#include <ArgoNavis/Base/XDR.hpp>

#include "State.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::Clustering::Impl;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
State::State(const std::string& name) :
    dm_name(name),
    dm_centroids(),
    dm_radii(),
    dm_sizes(),
    dm_clusters(),
    dm_features(),
    dm_features_named(boost::logic::tribool::indeterminate_value),
    dm_threads()
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
State::State(const ANCI_State& message) :
    dm_name(message.name),
    dm_centroids(),
    dm_radii(),
    dm_sizes(),
    dm_clusters(),
    dm_features(),
    dm_features_named(boost::logic::tribool::indeterminate_value),
    dm_threads()
{
    //
    // Iterate over each feature name (if any) in the given ANCI_State and
    // construct the corresponding entries in this state.
    //

    for (u_int i = 0; i < message.features.features_len; ++i)
    {
        const ANCI_FeatureName& raw = message.features.features_val[i];

        FeatureName name(raw.name.name_len);
        
        memcpy(&name[0], raw.name.name_val,
               raw.name.name_len * sizeof(uint8_t));

        dm_features.insert(FeatureNameMap::value_type(name, i));
    }

    dm_features_named = (message.features.features_len > 0);
        
    //
    // Iterate over each cluster in the given ANCI_State and construct the
    // corresponding entries in this state. Including not only the obvious
    // dm_clusters, but also the dm_threads acceleration structure and the
    // dm_sizes cluster size vector.
    //

    size_t rows = message.clusters.clusters_len;
    
    dm_sizes.resize(rows);

    for (u_int i = 0; i < message.clusters.clusters_len; ++i)
    {
        const ANCI_ThreadUIDGroup& group = message.clusters.clusters_val[i];
                
        ThreadUIDGroup uids;
        
        for (u_int j = 0; j < group.uids.uids_len; ++j)
        {
            const ANCI_ThreadUID& uid = group.uids.uids_val[j];

            uids.insert(uid);
            dm_threads.insert(std::make_pair(uid, i));
        }

        dm_clusters.push_back(uids);
        dm_sizes(i) = static_cast<float>(uids.size());
    }
    
    //
    // Copy the centroids matrix from the given ANCI_State into this state.
    // Calculate the number of columns (features) according to the procedure
    // outlined in the documentation for ANCI_State in "Messages.x".
    //
    
    size_t columns = dm_features_named ? 
        dm_features.size() : (message.centroids.centroids_len / rows);
    
    dm_centroids.resize(rows, columns);
    
    memcpy(&dm_centroids(0, 0), message.centroids.centroids_val, 
           message.centroids.centroids_len * sizeof(float));

    // Copy the radii vector from the given ANCI_State into this state
    
    dm_radii.resize(message.radii.radii_len);
    
    memcpy(&dm_radii(0), message.radii.radii_val,
           message.radii.radii_len * sizeof(float));
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
State::operator ANCI_State() const
{
    ANCI_State message;
    memset(&message, 0, sizeof(message));

    message.name = strdup(dm_name.c_str());

    //
    // Allocate an appropriately sized array of ANCI_FeatureName within
    // the message. Iterate over each feature in this state, copying them
    // into the message.
    //

    message.features.features_len = dm_features.size();

    message.features.features_val = allocateXDRCountedArray<ANCI_FeatureName>(
        message.features.features_len
        );

    u_int n = 0;
    for (FeatureNameMap::right_const_iterator
             i = dm_features.right.begin(); i != dm_features.right.end(); ++i)
    {
        ANCI_FeatureName& name = message.features.features_val[n++];
        
        name.name.name_len = i->second.size();

        name.name.name_val = 
            allocateXDRCountedArray<uint8_t>(name.name.name_len);
        
        memcpy(name.name.name_val, &(i->second[0]), 
               name.name.name_len * sizeof(uint8_t));
    }

    //
    // Allocate an appropriately sized array of ANCI_ThreadUIDGroup within
    // the message. Iterate over each cluster in this state, copying their
    // threads into the message.
    //
    
    message.clusters.clusters_len = dm_clusters.size();

    message.clusters.clusters_val =
        reinterpret_cast<ANCI_ThreadUIDGroup*>(
            malloc(std::max(1U, message.clusters.clusters_len) *
                   sizeof(ANCI_ThreadUIDGroup))
            );

    for (u_int i = 0; i < dm_clusters.size(); ++i)
    {
        ANCI_ThreadUIDGroup& group = message.clusters.clusters_val[i];

        group.uids.uids_len = dm_clusters[i].size();
        
        group.uids.uids_val = 
            allocateXDRCountedArray<ANCI_ThreadUID>(group.uids.uids_len);

        n = 0;
        for (ThreadUIDGroup::const_iterator
                 j = dm_clusters[i].begin(); j != dm_clusters[i].end(); ++j)
        {
            group.uids.uids_val[n++] = *j;
        }
    }
    
    //
    // Allocate an appropriately sized array within the message to hold the
    // contents of the centroids matrix. Copy that matrix into the message.
    //
    
    message.centroids.centroids_len = 
        dm_centroids.size1() /* Rows */ * dm_centroids.size2() /* Columns */;
    
    message.centroids.centroids_val =
        allocateXDRCountedArray<float>(message.centroids.centroids_len);

    memcpy(message.centroids.centroids_val, &dm_centroids(0, 0),
           message.centroids.centroids_len * sizeof(float));

    //
    // Allocate an appropriately sized array within the message to hold the
    // contents of the radii vector. Copy that vector into the message.
    //

    message.radii.radii_len = dm_radii.size() /* Rows */;

    message.radii.radii_val = 
        allocateXDRCountedArray<float>(message.radii.radii_len);
    
    memcpy(message.radii.radii_val, &dm_radii(0),
           message.radii.radii_len * sizeof(float));
    
    // Return the completed ANCI_State to the caller
    return message;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void State::add(const State& state)
{
    // Check preconditions on the feature vector names

    if (state.dm_name != dm_name)
    {
        raise<std::invalid_argument>(
            "The given state is for a different named feature vector "
            "(%1%) than this state (%2%).", state.dm_name, dm_name
            );
    }

    //
    // Handle two fast paths. If the state to be added to this state is
    // empty, checked by testing if dm_features_named is indeterminate,
    // return early since there is nothing to do. If this state is empty,
    // replace its fields with the other state's fields and return early.
    // 

    if (state.dm_features_named == boost::logic::tribool::indeterminate_value)
    {
        return;
    }

    if (dm_features_named == boost::logic::tribool::indeterminate_value)
    {
        dm_centroids = state.dm_centroids;
        dm_radii = state.dm_radii;
        dm_sizes = state.dm_sizes;
        dm_clusters = state.dm_clusters;
        dm_features = state.dm_features;
        dm_features_named = state.dm_features_named;
        dm_threads = state.dm_threads;

        return;
    }

    // Check preconditions on the threads

    ThreadUIDGroup ours, theirs, common;

    for (std::vector<ThreadUIDGroup>::const_iterator
             i = dm_clusters.begin(); i != dm_clusters.end(); ++i)
    {
        ours.insert(i->begin(), i->end());
    }

    for (std::vector<ThreadUIDGroup>::const_iterator
             i = state.dm_clusters.begin(); i != state.dm_clusters.end(); ++i)
    {
        theirs.insert(i->begin(), i->end());
    }

    std::set_intersection(ours.begin(), ours.end(),
                          theirs.begin(), theirs.end(),
                          std::inserter(common, common.begin()));
    
    if (!common.empty())
    {
        raise<std::invalid_argument>(
            "The given state contains feature vectors for "
            "threads already found within this state."
            );
    }

    // Check preconditions on the features

    if ((dm_features_named == true) && (state.dm_features_named == false))
    {
        raise<std::invalid_argument>(
            "The given state contains unnamed features "
            "and this state contains named features."
            );
    }

    if ((dm_features_named == false) && (state.dm_features_named == true))
    {
        raise<std::invalid_argument>(
            "The given state contains named features "
            "and this state contains unnamed features."
            );
    }

    if ((dm_features_named == false) &&
        (dm_centroids.size2() != state.dm_centroids.size2() /* Columns */))
    {
        raise<std::invalid_argument>(
            "The given state contains a different number of "
            "unnamed features (%1%) than this state (%2%).",
            dm_centroids.size2(), state.dm_centroids.size2()            
            );
    }

    //
    // All the preconditon checking is done. Finally! Merge the cluster
    // centroid and, in the case of named features, the feature name map.
    //

    if (dm_features_named == false)
    {
        dm_centroids = vertcat(dm_centroids, state.dm_centroids);
    }
    else if (dm_features_named == true)
    {
        //
        // The following gets a little bit complicated, so some additional
        // explanation is in order. The following ASCII art illustrates the
        // composition of the new cluster centroid matrix:
        //
        //
        //       Existing     New
        //       Features   Features
        //     +----------+----------+
        //     |          |          | \
        //     | current  |  empty   |  + dm_centroids.size1()
        //     |          |          | /
        //     +----------+----------+
        //     |          |          | \
        //     | existing |  fresh   |  + state.dm_centroids.size1()
        //     |          |          | /
        //     +----------+----------+
        //       \      /   \      /
        //        ------     ------
        //          |           |
        //          |            # of features in "state" that aren't in "this"
        //          |
        //           dm_centroids.size2()
        //
        //
        // 1) The "existing" quadrant is populated by iterating over each of
        //    features in the state to be added and determining if/where they
        //    are found within this state.
        //
        //    a) When a feature IS found within this state, a single column of
        //       data is copied from the feature's location within the state to
        //       be added, to the "existing" quadrant at the feature's location
        //       within this state.
        //
        //    b) When a feature ISN'T found within this state, the feature name
        //       map is updated. An additional "mapping" map is updated, noting
        //       the feature's column number within the state to be added, and
        //       that feature's eventual column number in the "fresh" quadrant.
        //
        // 2) The width of the "fresh" and "empty" quadrants are now known. The
        //    "fresh" quadrant is populated using the "mapping" map.
        //
        // 3) The "empty" quadrant is created.
        //
        // 4) Finally, 2 vertical concatentations are applied to join "current"
        //    with "existing", and "empty" with "fresh", and then a horizontal
        //    concatenation is used to join the 2 vertical concatenations into
        //    a single new, final, cluster centroid matrix.
        //

        using namespace boost::numeric::ublas;
        
        Matrix existing(state.dm_centroids.size1() /* Rows */,
                        dm_centroids.size2() /* Columns */);
        
        std::map<std::size_t, std::size_t> mapping;

        for (FeatureNameMap::left_const_iterator 
                 i = state.dm_features.left.begin(); 
             i != state.dm_features.left.end();
             ++i)
        {
            FeatureNameMap::left_const_iterator j = 
                dm_features.left.find(i->first);

            if (j != dm_features.left.end())
            {
                column(existing, j->second) = 
                    column(state.dm_centroids, i->second);
            }
            else
            {
                dm_features.insert(
                    FeatureNameMap::value_type(
                        i->first, dm_centroids.size2() + mapping.size()
                        )
                    );
                
                mapping.insert(std::make_pair(i->second, mapping.size()));
            }
        }

        Matrix fresh(state.dm_centroids.size1() /* Rows */,
                     mapping.size() /* Columns */);

        for (std::map<std::size_t, std::size_t>::const_iterator
                 i = mapping.begin(); i != mapping.end(); ++i)
        {
            column(fresh, i->second) = column(state.dm_centroids, i->first);
        }

        Matrix empty(dm_centroids.size1() - fresh.size1() /* Rows */,
                     fresh.size2() /* Columns */);
        
        dm_centroids = horzcat(
            vertcat(dm_centroids, existing), vertcat(empty, fresh)
            );
    }

    //
    // Merging the cluster radii, sizes, and thread groups are not dependent
    // on whether or not the features are named because they have nothing to
    // do with the columns of the centroid matrix. Also make sure to update
    // the dm_threads acceleration structure along the way...
    //

    dm_radii = cat(dm_radii, state.dm_radii);
    dm_sizes = cat(dm_sizes, state.dm_sizes);

    for (size_t i = 0; i < state.dm_clusters.size(); ++i)
    {
        const ThreadUIDGroup& group = state.dm_clusters[i];
        
        dm_clusters.push_back(group);
        
        for (ThreadUIDGroup::const_iterator
                 j = group.begin(); j != group.end(); ++j)
        {
            dm_threads.insert(std::make_pair(*j, dm_clusters.size()));
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void State::add(const FeatureVector& vector, const ThreadTable& threads)
{
    if (vector.name() != dm_name)
    {
        raise<std::invalid_argument>(
            "The given feature vector has a different name (%1%) "
            "than this state's name (%2%).", vector.name(), dm_name
            );
    }

    ThreadUID uid = threads.uid(vector.thread());

    if (dm_threads.find(uid) != dm_threads.end())
    {
        raise<std::invalid_argument>(
            "The given feature vector is for a thread (%1%) "
            "that is already in this state.", vector.thread()
            );
    }

    // ...

}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void State::join(const std::set<size_t>& rows,
                 const Vector& centroid,
                 float radius)
{
    // ...
}
