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
#include <set>
#include <typeinfo>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>

#include <KrellInstitute/Core/AddressBuffer.hpp>

#include <KrellInstitute/Messages/Clustering.h>
#include <KrellInstitute/Messages/LinkedObjectEvents.h>
#include <KrellInstitute/Messages/ThreadEvents.h>

#include <ArgoNavis/Base/AddressSpaces.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>

#include <ArgoNavis/Clustering/FeatureVector.hpp>

using namespace ArgoNavis::Base;
using namespace ArgoNavis::Clustering;
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

    /** Handler for the "AttachedToThreads" input. */
    void handleAttachedToThreads(
        const boost::shared_ptr<CBTF_Protocol_AttachedToThreads>& message
        );

    /** Handler for the "EmitPerformanceData" input. */
    void handleEmitPerformanceData(
        const boost::shared_ptr<Clustering_EmitPerformanceData>& message
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

    /** Address buffer containing all of the observed addresses. */
    AddressBuffer dm_addresses;

    /** Address spaces of all observed threads. */
    AddressSpaces dm_spaces;

    /** Names of all active (non-terminated) threads. */
    std::set<ThreadName> dm_threads;

}; // class ClusteringLeaf

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(ClusteringLeaf)



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ClusteringLeaf::ClusteringLeaf():
    Component(Type(typeid(ClusteringLeaf)), Version(1, 0, 0)),
    dm_addresses(),
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

    declareInput<boost::shared_ptr<Clustering_EmitPerformanceData> >(
        "EmitPerformanceData",
        boost::bind(&ClusteringLeaf::handleEmitPerformanceData, this, _1)
        );
    
    declareOutput<AddressBuffer>("AddressBuffer");

    // ClusteringManager Interface (not intercepted by ClusteringFilter)
    
    declareOutput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "LinkedObjectGroup"
        );
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
//------------------------------------------------------------------------------
void ClusteringLeaf::handleEmitPerformanceData(
    const boost::shared_ptr<Clustering_EmitPerformanceData>& message
    )
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringLeaf::handleFeature(const FeatureVector& feature)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringLeaf::handleInitialLinkedObjects(
    const boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>& message
    )
{
    dm_spaces.apply(*message);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringLeaf::handleLoadedLinkedObject(
    const boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject>& message
    )
{
    dm_spaces.apply(*message);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringLeaf::handleObservedAddress(
    const ArgoNavis::Base::Address& address
    )
{
    dm_addresses.updateAddressCounts(address, 1);
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
#if defined(HIDE_FOR_NOW)
            emitOutput<bool>("TriggerData", true);
            
            CBTF_Protocol_AttachedToThreads threads = dm_spaces;

            emitOutput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
                "AttachedToThreads",
                boost::shared_ptr<CBTF_Protocol_AttachedToThreads>(
                    new CBTF_Protocol_AttachedToThreads(threads)
                    )
                );

            emitOutput<bool>("TriggerAddressBuffer", true);

            std::vector<CBTF_Protocol_LinkedObjectGroup> groups = dm_spaces;
            
            for (std::vector<CBTF_Protocol_LinkedObjectGroup>::const_iterator
                     i = groups.begin(); i != groups.end(); ++i)
            {
                emitOutput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
                    "LinkedObjectGroup",
                    boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>(
                        new CBTF_Protocol_LinkedObjectGroup(*i)
                        )
                    );
            }
            
            emitOutput<bool>("ThreadsFinished", true);
#endif
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringLeaf::handleUnloadedLinkedObject(
    const boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject>& message
    )
{
    dm_spaces.apply(*message);
}
