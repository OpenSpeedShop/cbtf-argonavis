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

/** @file Cluster analysis frontend component. */

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <typeinfo>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>

#include <KrellInstitute/Core/AddressBuffer.hpp>

#include <KrellInstitute/Messages/Blob.h>
#include <KrellInstitute/Messages/Clustering.h>
#include <KrellInstitute/Messages/LinkedObjectEvents.h>
#include <KrellInstitute/Messages/ThreadEvents.h>

#include "Messages.h"
#include "ThreadTable.hpp"

using namespace ArgoNavis::Clustering::Impl;
using namespace KrellInstitute::CBTF;
using namespace KrellInstitute::Core;



/**
 * ...
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

    /** Address buffer containing all observed addresses. */
    AddressBuffer dm_addresses;

    /** Table of all observed threads. */
    ThreadTable dm_threads;
            
}; // class ClusteringManager

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(ClusteringManager)



//------------------------------------------------------------------------------
// TEMPORARY NOTE TO SELF (WDH)
//
// The proper order to emit messages in order for Open|SpeedShop to properly
// process everything is:
//
// 1) One or more CBTF_Protocol_Blob containing the performance data.
// 2) Exactly one CBTF_Protocol_AttachedToThreads for all found threads.
// 3) Exactly one AddressBuffer for all threads.
// 4) One CBTF_Protocol_LinkedObjectGroup for each found thread.
// 5) Single emission of "true" on the "ThreadsFinished" output.
//
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ClusteringManager::ClusteringManager():
    Component(Type(typeid(ClusteringManager)), Version(1, 0, 0))
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
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringManager::handleLinkedObjectGroup(
    const boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>& message
    )
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringManager::handlePerformanceData(
    const boost::shared_ptr<CBTF_Protocol_Blob>& message
    )
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringManager::handleState(
    const boost::shared_ptr<ANCI_State>& message
    )
{
    // ...
}



//------------------------------------------------------------------------------
// Update the table of all observed threads.
//------------------------------------------------------------------------------
void ClusteringManager::handleThreadTable(
    const boost::shared_ptr<ANCI_ThreadTable>& message
    )
{
    dm_threads += *message;
}
