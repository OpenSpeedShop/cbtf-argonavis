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

/** @file Definition of Boost uBLAS extensions. */

#include <stdexcept>

#include <ArgoNavis/Base/Raise.hpp>

#include "BLAS.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::Clustering;
using namespace ArgoNavis::Clustering::Impl;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Matrix Impl::horzcat(const Matrix& A, const Matrix& B)
{
    using namespace boost::numeric::ublas;
    
    if (A.size1() != B.size1())
    {
        raise<std::invalid_argument>(
            "The number of rows in matrix A (%1%) and "
            "B (%2%) are not the same.", A.size1(), B.size1()
            );
    }
    
    Matrix C(A.size1(), A.size2() + B.size2());
    
    for (size_t c = 0; c < A.size2(); ++c)
    {
        column(C, c) = column(A, c);
    }
    
    for (size_t c = 0; c < B.size2(); ++c)
    {
        column(C, A.size1() + c) = column(B, c);
    }
    
    return C;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Matrix Impl::vertcat(const Matrix& A, const Matrix& B)
{
    using namespace boost::numeric::ublas;        
    
    if (A.size2() != B.size2())
    {            
        raise<std::invalid_argument>(
            "The number of columns in matrix A (%1%) and "
            "B (%2%) are not the same.", A.size2(), B.size2()
            );
    }
    
    Matrix C(A.size1() + B.size1(), A.size2());
    
    for (size_t r = 0; r < A.size1(); ++r)
    {
        row(C, r) = row(A, r);
    }

    for (size_t r = 0; r < B.size1(); ++r)
    {
        row(C, A.size2() + r) = row(B, r);
    }
    
    return C;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Vector Impl::cat(const Vector& A, const Vector& B)
{
    using namespace boost::numeric::ublas;
    
    Vector C(A.size() + B.size());
    
    subrange(C, 0, A.size()) = A;

    subrange(C, A.size(), A.size() + B.size()) = B;
    
    return C;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
MatrixCoordinates Impl::min_element(const DistanceMatrix& A)
{
    MatrixCoordinates coordinates(0, 0);
    float value = A(0, 0);

    for (size_t r = 0; r < A.size1(); ++r)
    {
        for (size_t c = r + 1; c < A.size2(); ++c)
        {
            if (A(r, c) < value)
            {
                coordinates = boost::make_tuple(r, c);
                value = A(r, c);
            }
        }
    }
    
    return coordinates;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
MatrixCoordinates Impl::max_element(const DistanceMatrix& A)
{
    MatrixCoordinates coordinates(0, 0);
    float value = A(0, 0);

    for (size_t r = 0; r < A.size1(); ++r)
    {
        for (size_t c = r + 1; c < A.size2(); ++c)
        {
            if (A(r, c) > value)
            {
                coordinates = boost::make_tuple(r, c);
                value = A(r, c);
            }
        }
    }
    
    return coordinates;    
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DistanceMatrix Impl::manhattan(const Matrix& A)
{
    using namespace boost::numeric::ublas;

    DistanceMatrix B(A.size1(), A.size1());
        
    for (size_t ri = 0; ri < A.size1(); ++ri)
    {
        for (size_t rj = ri + 1; rj < A.size1(); ++rj)
        {
            B(ri, rj) = norm_1(row(A, ri) - row(A, rj));
        }
    }
    
    return B;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DistanceMatrix Impl::euclidean(const Matrix& A)
{
    using namespace boost::numeric::ublas;

    DistanceMatrix B(A.size1(), A.size1());
        
    for (size_t ri = 0; ri < A.size1(); ++ri)
    {
        for (size_t rj = ri + 1; rj < A.size1(); ++rj)
        {
            B(ri, rj) = norm_2(row(A, ri) - row(A, rj));
        }
    }
    
    return B;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DistanceMatrix Impl::complete_linkage(const DistanceMatrix& distance,
                                      const Vector& radii)
{
    if (distance.size1() != radii.size())
    {            
        raise<std::invalid_argument>(
            "The order of the distance matrix (%1%) and the radii vector "
            "(%2%) are not the same.", distance.size1(), radii.size()
            );
    }

    DistanceMatrix A = distance;

    for (size_t ri = 0; ri < radii.size(); ++ri)
    {
        for (size_t rj = ri + 1; rj < radii.size(); ++rj)
        {
            A(ri, rj) += radii(ri) + radii(rj);
        }
    }

    return A;    
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DistanceMatrix Impl::single_linkage(const DistanceMatrix& distance,
                                    const Vector& radii)
{
    if (distance.size1() != radii.size())
    {            
        raise<std::invalid_argument>(
            "The order of the distance matrix (%1%) and the radii vector "
            "(%2%) are not the same.", distance.size1(), radii.size()
            );
    }

    DistanceMatrix A = distance;

    for (size_t ri = 0; ri < radii.size(); ++ri)
    {
        for (size_t rj = ri + 1; rj < radii.size(); ++rj)
        {
            A(ri, rj) -= radii(ri) + radii(rj);
        }
    }

    return A;    
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Sphere Impl::enclosing(const Sphere& A, const Sphere& B)
{
    if (boost::get<0>(A).size() != boost::get<0>(B).size())
    {
        raise<std::invalid_argument>(
            "The number of dimensions in sphere A (%1%) and B (%2%) are not "
            "the same.", boost::get<0>(A).size(), boost::get<0>(B).size()
            );
    }

    // Compute the vector AB from the centroid of sphere A to the centroid
    // of sphere B and normalize AB. Then use AB to construct a point C on
    // sphere A, and a point D on sphere B, that both lie on the line that
    // connects the centroids of the two spheres.

    Vector AB = boost::get<0>(B) - boost::get<0>(A);

    AB = AB / norm_2(AB);

    Vector C = boost::get<0>(A) - (AB * boost::get<1>(A));
    Vector D = boost::get<0>(B) + (AB * boost::get<1>(B));

    // The centroid of the minimum bounding sphere of spheres A and B is
    // the mid-point between C and D. And its radius is equal to half the
    // length of the vector between C and D.
    
    Vector centroid = 0.5 * (C + D);
    
    float radius = 0.5 * norm_2(D - C);

    return boost::make_tuple(centroid, radius);
}
