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
#include <typeinfo>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>

#include <KrellInstitute/Core/AddressBuffer.hpp>

#include <KrellInstitute/Messages/Clustering.h>

using namespace KrellInstitute::CBTF;
using namespace KrellInstitute::Core;



/**
 * ...
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
        const boost::shared_ptr<Clustering_EmitPerformanceData>& message
        );
    
}; // class ClusteringFilter

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(ClusteringFilter)



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
ClusteringFilter::ClusteringFilter():
    Component(Type(typeid(ClusteringFilter)), Version(1, 0, 0))
{
    // ClusteringLeaf/ClusteringFilter Interface

    declareInput<AddressBuffer>(
        "AddressBuffer",
        boost::bind(&ClusteringFilter::handleAddressBuffer, this, _1)
        );
    
    declareOutput<boost::shared_ptr<Clustering_EmitPerformanceData> >(
        "EmitPerformanceData"
        );
    
    // ClusteringManager/ClusteringFilter Interface
    
    declareInput<boost::shared_ptr<Clustering_EmitPerformanceData> >(
        "EmitPerformanceData",
        boost::bind(&ClusteringFilter::handleEmitPerformanceData, this, _1)
        );

    declareOutput<AddressBuffer>("AddressBuffer");
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringFilter::handleAddressBuffer(const AddressBuffer& buffer)
{
    // ...
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void ClusteringFilter::handleEmitPerformanceData(
    const boost::shared_ptr<Clustering_EmitPerformanceData>& message
    )
{
    // ...
}
