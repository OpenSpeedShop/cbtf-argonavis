////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration and definition of the SimpleThreadName class. */

#pragma once

#include <boost/operators.hpp>
#include <stdint.h>
#include <string>

#include <KrellInstitute/Messages/Thread.h>

/**
 * Simplification of the ThreadName class, limited to the host
 * name, process identifier, and POSIX thread identifier only.
 */
class SimpleThreadName :
    private boost::equivalent<SimpleThreadName>,
    private boost::totally_ordered<SimpleThreadName>
{
    
public:
    
    /** Constructor from individual fields. */
    SimpleThreadName(const std::string& host,
                     const int64_t& pid,
                     const int64_t& tid) :
        dm_host(host),
        dm_pid(pid),
        dm_tid(tid)
    {            
    }
    
    /** Constructor from a CBTF_Protocol_ThreadName. */
    SimpleThreadName(const CBTF_Protocol_ThreadName& name) :
        dm_host(name.host),
        dm_pid(name.pid),
        dm_tid(name.has_posix_tid ? name.posix_tid : 0)
    {
    }
    
    /** Is this simple thread name less than another one? */
    bool operator<(const SimpleThreadName& other) const
    {
        if(dm_host < other.dm_host)
            return true;
        if(dm_host > other.dm_host)
            return false;
        if(dm_pid < other.dm_pid)
            return true;
        if(dm_pid > other.dm_pid)
            return false;
        return dm_tid < other.dm_tid;
    }
    
private:
    
    /** Name of the host on which this thread is located. */
    std::string dm_host;
    
    /** Identifier of the process containing this thread. */
    int64_t dm_pid;
    
    /** POSIX identifier of the thread. */
    int64_t dm_tid;
    
}; // class SimpleThreadName
