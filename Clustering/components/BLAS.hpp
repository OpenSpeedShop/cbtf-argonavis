////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2017 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration of Boost uBLAS types and extensions. */

#pragma once

#include <boost/numeric/ublas/triangular.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/vector_proxy.hpp>
#include <boost/tuple/tuple.hpp>

namespace ArgoNavis { namespace Clustering { namespace Impl {

    /** Type of Boost uBLAS distance matrices used for the cluster analysis. */
    typedef boost::numeric::ublas::triangular_matrix<
        float, boost::numeric::ublas::strict_upper
        > DistanceMatrix;
    
    /** Type of Boost uBLAS (general) matrices used for the cluster analysis. */
    typedef boost::numeric::ublas::matrix<float> Matrix;

    /** Type of Boost uBLAS (general) vectors used for the cluster analysis. */
    typedef boost::numeric::ublas::vector<float> Vector;

    /** Type representing coordinates in a Boost uBLAS matrix. */
    typedef boost::tuple<
        size_t /* Row */, size_t /* Column */
        > MatrixCoordinates;

    /** Type representing a multi-dimensional sphere's centroid and radius. */
    typedef boost::tuple<Vector /* Centroid */, float /* Radius */> Sphere;

    /** Concatenate two matrices horizontally. */
    Matrix horzcat(const Matrix& A, const Matrix& B);

    /** Concatenate two matrices vertically. */
    Matrix vertcat(const Matrix& A, const Matrix& B);

    /** Concatenate two vectors. */
    Vector cat(const Vector& A, const Vector& B);
    
    /** Find the coordinates of the minimum element in a distance matrix. */
    MatrixCoordinates min(const DistanceMatrix& A);

    /** Find the coordinates of the maximum element in a distance matrix. */
    MatrixCoordinates max(const DistanceMatrix& A);

    /** Compute the Manhattan distance matrix for pairwise rows of a matrix. */
    DistanceMatrix manhattan(const Matrix& A);

    /** Compute the Euclidean distance matrix for pairwise rows of a matrix. */
    DistanceMatrix euclidean(const Matrix& A);

    /**
     * Compute the complete linkage distance (maximum distance between any two
     * elements) matrix for the specified distance matrix and cluster radii.
     *
     * @sa http://en.wikipedia.org/wiki/Complete-linkage_clustering
     */
    DistanceMatrix complete_linkage(const DistanceMatrix& distance,
                                    const Vector& radii);

    /**
     * Compute the single linkage distance (minimum distance between any two
     * elements) matrix for the specified distance matrix and cluster radii.
     * 
     * @sa http://en.wikipedia.org/wiki/Single-linkage_clustering
     */
    DistanceMatrix single_linkage(const DistanceMatrix& distance,
                                  const Vector& radii);

    /** Compute the minimum bounding sphere of the two specified spheres. */
    Sphere enclosing(const Sphere& A, const Sphere& B);

} } } // namespace ArgoNavis::Clustering::Impl
