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

/** @file Declaration of the State class. */

#pragma once

#include <boost/bimap.hpp>
#include <boost/logic/tribool.hpp>
#include <map>
#include <set>
#include <stddef.h>
#include <string>
#include <vector>

#include <ArgoNavis/Clustering/FeatureName.hpp>
#include <ArgoNavis/Clustering/FeatureVector.hpp>

#include "BLAS.hpp"
#include "CompareFeatureNames.hpp"
#include "Messages.h"
#include "ThreadTable.hpp"
#include "ThreadUID.hpp"

namespace ArgoNavis { namespace Clustering { namespace Impl {

    /**
     * Current cluster analysis state for a single feature vector name.
     * Provides the necessary underlying data structures and operations
     * for implementing the actual clustering algorithms.
     */
    class State
    {
	
    public:

        /** Construct empty state for the given named feature vector. */
        State(const std::string& name);

        /** Construct state from an ANCI_State. */
        State(const ANCI_State& message);
        
        /** Type conversion to an ANCI_State. */
        operator ANCI_State() const;

        /**
         * Add another state to this state.
         *
         * @param other    State to be added to this state.
         *
         * @throw std::invalid_argument    The given state is for a different
         *                                 named feature vector than this state.
         *
         * @throw std::invalid_argument    The given state contains feature
         *                                 vectors for threads already found
         *                                 within this state.
         *
         * @throw std::invalid_argument    The given state contains unnamed
         *                                 features and this state contains
         *                                 named features. Or vice versa.
         *
         * @throw std::invalid_argument    The given state contains a different
         *                                 number of unnamed features than this
         *                                 state.
         */
        void add(const State& state);

        /**
         * Add a feature vector to this state.
         *
         * @param vector     Feature vector to be added to this state.
         * @param threads    Table of all known threads.
         *
         * @throw std::invalid_argument    The given feature vector has a
         *                                 different name than this state's
         *                                 name.
         *
         * @throw std::invalid_argument    The given feature vector is for
         *                                 a thread that is already in this
         *                                 state.
         */
        void add(const FeatureVector& vector, const ThreadTable& threads);
        
        /** Get the name of the feature vector for which this is the state. */
        const std::string& name() const
        {
            return dm_name;
        }

        /** Get the centroids of all clusters. */
        const Matrix& centroids() const
        {
            return dm_centroids;
        }

        /** Get the radii of all clusters. */
        const Vector& radii() const
        {
            return dm_radii;
        }

        /** Get the sizes (in threads) of all cluster. */
        const Vector& sizes() const
        {
            return dm_sizes;
        }

        /**
         * Join two or more existing clusters into a single cluster.
         *
         * @param rows        Row indicies of the clusters to be joined.
         * @param centroid    Centroid of the joined cluster.
         * @param radius      Radius of the joined cluster.
         */
        void join(const std::set<size_t>& rows,
                  const Vector& centroid,
                  float radius);
        
    private:

        /**
         * Type of associative container used to find the column number
         * within "dm_centroids" corresponding to a given feature name.
         */
        typedef boost::bimap<
            boost::bimaps::set_of<FeatureName, CompareFeatureNames>,
            boost::bimaps::set_of<size_t>
            > FeatureNameMap;

        /** Type of container storing an arbitrary group of ThreadUID. */
        typedef std::set<ThreadUID> ThreadUIDGroup;
        
        /**
         * Type of associative container used to find the row number within
         * "dm_clusters", "dm_centroids", and "dm_radii", corresponding to a
         * given thread unique identifier.
         */
        typedef std::map<ThreadUID, size_t> ThreadUIDMap;

        /** Name of the feature vector for which this is the state. */
        const std::string dm_name;

        /** Centroids of all clusters. */
        Matrix dm_centroids;
        
        /** Radii of all clusters. */
        Vector dm_radii;

        /** Sizes (in threads) of all clusters. */
        Vector dm_sizes;

        /** Threads within each cluster. */
        std::vector<ThreadUIDGroup> dm_clusters;
        
        /** Name of each feature in this state's feature vector. */
        FeatureNameMap dm_features;

        /** Are the features in this state's feature vector named? */
        boost::logic::tribool dm_features_named;
                
        /** Unique identifier of each thread in this state's feature vector. */
        ThreadUIDMap dm_threads;
        
    }; // class State
            
} } } // namespace ArgoNavis::Clustering::Impl
