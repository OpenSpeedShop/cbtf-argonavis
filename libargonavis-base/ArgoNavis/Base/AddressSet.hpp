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

/** @file Declaration of the AddressSet class. */

#pragma once

#include <boost/operators.hpp>
#include <set>
#include <vector>

#include <KrellInstitute/Messages/Symbol.h>

#include <ArgoNavis/Base/AddressBitmap.hpp>
#include <ArgoNavis/Base/AddressRange.hpp>

namespace ArgoNavis { namespace Base {

    /**
     * A set of memory addresses. Used to represent a non-contiguous, possibly
     * large and/or fragmented, portion of an address space.
     *
     * @sa http://en.wikipedia.org/wiki/Set_data_structure
     *
     * @todo    The interface and implementation of this class is currently
     *          as minimal as possible while still getting the job done. In
     *          the future a more extensive interface and higher performance
     *          implementation should be investigated.
     */
    class AddressSet :
        public boost::addable<AddressSet, std::set<AddressRange> >
    {
        
    public:

        /** Default constructor. */
        AddressSet();

        /**
         * Construct an address set from a CBTF_Protocol_AddressBitmap array.
         */
        AddressSet(CBTF_Protocol_AddressBitmap* messages, u_int len);
        
        /** Type conversion to a set of address ranges. */
        operator std::set<AddressRange>() const;
        
        /** Add a set of address ranges to this address set. */
        AddressSet& operator+=(const std::set<AddressRange>& ranges);

        /** Extract an address set into a CBTF_Protocol_AddressBitmap array. */
        void extract(CBTF_Protocol_AddressBitmap*& messages, u_int& len) const;
        
    private:

        /** Address bitmap(s) containing this address set. */
        std::vector<AddressBitmap> dm_bitmaps;
        
    }; // class AddressSet

} } // namespace ArgoNavis::Base
