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

/** @file Component ... */

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <set>
#include <typeinfo>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>

#include <KrellInstitute/Messages/ThreadEvents.h>

#include <ArgoNavis/Base/ThreadName.hpp>

using namespace ArgoNavis::Base;
using namespace KrellInstitute::CBTF;



/**
 * ...
 */
class __attribute__ ((visibility ("hidden"))) ClusteringLeaf :
    public Component
{
    
public:

    /** Factory function for this component type. */
    static Component::Instance factoryFunction()
    {
        return Component::Instance(
            reinterpret_cast<Component*>(new ClusteringLeaf())
            );
    }
    
private:

    /** Default constructor. */
    ClusteringLeaf();

    /** Handler for the "AttachedToThreads" input. */
    void handleAttachedToThreads(
        const boost::shared_ptr<CBTF_Protocol_AttachedToThreads>& message
        );

    /** Handler for the "ThreadsStateChanged" input. */
    void handleThreadsStateChanged(
        const boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged>& message
        );

    /** Names of all active (non-terminated) threads. */
    std::set<ThreadName> dm_threads;

}; // class ClusteringLeaf

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(ClusteringLeaf)



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ClusteringLeaf::ClusteringLeaf():
    Component(Type(typeid(ClusteringLeaf)), Version(1, 0, 0)),
    dm_threads()
{
    declareInput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
        "AttachedToThreads",
        boost::bind(&ClusteringLeaf::handleAttachedToThreads, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged> >(
        "ThreadsStateChanged",
        boost::bind(&ClusteringLeaf::handleThreadsStateChanged, this, _1)
        );
        
    declareOutput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
        "AttachedToThreads"
        );
    declareOutput<bool>("ThreadsFinished");
    declareOutput<bool>("EmitAddressBuffer");
    declareOutput<ThreadName>("EmitData");
    declareOutput<ThreadName>("EmitFeatures");
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringLeaf::handleAttachedToThreads(
    const boost::shared_ptr<CBTF_Protocol_AttachedToThreads>& message
    )
{
    for (u_int i = 0; i < message->threads.names.names_len; ++i)
    {
        dm_threads.insert(ThreadName(message->threads.names.names_val[i]));
    }
}



//------------------------------------------------------------------------------
// If threads are terminating, update the list of active threads and, if that
// list is empty, ...
//------------------------------------------------------------------------------
void ClusteringLeaf::handleThreadsStateChanged(
    const boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged>& message
    )
{
    if (message->state == Terminated)
    {
        for (u_int i = 0; i < message->threads.names.names_len; ++i)
        {
            dm_threads.erase(ThreadName(message->threads.names.names_val[i]));
        }
        
        if (dm_threads.empty())
        {
            // ...
            
            emitOutput<bool>("ThreadsFinished", true);
        }
    }
}
