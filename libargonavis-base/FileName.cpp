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

/** @file Definition of the FileName class. */

#include <boost/crc.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>
#include <cstring>
#include <sstream>

#include <ArgoNavis/Base/FileName.hpp>

using namespace ArgoNavis::Base;



//------------------------------------------------------------------------------
// If the path names a real file, compute the checksum of that file's contents
// using "CRC-64" from:
//
//     http://reveng.sourceforge.net/crc-catalogue/17plus.htm#crc.cat-bits.64
//
// Comments below note the names of the template parameters to the crc_optimized
// class as listed in Boost documentation, and their correspondence to fields in
// the above-mentioned document (in parentheses).
//------------------------------------------------------------------------------
FileName::FileName(const boost::filesystem::path& path) :
    dm_path(path),
    dm_checksum(0)
{
    if (boost::filesystem::is_regular_file(path))
    {
        char buffer[1 * 1024 * 1024 /* 1 MB */];

        boost::crc_optimal<
            64,                 // Bits (width)
            0x42f0e1eba9ea3693, // TruncPoly (poly)
            0x0000000000000000, // InitRem (init)
            0x0000000000000000, // FinalXor (xorout)
            false,              // ReflectIn (refin)
            false               // ReflectRem (refout)
            > crc;
    
        boost::filesystem::ifstream stream(path, std::ios::binary);
        
        for (std::streamsize n = stream.readsome(buffer, sizeof(buffer));
             n > 0;
             n = stream.readsome(buffer, sizeof(buffer)))
        {
            crc.process_bytes(buffer, n);
        }
        
        stream.close();
        
        dm_checksum = static_cast<boost::uint64_t>(crc.checksum());
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
FileName::FileName(const CBTF_Protocol_FileName& message) :
    dm_path(message.path),
    dm_checksum(message.checksum)
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
FileName::operator boost::filesystem::path() const
{
    return dm_path;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
FileName::operator CBTF_Protocol_FileName() const
{
    CBTF_Protocol_FileName message;
    
    message.path = strdup(dm_path.c_str());
    message.checksum = dm_checksum;
    
    return message;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
FileName::operator std::string() const
{
    std::ostringstream stream;
    stream << *this;
    return stream.str();
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool FileName::operator<(const FileName& other) const
{
    if (dm_path != other.dm_path)
    {
        return dm_path < other.dm_path;
    }
    else if ((dm_checksum != 0) && (other.dm_checksum != 0))
    {
        return dm_checksum < other.dm_checksum;
    }
    else
    {
        return false;
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool FileName::operator==(const FileName& other) const
{
    if (dm_path != other.dm_path)
    {
        return false;
    }
    else if ((dm_checksum != 0) && (other.dm_checksum != 0) &&
             (dm_checksum != other.dm_checksum))
    {
        return false;
    }
    else
    {
        return true;
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::ostream& ArgoNavis::Base::operator<<(std::ostream& stream,
                                          const FileName& name)
{
    stream << boost::str(
        boost::format("0x%016X: %s") % name.checksum() % name.path()
        );
    
    return stream;
}
