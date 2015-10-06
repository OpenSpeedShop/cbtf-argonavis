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

/** @file Definition of the ThreadName class. */

#include <boost/format.hpp>
#include <cstring>
#include <sstream>

#include <ArgoNavis/Base/ThreadName.hpp>

using namespace ArgoNavis::Base;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ThreadName::ThreadName(const std::string& host, boost::uint64_t pid,
                       const boost::optional<boost::uint64_t>& tid,
                       const boost::optional<boost::uint32_t>& mpi_rank,
                       const boost::optional<boost::uint32_t>& omp_rank) :
    dm_host(host),
    dm_pid(pid),
    dm_tid(tid),
    dm_mpi_rank(mpi_rank),
    dm_omp_rank(omp_rank)
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ThreadName::ThreadName(const CBTF_DataHeader& message) :
    dm_host(message.host),
    dm_pid(static_cast<boost::uint64_t>(message.pid)),
    dm_tid(),
    dm_mpi_rank(),
    dm_omp_rank()
{
    if (message.posix_tid > 0)
    {
        dm_tid = static_cast<boost::uint64_t>(message.posix_tid);
    }
    if (message.rank >= 0)
    {
        dm_mpi_rank = static_cast<boost::uint32_t>(message.rank);
    }
    if (message.omp_tid >= 0)
    {
        dm_omp_rank = static_cast<boost::uint32_t>(message.omp_tid);
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ThreadName::ThreadName(const CBTF_Protocol_ThreadName& message) :
    dm_host(message.host),
    dm_pid(static_cast<boost::uint64_t>(message.pid)),
    dm_tid(),
    dm_mpi_rank(),
    dm_omp_rank()
{
    if (message.has_posix_tid)
    {
        dm_tid = static_cast<boost::uint64_t>(message.posix_tid);
    }
    if (message.rank >= 0)
    {
        dm_mpi_rank = static_cast<boost::uint32_t>(message.rank);
    }
    if (message.omp_tid >= 0)
    {
        dm_omp_rank = static_cast<boost::uint32_t>(message.omp_tid);
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ThreadName::operator CBTF_DataHeader() const
{
    CBTF_DataHeader message;
    memset(&message, 0, sizeof(message));

    strncpy(message.host, dm_host.c_str(), sizeof(message.host) - 1);
    message.pid = static_cast<int64_t>(dm_pid);
    message.posix_tid = dm_tid ? static_cast<int64_t>(*dm_tid) : 0;
    message.rank = dm_mpi_rank ? static_cast<int32_t>(*dm_mpi_rank) : -1;
    message.omp_tid = dm_omp_rank ? static_cast<int32_t>(*dm_omp_rank) : -1;
    
    return message;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ThreadName::operator CBTF_Protocol_ThreadName() const
{
    CBTF_Protocol_ThreadName message;
    memset(&message, 0, sizeof(message));

    message.host = strdup(dm_host.c_str());
    message.pid = static_cast<int64_t>(dm_pid);
    message.has_posix_tid = dm_tid ? true : false;
    message.posix_tid = dm_tid ? static_cast<int64_t>(*dm_tid) : 0;
    message.rank = dm_mpi_rank ? static_cast<int32_t>(*dm_mpi_rank) : -1;
    message.omp_tid = dm_omp_rank ? static_cast<int32_t>(*dm_omp_rank) : -1;
    
    return message;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ThreadName::operator std::string() const
{
    std::ostringstream stream;
    stream << *this;
    return stream.str();
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool ThreadName::operator<(const ThreadName& other) const
{
    if (dm_host != other.dm_host)
    {
        return dm_host < other.dm_host;
    }
    else if (dm_pid != other.dm_pid)
    {
        return dm_pid < other.dm_pid;
    }
    else if (dm_tid && other.dm_tid)
    {
        return *dm_tid < *other.dm_tid;
    }
    else
    {
        return !dm_tid && other.dm_tid;
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool ThreadName::operator==(const ThreadName& other) const
{
    if (dm_host != other.dm_host)
    {
        return false;
    }
    else if (dm_pid != other.dm_pid)
    {
        return false;
    }
    else if (dm_tid && other.dm_tid)
    {
        return *dm_tid == *other.dm_tid;
    }
    else
    {
        return !dm_tid && !other.dm_tid;
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
std::ostream& ArgoNavis::Base::operator<<(std::ostream& stream,
                                          const ThreadName& name)
{
    if (name.mpi_rank())
    {
        stream << boost::str(
            boost::format("MPI Rank %u") % *name.mpi_rank()
            );
    }
    else
    {
        stream << boost::str(
            boost::format("Host \"%s\", PID %llu") % name.host() % name.pid()
            );        
    }

    if (name.omp_rank())
    {
        stream << boost::str(
            boost::format(", OpenMP Rank %u") % *name.omp_rank()
            );
    }
    else if (name.tid())
    {
        stream << boost::str(
            boost::format(", TID 0x%016X") % *name.tid()
            );
    }
    
    return stream;
}
