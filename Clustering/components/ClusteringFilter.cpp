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

/** @file Cluster analysis filter component. */

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <stddef.h>
#include <string>
#include <typeinfo>
#include <utility>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>
#include <KrellInstitute/CBTF/Impl/MRNet.hpp>

#include <KrellInstitute/Core/AddressBuffer.hpp>

#include "Messages.h"
#include "State.hpp"
#include "ThreadTable.hpp"

using namespace ArgoNavis::Clustering::Impl;
using namespace KrellInstitute::CBTF;
using namespace KrellInstitute::CBTF::Impl;
using namespace KrellInstitute::Core;



/**
 * Cluster analysis component residing on the non-leaf nodes of the CBTF/MRNet
 * distributed component network. Provides the following functionality:
 *
 *     - Receives ThreadTable objects from the ClusteringLeaf/ClusteringFilter
 *       residing on the nodes below this one, and the aggregate is provided
 *       to the ClusteringFilter/ClusteringManager residing on the node above
 *       this one.
 *
 *     - Receives State objects from the ClusteringLeaf/ClusteringFilter
 *       residing on the nodes below this one, aggregating those State with
 *       identically named feature vectors. Once all State are received, a
 *       cluster analysis algorithm is applied to each and the new State are
 *       sent to the ClusteringFilter/ClusteringManager residing on the
 *       node above this one.
 *
 *     - Receives requests to emit performance data for individual threads
 *       from the ClusteringFilter/ClusteringManager residing on the node
 *       above this one. Forwards them to the ClusteringLeaf/ClusteringFilter
 *       residing on the node below this one. But ONLY when the requested
 *       thread is actually found on one of the nodes below this one.
 */
class __attribute__ ((visibility ("hidden"))) ClusteringFilter :
    public Component
{
    
public:

    /** Factory function for this component type. */
    static Component::Instance factoryFunction()
    {
        return Component::Instance(
            reinterpret_cast<Component*>(new ClusteringFilter())
            );
    }
    
private:

    /** Default constructor. */
    ClusteringFilter();

    /** Handler for the "AddressBuffer" input. */
    void handleAddressBuffer(const AddressBuffer& buffer);

    /** Handler for the "EmitPerformanceData" input. */
    void handleEmitPerformanceData(
        const boost::shared_ptr<ANCI_EmitPerformanceData>& message
        );

    /** Handler for the "State" input. */
    void handleState(const boost::shared_ptr<ANCI_State>& message);
    
    /** Handler for the "ThreadTable" input. */
    void handleThreadTable(const boost::shared_ptr<ANCI_ThreadTable>& message);

    /** Address buffer containing all observed addresses. */
    AddressBuffer dm_addresses;

    /** Table of all cluster analysis state. */
    std::map<std::string, State> dm_states;

    /** Number of child nodes finished sending their cluster analysis state. */
    size_t dm_states_finished;
    
    /** Table of all observed threads. */
    ThreadTable dm_threads;

}; // class ClusteringFilter

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(ClusteringFilter)



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ClusteringFilter::ClusteringFilter():
    Component(Type(typeid(ClusteringFilter)), Version(1, 0, 0)),
    dm_addresses(),
    dm_states(),
    dm_states_finished(0),
    dm_threads()
{
    // ClusteringLeaf/ClusteringFilter Interface

    declareInput<AddressBuffer>(
        "AddressBuffer",
        boost::bind(&ClusteringFilter::handleAddressBuffer, this, _1)
        );
    declareInput<boost::shared_ptr<ANCI_State> >(
        "State", boost::bind(&ClusteringFilter::handleState, this, _1)
        );
    declareInput<boost::shared_ptr<ANCI_ThreadTable> >(
        "ThreadTable",
        boost::bind(&ClusteringFilter::handleThreadTable, this, _1)
        );
    
    declareOutput<boost::shared_ptr<ANCI_EmitPerformanceData> >(
        "EmitPerformanceData"
        );
    
    // ClusteringManager/ClusteringFilter Interface
    
    declareInput<boost::shared_ptr<ANCI_EmitPerformanceData> >(
        "EmitPerformanceData",
        boost::bind(&ClusteringFilter::handleEmitPerformanceData, this, _1)
        );

    declareOutput<AddressBuffer>("AddressBuffer");
    declareOutput<boost::shared_ptr<ANCI_State> >("State");
    declareOutput<boost::shared_ptr<ANCI_ThreadTable> >("ThreadTable");
}



//------------------------------------------------------------------------------
// Update the address buffer containing all observed addresses.
//------------------------------------------------------------------------------
void ClusteringFilter::handleAddressBuffer(const AddressBuffer& buffer)
{
    dm_addresses.updateAddressCounts(const_cast<AddressBuffer&>(buffer));
}



//------------------------------------------------------------------------------
// Reemit the message, but only if the thread is in our thread table, implying
// that one of our children can actually supply the requested performance data.
//------------------------------------------------------------------------------
void ClusteringFilter::handleEmitPerformanceData(
    const boost::shared_ptr<ANCI_EmitPerformanceData>& message
    )
{
    if (dm_threads.contains(message->thread))
    {
        emitOutput<boost::shared_ptr<ANCI_EmitPerformanceData> >(
            "EmitPerformanceData", message
            );
    }
}



//------------------------------------------------------------------------------
// Update the table of all cluster analysis state.
//------------------------------------------------------------------------------
void ClusteringFilter::handleState(
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
void ClusteringFilter::handleThreadTable(
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
        // Emit the address buffer containing all observed addresses
        emitOutput<AddressBuffer>("AddressBuffer", dm_addresses);
        

        
        // TODO: Apply a cluster analysis algorithm to each State

        

        // Emit the table of all (new) cluster analysis state
        for (std::map<std::string, State>::const_iterator
                 i = dm_states.begin(); i != dm_states.end(); ++i)
        {
            emitOutput<boost::shared_ptr<ANCI_State> >(
                "State",
                boost::shared_ptr<ANCI_State>(new ANCI_State(i->second))
                );
        }

        //
        // Emit the table of all observed threads.
        //
        // It is important that this message not be sent until AFTER all
        // of the cluster analysis state is emitted. ClusteringFilter and
        // ClusteringManager both use the receipt of this message to also
        // indicate that all cluster analysis state for a given node has
        // been sent.
        //

        emitOutput<boost::shared_ptr<ANCI_ThreadTable> >(
            "ThreadTable",
            boost::shared_ptr<ANCI_ThreadTable>(
                new ANCI_ThreadTable(dm_threads)
                )
            );
    }
}
