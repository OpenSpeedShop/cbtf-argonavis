////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Argo Navis Technologies. All Rights Reserved.
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

/** @file Definition of the ThreadTable class. */

#include <algorithm>
#include <boost/cstdint.hpp>
#include <cstring>
#include <stdexcept>

#include <KrellInstitute/CBTF/Impl/MRNet.hpp>

#include <ArgoNavis/Base/Raise.hpp>
#include <ArgoNavis/Base/XDR.hpp>

#include "ThreadTable.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::Clustering::Impl;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ThreadTable::ThreadTable() :
    dm_threads()
{
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ThreadTable::ThreadTable(const ANCI_ThreadTable& message) :
    dm_threads()
{
    u_int size = std::min(message.uids.uids_len, message.names.names_len);

    for (u_int i = 0; i < size; ++i)
    {
        dm_threads.insert(
            boost::bimap<ThreadUID, ThreadName>::value_type(
                message.uids.uids_val[i], message.names.names_val[i]
                )
            );
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ThreadTable::operator ANCI_ThreadTable() const
{
    ANCI_ThreadTable message;
    memset(&message, 0, sizeof(message));

    message.uids.uids_len = dm_threads.size();

    message.uids.uids_val =
        allocateXDRCountedArray<ANCI_ThreadUID>(message.uids.uids_len);
    
    message.names.names_len = dm_threads.size();
    
    message.names.names_val = allocateXDRCountedArray<CBTF_Protocol_ThreadName>(
        message.names.names_len
        );
    
    u_int n = 0;
    for (boost::bimap<ThreadUID, ThreadName>::const_iterator
             i = dm_threads.begin(); i != dm_threads.end(); ++i, ++n)
    {
        message.uids.uids_val[n] = i->left;
        message.names.names_val[n] = i->right;
    }
    
    return message;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ThreadTable::add(const ThreadTable& threads)
{
    for (boost::bimap<ThreadUID, ThreadName>::const_iterator
             i = threads.dm_threads.begin(); i != threads.dm_threads.end(); ++i)
    {
        boost::bimap<ThreadUID, ThreadName>::right_const_iterator j =
            dm_threads.right.find(i->right);
        
        if (j == dm_threads.right.end())
        {
            dm_threads.insert(
                boost::bimap<ThreadUID, ThreadName>::value_type(
                    i->left, i->right
                    )
                );
        }
        else if (i->left != j->second)
        {
            raise<std::runtime_error>(
                "The given thread table contained a thread name and unique "
                "identifier pairing (%s : 0x%016X) that contradicted the one "
                "(%s : 0x%016X) in this thread table.",
                i->right, i->left, j->first, j->second
                );
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ThreadTable::add(const ThreadName& thread)
{
    boost::bimap<ThreadUID, ThreadName>::right_const_iterator i =
        dm_threads.right.find(thread);

    if (i == dm_threads.right.end())
    {
        dm_threads.insert(
            boost::bimap<ThreadUID, ThreadName>::value_type(next(), thread)
            );
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool ThreadTable::contains(const Base::ThreadName& thread) const
{
    return dm_threads.right.find(thread) != dm_threads.right.end();
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool ThreadTable::contains(const ThreadUID& uid) const
{
    return dm_threads.left.find(uid) != dm_threads.left.end();
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ThreadUID ThreadTable::uid(const ThreadName& thread) const
{
    boost::bimap<ThreadUID, ThreadName>::right_const_iterator i =
        dm_threads.right.find(thread);

    if (i == dm_threads.right.end())
    {
        raise<std::runtime_error>(
            "The given thread (%1%) isn't contained "
            "within this thread table.", thread
            );
    }
    
    return i->second;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ThreadName ThreadTable::name(const ThreadUID& uid) const
{
    boost::bimap<ThreadUID, ThreadName>::left_const_iterator i =
        dm_threads.left.find(uid);

    if (i == dm_threads.left.end())
    {
        raise<std::runtime_error>(
            "The given thread (0x%016X) isn't contained "
            "within this thread table.", uid
            );
    }
    
    return i->second;
}



//------------------------------------------------------------------------------
// Note that this method, as written, is NOT thread safe!
//------------------------------------------------------------------------------
ThreadUID ThreadTable::next()
{
    static boost::uint32_t NextUID = 0;

    boost::uint64_t rank = KrellInstitute::CBTF::Impl::TheTopologyInfo.Rank;
    boost::uint64_t uid = NextUID++;

    return (rank << 32) | uid;
}
