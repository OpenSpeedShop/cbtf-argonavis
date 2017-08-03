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
DistanceMatrix Impl::manhattan(const Matrix& A)
{
    using namespace boost::numeric::ublas;

    if (A.size1() != A.size2())
    {
        raise<std::invalid_argument>(
            "The number of rows (%1%) and columns (%2%) in "
            "matrix A are not the same.", A.size1(), A.size2()
            );
    }

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

    if (A.size1() != A.size2())
    {
        raise<std::invalid_argument>(
            "The number of rows (%1%) and columns (%2%) in "
            "matrix A are not the same.", A.size1(), A.size2()
            );
    }

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
