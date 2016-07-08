/*******************************************************************************
** Copyright (c) 2016 Argo Navis Technologies. All Rights Reserved.
**
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
** details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA  02111-1307  USA
*******************************************************************************/

/** @file
 * Specification of the internal cluster analysis communication protocol.
 *
 * @note    The prefix "ANCI_" is being used below as as short-hand for
 *          "ArgoNavis::Clustering::Impl::", which is the namespace into
 *          which these types would be placed if XDR was more C++-aware.
 */

%#include "KrellInstitute/Messages/Thread.h"



/** Name of a feature stored as a buffer of raw, untyped, binary data. */
struct ANCI_FeatureName
{
    uint8_t name<>; /**< Name of the feature. */
};



/**
 * Unique identifier used to reference threads within the other messages below
 * without having to always resort to CBTF_Protocol_ThreadName, which is quite
 * large in comparison.
 */
typedef uint64_t ANCI_ThreadUID;



/** List containing an arbitrary group of thread unique identifiers. */
struct ANCI_ThreadUIDGroup
{
    ANCI_ThreadUID uids<>; /**< Unique identifiers of threads in the group. */
};



/**
 * Emit performance data for the specified thread. Issued by ClusteringManager
 * to request the performance data and associated address space information for
 * a single thread.
 */
struct ANCI_EmitPerformanceData
{
    ANCI_ThreadUID thread; /**< UID of the requested thread. */
};



/**
 * Current cluster analysis state for a single feature vector name.
 *
 * This state is initially produced by each ClusteringLeaf by accumlating all
 * FeatureVector with a given name produced by the FeatureGenerator, and then
 * creating clusters containing:
 *
 *     - A single thread.
 *     - A centroid equal to the feature vector for that singular thread.
 *     - A radius of zero.
 *
 * Each ClusteringFilter receives the state sent by its children, accumulates
 * them into a single new state, and applies agglomerative clustering analysis
 * to produce (hopefully) a smaller set of clusters with non-zero radii. When
 * the final state arrives in the ClusteringManager, it determines whether or
 * not the feature vector produced a "useful" clustering. And, for the feature
 * vector that did, requests the full performance for a representative thread
 * from each cluster by issuing ANCI_EmitPerformanceData messages.
 *
 * @sa http://en.wikipedia.org/wiki/Cluster_analysis
 */
struct ANCI_State
{
    /** Name of the feature vector for which this is the state. */
    string name<>;

    /**
     * Name of each feature contained in this feature vector. These feature
     * names are the columns of the "centroids" matrix below. This array is
     * empty if the feature vector contains unnamed features. In that case,
     * the number of features can be calculated as the length of "centroids"
     * divided by the length of "clusters". 
     */
    ANCI_FeatureName features<>;

    /**
     * Threads contained within each cluster for this feature vector. These
     * groups are the rows of the "centroid" matrix and "radii" vector below.
     */
    ANCI_ThreadUIDGroup clusters<>;

    /**
     * Matrix (two-dimensional array) of cluster centroids. Each centroid is
     * in the multi-dimensional space of this feature vector. This matrix is
     * stored in row-major order, with the rows and columns as noted avove.
     *
     * @sa https://en.wikipedia.org/wiki/Row-major_order
     */
    float centroids<>;

    /**
     * Vector (one-dimensional array) of cluster radii. Each radius is an
     * estimate of the maximum distance from each cluster's centroid to the
     * feature vector furthest from that centroid. This vector has rows as
     * noted above.
     */
    float radii<>;
};



/**
 * Table of all threads known to a given clustering component. Issued by both
 * ClusteringLeaf and ClusteringFilter to inform their parent of their current
 * ANCI_ThreadUID to CBTF_Protocol_ThreadName mapping.
 */
struct ANCI_ThreadTable
{
    ANCI_ThreadUID uids<>;            /**< Unique identifiers of all threads. */
    CBTF_Protocol_ThreadName names<>; /**< Name of all threads. */
};
