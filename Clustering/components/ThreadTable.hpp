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

/** @file Declaration of the ThreadTable class. */

#pragma once

#include <boost/bimap.hpp>
#include <boost/operators.hpp>

#include <ArgoNavis/Base/ThreadName.hpp>

#include "Messages.h"
#include "ThreadUID.hpp"

namespace ArgoNavis { namespace Clustering { namespace Impl {

    /**
     * Table of all threads known to a given clustering component. Associates
     * a unique identifier with each thread, allowing them to be referenced
     * by cluster analysis state in relatively compact form without having to
     * resort to their full thread name.
     */
    class ThreadTable :
        public boost::addable<ThreadTable>,
        public boost::addable<ThreadTable, Base::ThreadName>
    {
	
    public:

        /** Construct an empty thread table. */
        ThreadTable();

        /** Construct a thread table from an ANCI_ThreadTable. */
        ThreadTable(const ANCI_ThreadTable& message);

        /** Type conversion to an ANCI_ThreadTable. */
        operator ANCI_ThreadTable() const;
        
        /**
         * Add another thread table to this thread table.
         *
         * @param other    Thread table to be added to this thread table.
         * @return         This thread table.
         *
         * @throw std::invalid_argument    The given thread table contained
         *                                 a thread name and unique identifer
         *                                 pairing that contradicted the one
         *                                 in this thread table.
         */
        ThreadTable& operator+=(const ThreadTable& other);

        /** Add a thread to this thread table. */
        ThreadTable& operator+=(const Base::ThreadName& thread);

        /** Does this thread table contain a thread name? */
        bool contains(const Base::ThreadName& thread);

        /** Does this thread table contain a unique identifier? */
        bool contains(const ThreadUID& uid);

        /**
         * Get the unique identifier for the given thread name.
         *
         * @param thread    Name of the thread to be found.
         * @return          Unique identifier for that thread.
         *
         * @throw std::invalid_argument    The given thread isn't contained
         *                                 within this thread table.
         */
        ThreadUID uid(const Base::ThreadName& thread);

        /**
         * Get the name for the given thread unique identifier.
         *
         * @param uid    Unique identifier of the thread to be found.
         * @return       Name of that thread.
         *
         * @throw std::invalid_argument    The given thread isn't contained
         *                                 within this thread table.
         */        
        Base::ThreadName name(const ThreadUID& uid);
        
    private:

        /** Get the next available thread unique identifier for this node. */
        static ThreadUID next();
        
        /** Map of thread unique identifiers their corresponding names. */
        boost::bimap<ThreadUID, Base::ThreadName> dm_threads;
        
    }; // class ThreadTable
            
} } } // namespace ArgoNavis::Clustering::Impl
