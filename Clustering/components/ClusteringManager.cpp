////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2017 Argo Navis Technologies. All Rights Reserved.
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

/** @file Cluster analysis frontend component. */

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <iterator>
#include <map>
#include <stddef.h>
#include <string>
#include <typeinfo>
#include <vector>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>
#include <KrellInstitute/CBTF/Impl/MRNet.hpp>

#include <KrellInstitute/Core/AddressBuffer.hpp>

#include <KrellInstitute/Messages/Blob.h>
#include <KrellInstitute/Messages/Clustering.h>
#include <KrellInstitute/Messages/LinkedObjectEvents.h>
#include <KrellInstitute/Messages/ThreadEvents.h>

#include <ArgoNavis/Base/AddressSpaces.hpp>
#include <ArgoNavis/Base/XDR.hpp>

#include "Algorithms.hpp"
#include "Messages.h"
#include "State.hpp"
#include "ThreadTable.hpp"
#include "ThreadUIDGroup.hpp"

using namespace ArgoNavis::Base;
using namespace ArgoNavis::Clustering::Impl;
using namespace KrellInstitute::CBTF;
using namespace KrellInstitute::CBTF::Impl;
using namespace KrellInstitute::Core;



/**
 * Cluster analysis component residing on the frontend node of the CBTF/MRNet
 * distributed component network. Provides the following functionality:
 *
 *     - Receives AddressBuffer and LinkedObjectGroup objects from the single
 *       ClusteringFilter residing on this node. The information is collected
 *       and provided to Open|SpeedShop at the appropriate time.
 *
 *     - Receives performance data from all of the ClusteringLeaf. The data is
 *       forwarded immediately to Open|SpeedShop.
 *
 *     - Receives ThreadTable objects from the single ClusteringFilter residing
 *       on this node. They are aggregated and provided to Open|SpeedShop at the
 *       appropriate time.
 *
 *     - Receives State objects from the single ClusteringFilter residing on
 *       this node and aggregates those State with identically named feature
 *       vectors. Once all State are received a fitness algorithm is applied
 *       to each to determine if they are "interesting". Performance data is
 *       requested for the representative thread of each cluster whose State
 *       was interesting.
 */
class __attribute__ ((visibility ("hidden"))) ClusteringManager :
    public Component
{
    
public:

    /** Factory function for this component type. */
    static Component::Instance factoryFunction()
    {
        return Component::Instance(
            reinterpret_cast<Component*>(new ClusteringManager())
            );
    }
    
private:

    /** Default constructor. */
    ClusteringManager();

    /** Handler for the "AddressBuffer" input. */
    void handleAddressBuffer(const AddressBuffer& buffer);

    /** Handler for the "BackendCount" input. */
    void handleBackendCount(const int& count);

    /** Handler for the "LinkedObjectGroup" input. */
    void handleLinkedObjectGroup(
        const boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>& message
        );

    /** Handler for the "PerformanceData" input. */
    void handlePerformanceData(
        const boost::shared_ptr<CBTF_Protocol_Blob>& message
        );

    /** Handler for the "State" input. */
    void handleState(const boost::shared_ptr<ANCI_State>& message);
    
    /** Handler for the "ThreadTable" input. */
    void handleThreadTable(const boost::shared_ptr<ANCI_ThreadTable>& message);

    /** Request performance data for the specified cluster analysis state. */
    void requestPerformanceData(const State& state);
    
    /** Address buffer containing all observed addresses. */
    AddressBuffer dm_addresses;

    /** Table of all "interesting" clustering criteria. */
    std::vector<boost::shared_ptr<Clustering_Criterion> > dm_criteria;

    /** Number of threads for which performance data has been received. */
    size_t dm_received;
    
    /** UIDs of all threads for which performance data has been requested. */
    ThreadUIDGroup dm_requested;

    /** Address spaces for all threads whose performance data was requested. */
    AddressSpaces dm_spaces;
    
    /** Table of all cluster analysis state. */
    std::map<std::string, State> dm_states;

    /** Number of child nodes finished sending their cluster analysis state. */
    size_t dm_states_finished;

    /** Table of all observed threads. */
    ThreadTable dm_threads;
            
}; // class ClusteringManager

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(ClusteringManager)



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ClusteringManager::ClusteringManager():
    Component(Type(typeid(ClusteringManager)), Version(1, 0, 0)),
    dm_addresses(),
    dm_criteria(),
    dm_received(0),
    dm_requested(),
    dm_spaces(),
    dm_states(),
    dm_states_finished(0),
    dm_threads()
{
    // ClusteringLeaf Interface (not intercepted by ClusteringFilter)

    declareInput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "LinkedObjectGroup",
        boost::bind(&ClusteringManager::handleLinkedObjectGroup, this, _1)
        );
    
    // FeatureGenerator Interface (not intercepted by ClusteringFilter)

    declareInput<boost::shared_ptr<CBTF_Protocol_Blob> >(
        "PerformanceData",
        boost::bind(&ClusteringManager::handlePerformanceData, this, _1)
        );

    // ClusteringFilter Interface

    declareInput<AddressBuffer>(
        "AddressBuffer",
        boost::bind(&ClusteringManager::handleAddressBuffer, this, _1)
        );
    declareInput<boost::shared_ptr<ANCI_State> >(
        "State", boost::bind(&ClusteringManager::handleState, this, _1)
        );
    declareInput<boost::shared_ptr<ANCI_ThreadTable> >(
        "ThreadTable",
        boost::bind(&ClusteringManager::handleThreadTable, this, _1)
        );
    
    declareOutput<boost::shared_ptr<ANCI_EmitPerformanceData> >(
        "EmitPerformanceData"
        );
    
    // Open|SpeedShop Interface

    declareInput<int>(
        "BackendCount",
        boost::bind(&ClusteringManager::handleBackendCount, this, _1)
        );

    declareOutput<AddressBuffer>("AddressBuffer");
    declareOutput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
        "AttachedToThreads"
        );
    declareOutput<boost::shared_ptr<Clustering_Criterion> >(
        "ClusteringCriterion"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "LinkedObjectGroup"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_Blob> >("PerformanceData");
    declareOutput<bool>("ThreadsFinished");
}



//------------------------------------------------------------------------------
// Update the address buffer containing all observed addresses.
//------------------------------------------------------------------------------
void ClusteringManager::handleAddressBuffer(const AddressBuffer& buffer)
{
    dm_addresses.updateAddressCounts(const_cast<AddressBuffer&>(buffer));
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringManager::handleBackendCount(const int& count)
{
    // TODO: Does something need to be done with this?
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringManager::handleLinkedObjectGroup(
    const boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>& message
    )
{
    // Update the address spaces of threads whose performance data was requested
    dm_spaces.apply(*message);

    //
    // Increment the number of threads for which performance data has been
    // received, as the receipt of this message is also overloaded to indicate
    // that. Then check if all expected performance has been received. If so,
    // emit a flurry of messages in the proper order.
    //

    if (++dm_received == dm_requested.size())
    {
        //
        // All of the CBTF_Protocol_Blob message(s) containing the performance
        // data have already been received and forwarded to Open|SpeedShop. Now
        // send a single CBTF_Protocol_AttachedToThreads message for ALL of the
        // observed threads. Not just ones for which we requested and forwarded
        // performance data.
        //

        emitOutput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
            "AttachedToThreads",
            boost::shared_ptr<CBTF_Protocol_AttachedToThreads>(
                new CBTF_Protocol_AttachedToThreads(dm_threads)
                )
            );

        // Emit a single AddressBuffer for ALL observed addresses
        emitOutput<AddressBuffer>("AddressBuffer", dm_addresses);
        
        //
        // Next emit a single CBTF_Protocol_LinkedObjectGroup message for each
        // of the threads for which performance data was requested and received.
        //
        
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
        
        //
        // Emit a Clustering_Criterion message for each of the "interesting"
        // clustering criteria. Simple because they have already been queued
        // queued up by requestPerformanceData().
        //

        for (std::vector<
                 boost::shared_ptr<Clustering_Criterion>
                 >::const_iterator
                 i = dm_criteria.begin(); i != dm_criteria.end(); ++i)
        {
            emitOutput<boost::shared_ptr<Clustering_Criterion> >(
                "ClusteringCriterion", *i
                );
        }

        // And, FINALLY, emit a single ThreadsFinished message
        emitOutput<bool>("ThreadsFinished", true);
    }
}



//------------------------------------------------------------------------------
// Simply reemit the message. The CBTF_Protocol_Blob message(s) containing the
// performance data are the first thing that Open|SpeedShop expects to receive.
// So there is no need to do anything other than pass them along.
//------------------------------------------------------------------------------
void ClusteringManager::handlePerformanceData(
    const boost::shared_ptr<CBTF_Protocol_Blob>& message
    )
{
    emitOutput<boost::shared_ptr<CBTF_Protocol_Blob> >(
        "OutgoingPerformanceData", message
        );
}



//------------------------------------------------------------------------------
// Update the table of all cluster analysis state.    
//------------------------------------------------------------------------------
void ClusteringManager::handleState(
    const boost::shared_ptr<ANCI_State>& message
    )
{
    State state(*message);

    std::map<std::string, State>::iterator i = dm_states.find(state.name());
    
    if (i == dm_states.end())
    {
        dm_states.insert(std::make_pair(state.name(), state));
    }
    else
    {
        i->second.add(state);
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringManager::handleThreadTable(
    const boost::shared_ptr<ANCI_ThreadTable>& message
    )
{
    // Update the table of all observed threads
    dm_threads.add(ThreadTable(*message));

    //
    // Increment the number of child nodes finished sending their cluster
    // analysis state, as the receipt of this message is also overloaded
    // to indicate that. Then check if all child nodes are finished.
    //

    if (++dm_states_finished == TheTopologyInfo.NumChildren)
    {
        //
        // Apply the default fitness algorithm to each cluster analysis
        // state. Request performance data for all such state which the
        // default fitness algorithm deems "interesting".
        //
        
        for (std::map<std::string, State>::iterator
                 i = dm_states.begin(); i != dm_states.end(); ++i)
        {
            if (defaultFitnessAlgorithm(i->second))
            {
                requestPerformanceData(i->second);
            }
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringManager::requestPerformanceData(const State& state)
{
    //
    // Allocate a Clustering_Criterion message, and an appropriately
    // sized array of Cluster within that message, for the given state.
    //
    
    boost::shared_ptr<Clustering_Criterion> criterion(
        new Clustering_Criterion()
        );

    criterion->name = strdup(state.name().c_str());

    criterion->clusters.clusters_len = state.sizes().size();
    
    criterion->clusters.clusters_val = allocateXDRCountedArray<Cluster>(
        criterion->clusters.clusters_len
        );
    
    // Iterate over each cluster in the given state
    for (size_t i = 0; i < state.sizes().size(); ++i)
    {
        Cluster& cluster = criterion->clusters.clusters_val[i];
        
        //
        // One key motivation for performing cluster analysis is to reduce the
        // amount of performance data being sent to Open|SpeedShop. Thus if we
        // already requested performance data for one or more of the cluster's
        // threads, it is preferable to use one of them as the representative
        // thread for this cluster. Get all of the threads in this cluster and
        // intersect them with the threads for which performance data has been
        // requested already.
        //
        // Currently there is no further reasonable criteria by which a thread
        // can be selected as the representative thread for the cluster. If we
        // kept the single-thread centroids in the cluster analysis state, the
        // thread's distance from the cluster's centroid would be a reasonable
        // criteria for further down-selection. But sending that much data is
        // time and memory prohibitive at very large scale. For now the first
        // thread is selected arbitrarily.
        //

        ThreadUIDGroup all = state.threads(i), requested;
        
        std::set_intersection(all.begin(), all.end(),
                              dm_requested.begin(), dm_requested.end(),
                              std::inserter(requested, requested.begin()));

        ThreadUID representative =
            requested.empty() ? *all.begin() : *requested.begin();

        //
        // Emit a request for the representative thread's performance data if
        // it hasn't already been requested. Update the table of all threads
        // for which performance data has been requested.
        //
        
        if (dm_requested.find(representative) == dm_requested.end())
        {
            boost::shared_ptr<ANCI_EmitPerformanceData> emit(
                new ANCI_EmitPerformanceData()
                );
            
            emit->thread = representative;
            
            emitOutput<boost::shared_ptr<ANCI_EmitPerformanceData> >(
                "EmitPerformanceData", emit
                );
            
            dm_requested.insert(representative);
        }

        // Fill in the Cluster within the Clustering_Criterion message

        cluster.representative = dm_threads.name(representative);
        
        cluster.threads.threads_len = all.size();
        
        cluster.threads.threads_val =
            allocateXDRCountedArray<CBTF_Protocol_ThreadName>(
                cluster.threads.threads_len
                );
        
        size_t n = 0;
        for (ThreadUIDGroup::const_iterator
                 j = all.begin(); j != all.end(); ++j, ++n)
        {
            cluster.threads.threads_val[n] = dm_threads.name(*j);
        }
    }

    //
    // The CBTF_Protocol_AttachedToThreads message must be sent first before
    // the Clustering_Criterion message(s) are sent. And the former can't be
    // sent until all the CBTF_Protocol_Blob containing the performance data
    // have been sent. For now just queue up the Clustering_Criterion in the
    // table of "interesting" clustering criteria.
    //

    dm_criteria.push_back(criterion);
}
