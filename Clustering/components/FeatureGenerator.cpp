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

/** @file Definition of the FeatureGenerator class. */

#include <boost/bind.hpp>

#include <ArgoNavis/Clustering/FeatureGenerator.hpp>

using namespace ArgoNavis::Base;
using namespace ArgoNavis::Clustering;
using namespace KrellInstitute::CBTF;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
FeatureGenerator::FeatureGenerator(const Type& type, const Version& version):
    Component(type, version),
    dm_spaces()
{
    // Performance Data Collector Interface
    
    declareInput<boost::shared_ptr<CBTF_Protocol_Blob> >(
        "PerformanceData",
        boost::bind(&FeatureGenerator::handlePerformanceData, this, _1)
        );

    // ClusteringLeaf Interface

    declareInput<AddressSpaces>(
        "AddressSpaces",
        boost::bind(&FeatureGenerator::handleAddressSpaces, this, _1)
        );
    declareInput<ThreadName>(
        "EmitFeatures",
        boost::bind(&FeatureGenerator::handleEmitFeatures, this, _1)
        );
    declareInput<ThreadName>(
        "EmitPerformanceData",
        boost::bind(&FeatureGenerator::handleEmitPerformanceData, this, _1)
        );
    
    declareOutput<FeatureVector>("Feature");
    declareOutput<Address>("ObservedAddress");

    // ClusteringManager Interface (not intercepted by ClusteringFilter)
    
    declareOutput<boost::shared_ptr<CBTF_Protocol_Blob> >("PerformanceData");
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::emitFeature(const FeatureVector& feature)
{
    emitOutput<FeatureVector>("Feature", feature);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::emitObservedAddress(const Address& address)
{
    emitOutput<Address>("ObservedAddress", address);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::emitPerformanceData(
    const boost::shared_ptr<CBTF_Protocol_Blob>& blob
    )
{
    emitOutput<boost::shared_ptr<CBTF_Protocol_Blob> >("PerformanceData", blob);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const AddressSpaces& FeatureGenerator::spaces() const
{
    return dm_spaces;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::handleAddressSpaces(const AddressSpaces& spaces)
{
    dm_spaces = spaces;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::handleEmitFeatures(const ThreadName& thread)
{
    onEmitFeatures(thread);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::handleEmitPerformanceData(const ThreadName& thread)
{
    onEmitPerformanceData(thread);
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::handlePerformanceData(
    const boost::shared_ptr<CBTF_Protocol_Blob>& message
    )
{
    onPerformanceData(message);
}
