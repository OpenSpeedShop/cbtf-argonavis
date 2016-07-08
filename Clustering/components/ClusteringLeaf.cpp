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

/** @file Cluster analysis leaf component. */

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <list>
#include <set>
#include <typeinfo>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>

#include <KrellInstitute/Core/AddressBuffer.hpp>

#include <KrellInstitute/Messages/LinkedObjectEvents.h>
#include <KrellInstitute/Messages/ThreadEvents.h>

#include <ArgoNavis/Base/AddressSpaces.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>

#include <ArgoNavis/Clustering/FeatureVector.hpp>

#include "Messages.h"
#include "ThreadTable.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::Clustering;
using namespace ArgoNavis::Clustering::Impl;
using namespace KrellInstitute::CBTF;
using namespace KrellInstitute::Core;



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

    /** Generate and emit the initial cluster analysis state. */
    void emitState();
    
    /** Handler for the "AttachedToThreads" input. */
    void handleAttachedToThreads(
        const boost::shared_ptr<CBTF_Protocol_AttachedToThreads>& message
        );

    /** Handler for the "EmitPerformanceData" input. */
    void handleEmitPerformanceData(
        const boost::shared_ptr<ANCI_EmitPerformanceData>& message
        );
    
    /** Handler for the "Feature" input. */
    void handleFeature(const FeatureVector& feature);
    
    /** Handler for the "InitialLinkedObjects" input. */
    void handleInitialLinkedObjects(
        const boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>& message
        );
    
    /** Handler for the "LoadedLinkedObject" input. */
    void handleLoadedLinkedObject(
        const boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject>& message
        );

    /** Handler for the "ObservedAddress" input. */
    void handleObservedAddress(const ArgoNavis::Base::Address& address);
    
    /** Handler for the "ThreadsStateChanged" input. */
    void handleThreadsStateChanged(
        const boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged>& message
        );

    /** Handler for the "UnloadedLinkedObject" input. */
    void handleUnloadedLinkedObject(
        const boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject>& message
        );

    /** Names of all active (non-terminated) threads. */
    std::set<ThreadName> dm_active;

    /** Address buffer containing all observed addresses. */
    AddressBuffer dm_addresses;

    /** List of feature vectors for all observed threads. */
    std::list<FeatureVector> dm_features;

    /** Names of all inactive (terminated) threads. */
    std::set<ThreadName> dm_inactive;
        
    /** Address spaces of all observed threads. */
    AddressSpaces dm_spaces;

    /** Table of all observed threads. */
    ThreadTable dm_threads;

}; // class ClusteringLeaf

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(ClusteringLeaf)



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ClusteringLeaf::ClusteringLeaf():
    Component(Type(typeid(ClusteringLeaf)), Version(1, 0, 0)),
    dm_active(),
    dm_addresses(),
    dm_features(),
    dm_inactive(),
    dm_spaces(),
    dm_threads()
{
    // Performance Data Collector Interface
    
    declareInput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
        "AttachedToThreads",
        boost::bind(&ClusteringLeaf::handleAttachedToThreads, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "InitialLinkedObjects",
        boost::bind(&ClusteringLeaf::handleInitialLinkedObjects, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject> >(
        "LoadedLinkedObject",
        boost::bind(&ClusteringLeaf::handleLoadedLinkedObject, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged> >(
        "ThreadsStateChanged",
        boost::bind(&ClusteringLeaf::handleThreadsStateChanged, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject> >(
        "UnloadedLinkedObject",
        boost::bind(&ClusteringLeaf::handleUnloadedLinkedObject, this, _1)
        );

    // FeatureGenerator Interface

    declareInput<FeatureVector>(
        "Feature",
        boost::bind(&ClusteringLeaf::handleFeature, this, _1)
        );
    declareInput<ArgoNavis::Base::Address>(
        "ObservedAddress",
        boost::bind(&ClusteringLeaf::handleObservedAddress, this, _1)
        );

    declareOutput<AddressSpaces>("AddressSpaces");
    declareOutput<ThreadName>("EmitFeatures");
    declareOutput<ThreadName>("EmitPerformanceData");

    // ClusteringFilter Interface

    declareInput<boost::shared_ptr<ANCI_EmitPerformanceData> >(
        "EmitPerformanceData",
        boost::bind(&ClusteringLeaf::handleEmitPerformanceData, this, _1)
        );
    
    declareOutput<AddressBuffer>("AddressBuffer");
    declareOutput<boost::shared_ptr<ANCI_State> >("State");
    declareOutput<boost::shared_ptr<ANCI_ThreadTable> >("ThreadTable");

    // ClusteringManager Interface (not intercepted by ClusteringFilter)
    
    declareOutput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "LinkedObjectGroup"
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringLeaf::emitState()
{
    // ...
}



//------------------------------------------------------------------------------
// Update the set of active threads and the table of all observed threads.
//------------------------------------------------------------------------------
void ClusteringLeaf::handleAttachedToThreads(
    const boost::shared_ptr<CBTF_Protocol_AttachedToThreads>& message
    )
{
    for (u_int i = 0; i < message->threads.names.names_len; ++i)
    {
        ThreadName thread(message->threads.names.names_val[i]);
        dm_active.insert(thread);
        dm_threads.add(thread);
    }
}



//------------------------------------------------------------------------------
// Ask the FeatureGenerator to emit the performance data for the requested
// thread and then emit the LinkedObjectGroup for that thread. But only if
// the thread is in our ThreadTable, implying that our FeatureGenerator can
// actually supply the requested performance data.
//------------------------------------------------------------------------------
void ClusteringLeaf::handleEmitPerformanceData(
    const boost::shared_ptr<ANCI_EmitPerformanceData>& message
    )
{
    if (dm_threads.contains(message->thread))
    {
        ThreadName thread(dm_threads.name(message->thread));
    
        emitOutput<ThreadName>("EmitPerformanceData", thread);
        
        std::vector<CBTF_Protocol_LinkedObjectGroup> groups = dm_spaces;
        
        for (std::vector<CBTF_Protocol_LinkedObjectGroup>::const_iterator
                 i = groups.begin(); i != groups.end(); ++i)
        {
            if (ThreadName(i->thread) == thread)
            {
                emitOutput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
                    "LinkedObjectGroup",
                    boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>(
                        new CBTF_Protocol_LinkedObjectGroup(*i)
                        )
                    );
            }
        }
    }
}



//------------------------------------------------------------------------------
// Update the list of feature vectors for all observed threads.
//------------------------------------------------------------------------------
void ClusteringLeaf::handleFeature(const FeatureVector& feature)
{
    dm_features.push_back(feature);
}



//------------------------------------------------------------------------------
// Update the address spaces of all observed threads.
//------------------------------------------------------------------------------
void ClusteringLeaf::handleInitialLinkedObjects(
    const boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>& message
    )
{
    dm_spaces.apply(*message);
}



//------------------------------------------------------------------------------
// Update the address spaces of all observed threads.
//------------------------------------------------------------------------------
void ClusteringLeaf::handleLoadedLinkedObject(
    const boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject>& message
    )
{
    dm_spaces.apply(*message);
}



//------------------------------------------------------------------------------
// Update the address buffer containing all observed addresses.
//------------------------------------------------------------------------------
void ClusteringLeaf::handleObservedAddress(
    const ArgoNavis::Base::Address& address
    )
{
    dm_addresses.updateAddressCounts(address, 1);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringLeaf::handleThreadsStateChanged(
    const boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged>& message
    )
{
    // Are threads terminating?
    if (message->state == Terminated)
    {
        // Update the sets of active and inactive threads
        for (u_int i = 0; i < message->threads.names.names_len; ++i)
        {
            ThreadName thread(message->threads.names.names_val[i]);
            dm_active.erase(thread);
            dm_inactive.insert(thread);
        }
        
        // Are all threads now inactive?
        if (dm_active.empty())
        {
            // Emit the address buffer containing all observed addresses
            emitOutput<AddressBuffer>("AddressBuffer", dm_addresses);
            
            //
            // Provide the FeatureGenerator with the address spaces of all
            // observed threads. Then ask it to emit the features for each
            // of the inactive threads. Each feature vector is accumulated
            // into dm_features.
            //

            emitOutput<AddressSpaces>("AddressSpaces", dm_spaces);

            for (std::set<ThreadName>::const_iterator
                     i = dm_inactive.begin(); i != dm_inactive.end(); ++i)
            {
                emitOutput<ThreadName>("EmitFeatures", *i);
            }

            // Generate and emit the initial cluster analysis state
            emitState();

            // Emit the table of all observed threads
            emitOutput<ANCI_ThreadTable>("ThreadTable", dm_threads);
        }
    }
}



//------------------------------------------------------------------------------
// Update the address spaces of all observed threads.
//------------------------------------------------------------------------------
void ClusteringLeaf::handleUnloadedLinkedObject(
    const boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject>& message
    )
{
    dm_spaces.apply(*message);
}
