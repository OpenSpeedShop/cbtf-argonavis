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

namespace ArgoNavis { namespace Clustering { namespace Impl {

    /** Type of Boost uBLAS distance matrices used for the cluster analysis. */
    typedef boost::numeric::ublas::triangular_matrix<
        float, boost::numeric::ublas::strict_upper
        > DistanceMatrix;
    
    /** Type of Boost uBLAS (general) matrices used for the cluster analysis. */
    typedef boost::numeric::ublas::matrix<float> Matrix;

    /** Type of Boost uBLAS (general) vectors used for the cluster analysis. */
    typedef boost::numeric::ublas::vector<float> Vector;
    
    /** Compute the Manhattan distance matrix for pairwise rows of a matrix. */
    DistanceMatrix manhattan(const Matrix& A);

    /** Compute the Euclidean distance matrix for pairwise rows of a matrix. */
    DistanceMatrix euclidean(const Matrix& A);
    
    /** Concatenate two matrices horizontally. */
    Matrix horzcat(const Matrix& A, const Matrix& B);

    /** Concatenate two matrices vertically. */
    Matrix vertcat(const Matrix& A, const Matrix& B);

    /** Concatenate two vectors. */
    Vector cat(const Vector& A, const Vector& B);

} } } // namespace ArgoNavis::Clustering::Impl
