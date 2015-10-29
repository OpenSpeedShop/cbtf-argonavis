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

/** @file Declaration of the FileName class. */

#pragma once

#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <boost/operators.hpp>
#include <iostream>
#include <string>

#include <KrellInstitute/Messages/File.h>

namespace ArgoNavis { namespace Base {

    /**
     * Unique name for a file that includes a checksum of the named file's
     * contents in addition to its full path.
     *
     * @note    The exact algorithm used to calculate the checksum is left
     *          unspecified, but can be expected to be something similar to
     *          CRC-64-ISO. This checksum is either calculated automagically
     *          upon the construction of a new FileName, or extracted from
     *          the CBTF_Protocol_FileName, as appropriate.
     */
    class FileName :
        public boost::totally_ordered<FileName>
    {
	
    public:

        /** Construct a file name from the file's full path. */
        FileName(const boost::filesystem::path& path);

        /** Construct a file name from a CBTF_Protocol_FileName. */
        FileName(const CBTF_Protocol_FileName& message);

        /** Type conversion to a Boost.Filesystem path. */
        operator boost::filesystem::path() const;

        /** Type conversion to a CBTF_Protocol_FileName. */
        operator CBTF_Protocol_FileName() const;

        /** Type conversion to a string. */
        operator std::string() const;

        /** Is this file name less than another one? */
        bool operator<(const FileName& other) const;

        /** Is this file name equal to another one? */
        bool operator==(const FileName& other) const;
        
        /** Get the full path of the named file. */
        const boost::filesystem::path& path() const
        {
            return dm_path;
        }
        
        /** Get the checksum of the named file's contents. */
        boost::uint64_t checksum() const
        {
            return dm_checksum;
        }
        
    private:

        /** Full path of the named file. */
        boost::filesystem::path dm_path;
        
        /** Checksum of the named file's contents. */
        boost::uint64_t dm_checksum;
        
    }; // class FileName

    /** Redirect a file name to an output stream. */
    std::ostream& operator<<(std::ostream& stream, const FileName& name);

} } // namespace ArgoNavis::Base
