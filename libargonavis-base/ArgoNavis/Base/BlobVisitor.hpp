////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014,2015 Argo Navis Technologies. All Rights Reserved.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
// details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library; if not, write to the Free Software Foundation, Inc.,
// 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////

/** @file Declaration of the BlobVisitor type. */

#pragma once

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include <KrellInstitute/Messages/Blob.h>

namespace ArgoNavis { namespace Base {

    /**
     * Type of function invoked when visiting CBTF_Protocol_Blob objects. Used
     * with implicit iterations, a shared pointer to the CBTF_Protocol_Blob is
     * passed as a parameter to the function, and the function returns either
     * "true" to continue the iteration or "false" to terminate it.
     *
     * @note    The usage of the term "visitor" here does <em>not</em>
     *          refer to the design pattern of the same name.
     *
     * @sa http://en.wikipedia.org/wiki/Iterator#Implicit_iterators
     * @sa http://en.wikipedia.org/wiki/Visitor_pattern
     */
    typedef boost::function<
        bool (const boost::shared_ptr<CBTF_Protocol_Blob>&)
        > BlobVisitor;
    
} } // namespace ArgoNavis::Base
