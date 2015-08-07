////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013 Krell Institute. All Rights Reserved.
// Copyright (c) 2015 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration of the MappingVisitor type. */

#pragma once

#include <boost/function.hpp>

#include <ArgoNavis/Base/AddressRange.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

namespace ArgoNavis { namespace Base {

    class LinkedObject;

    /**
     * Type of function invoked when visiting one or more mappings of a linked
     * object into the address space of a thread. Used with implicit iterations,
     * a reference to the ThreadName and LinkedObject, along with the address
     * range and time interval of the mapping, are passed as parameters to the
     * function, and the fuction returns either "true" to continue the iteration
     * or "false" to terminate it.
     *
     * @note    The usage of the term "visitor" here does <em>not</em> refer
     *          to the design pattern of the same name.
     *
     * @sa http://en.wikipedia.org/wiki/Iterator#Implicit_iterators
     * @sa http://en.wikipedia.org/wiki/Visitor_pattern
     */
    typedef boost::function<
        bool (const ThreadName&,
              const LinkedObject&,
              const AddressRange&,
              const TimeInterval&)
        > MappingVisitor;
    
} } // namespace ArgoNavis::Base
