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
    
}; // class ClusteringManager

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(ClusteringManager)



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
    
    declareOutput<boost::shared_ptr<Clustering_EmitPerformanceData> >(
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
//------------------------------------------------------------------------------
void ClusteringManager::handleAddressBuffer(const AddressBuffer& buffer)
{
    // ...
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
