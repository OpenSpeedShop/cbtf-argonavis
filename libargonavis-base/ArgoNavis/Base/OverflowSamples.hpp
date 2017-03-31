////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration of the OverflowSamples class. */

#pragma once

#include <boost/cstdint.hpp>
#include <map>
#include <string>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/AddressRange.hpp>
#include <ArgoNavis/Base/OverflowSampleVisitor.hpp>

namespace ArgoNavis { namespace Base {

    /**
     * Zero or more samples associated with specific addresses.
     */
    class OverflowSamples
    {
        
    public:
        
        /**
         * Construct an empty container.
         *
         * @param name    Name of these samples.
         */
        OverflowSamples(const std::string& name);

        /** Get the name of these samples. */
        const std::string& name() const
        {
            return dm_name;
        }

        /** Smallest address range containing all of these samples. */
        AddressRange range() const;
        
        /** Add a new sample. */
        void add(const Address& address, boost::uint64_t value);
        
        /**
         * Visit those samples within the specified address range.
         *
         * @note    The visitation is terminated immediately if "false"
         *          is returned by the visitor.
         *
         * @param range      Address range for the visitation.
         * @param visitor    Visitor invoked for each sample.
         */
        void visit(const AddressRange& range,
                   const OverflowSampleVisitor& visitor) const;
        
    private:

        /** Name of these samples. */
        std::string dm_name;

        /** Individual samples indexed by address. */
        std::map<Address, boost::uint64_t> dm_samples;
        
    }; // class OverflowSamples

} } // namespace ArgoNavis::Base
