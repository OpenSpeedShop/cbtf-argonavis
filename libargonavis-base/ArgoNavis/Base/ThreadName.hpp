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

/** @file Declaration of the ThreadName class. */

#pragma once

#include <boost/cstdint.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <iostream>
#include <string>

#include <KrellInstitute/Messages/Thread.h>

namespace ArgoNavis { namespace Base {

    /**
     * Unique name for a single thread of code execution that includes, at
     * minimum, the name of the host on which the thread is located and the
     * identifier of the process containing this thread. Also includes the
     * POSIX thread identifier for the thread when a specific thread within
     * the process is being named.
     */
    class ThreadName :
        public boost::totally_ordered<ThreadName>
    {
	
    public:

        /**
         * Construct a thread name from its individual fields.
         *
         * @param host    Name of the host on which this thread is located.
         * @param pid     Identifier for the process containing this thread.
         * @param tid     POSIX thread identifier for this thread.
         * @param rank    MPI rank of the process containing this thread.
         */
        ThreadName(const std::string& host, boost::uint64_t pid,
                   const boost::optional<boost::uint64_t>& tid =
                       boost::optional<boost::uint64_t>(),
                   const boost::optional<boost::uint32_t>& rank =
                       boost::optional<boost::uint32_t>());

        /**
         * Construct a thread name from a CBTF_Protocol_ThreadName.
         *
         * @param message    Message containing this thread name.
         */
        ThreadName(const CBTF_Protocol_ThreadName& message);

        /**
         * Type conversion to a CBTF_Protocol_ThreadName.
         *
         * @return    Message containing this thread name.
         */
        operator CBTF_Protocol_ThreadName() const;

        /**
         * Type conversion to a string.
         *
         * @return    String describing the named thread.
         */
        operator std::string() const;
        
        /**
         * Is this thread name less than another one?
         *
         * @param other    Thread name to be compared.
         * @return         Boolean "true" if this thread name is less than the
         *                 thread name to be compared, or "false" otherwise.
         */
        bool operator<(const ThreadName& other) const;

        /**
         * Is this thread name equal to another one?
         *
         * @param other    Thread name to be compared.
         * @return         Boolean "true" if the thread names are equal,
         *                 or "false" otherwise.
         */
        bool operator==(const ThreadName& other) const;

        /** Get the name of the host on which this thread is located. */
        const std::string& host() const
        {
            return dm_host;
        }

        /** Get the identifier for the process containing this thread. */
        boost::uint64_t pid() const
        {
            return dm_pid;
        }

        /** Get the POSIX thread identifier for this thread. */
        const boost::optional<boost::uint64_t>& tid() const
        {
            return dm_tid;
        }

        /** Get the MPI rank of the process containing this thread. */
        const boost::optional<boost::uint32_t>& rank() const
        {
            return dm_rank;
        }

    private:
        
        /** Name of the host on which this thread is located. */
        std::string dm_host;
        
        /** Identifier for the process containing this thread. */
        boost::uint64_t dm_pid;
        
        /** POSIX thread identifier for this thread. */
        boost::optional<boost::uint64_t> dm_tid;

        /** MPI rank of the process containing this thread. */
        boost::optional<boost::uint32_t> dm_rank;
        
    }; // class ThreadName
        
    /**
     * Redirect a thread name to an output stream.
     *
     * @param stream    Destination output stream.
     * @param name      Thread name to be redirected.
     * @return          Destination output stream.
     */
    std::ostream& operator<<(std::ostream& stream, const ThreadName& name);
            
} } // namespace ArgoNavis::Base
