////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2010,2013 Krell Institute. All Rights Reserved.
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

/** @file Declaration and definition of the raise() function. */

#pragma once

#include <boost/format.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/repetition/enum_trailing_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_trailing_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>

namespace ArgoNavis { namespace Base {

    /**
     * @fn raise
     *
     * Function making it more convenient to throw C++ exceptions that accept
     * a single C++ string argument describing the exception. Provides string
     * formatting using printf-style variable length argument lists.
     *
     * Implemented as a set of overriden functions that are built on top of
     * the Boost Format library and have all the advantages thereof. Format
     * strings must follow the rules of boost::format() format strings, and
     * also support printf-style and placeholder formatting strings.
     *
     * @note    A maximum of 25 arguments is current supported. This can be
     *          changed by modifying the KRELL_INSTITUTE_BASE_RAISE_MAXARGS
     *          macro definition.
     *
     * @note    The variadic template argument support present in the C++0x
     *          standard will eventually greatly simplify the implementation
     *          of this function.
     *
     * @sa http://www.boost.org/doc/libs/release/libs/format/index.html
     */

    #define KRELL_INSTITUTE_BASE_RAISE_MAXARGS 25

    #define KRELL_INSTITUTE_BASE_RAISE_ARG(z, n, unused) \
        formatter % arg##n ;

    #define KRELL_INSTITUTE_BASE_RAISE(z, n, unused)                     \
        template <                                                       \
            typename ExceptionType                                       \
            BOOST_PP_ENUM_TRAILING_PARAMS_Z(z, n, typename T)            \
            >                                                            \
        void raise(                                                      \
            const char* format                                           \
            BOOST_PP_ENUM_TRAILING_BINARY_PARAMS_Z(z, n, const T, & arg) \
            )                                                            \
        {                                                                \
            boost::format formatter(format);                             \
            BOOST_PP_REPEAT(n, KRELL_INSTITUTE_BASE_RAISE_ARG, ~)        \
            throw ExceptionType(formatter.str());                        \
        }

    BOOST_PP_REPEAT(BOOST_PP_INC(KRELL_INSTITUTE_BASE_RAISE_MAXARGS), \
                    KRELL_INSTITUTE_BASE_RAISE, ~)

} } // namespace ArgoNavis::Base
