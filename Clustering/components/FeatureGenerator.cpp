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
#include <vector>

#include <ArgoNavis/Clustering/FeatureGenerator.hpp>

using namespace ArgoNavis::Base;
using namespace ArgoNavis::Clustering;
using namespace KrellInstitute::CBTF;
using namespace KrellInstitute::Core;



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
FeatureGenerator::FeatureGenerator(const Type& type, const Version& version):
    Component(type, version),
    dm_addresses(),
    dm_address_spaces()
{
    declareInput<boost::shared_ptr<CBTF_Protocol_Blob> >(
        "Data",
        boost::bind(&FeatureGenerator::handleData, this, _1)
        );
    declareInput<bool>(
        "EmitAddressBuffer",
        boost::bind(&FeatureGenerator::handleEmitAddressBuffer, this, _1)
        );
    declareInput<ThreadName>(
        "EmitData",
        boost::bind(&FeatureGenerator::handleEmitData, this, _1)
        );
    declareInput<ThreadName>(
        "EmitFeatures",
        boost::bind(&FeatureGenerator::handleEmitFeatures, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "InitialLinkedObjects",
        boost::bind(&FeatureGenerator::handleInitialLinkedObjects, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject> >(
        "LoadedLinkedObject",
        boost::bind(&FeatureGenerator::handleLoadedLinkedObject, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject> >(
        "UnloadedLinkedObject",
        boost::bind(&FeatureGenerator::handleUnloadedLinkedObject, this, _1)
        );

    declareOutput<AddressBuffer>("AddressBuffer");
    declareOutput<boost::shared_ptr<CBTF_Protocol_Blob> >("Data");
    //declareOutput<...>("Feature");
    declareOutput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "LinkedObjectGroup"
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::emit(const boost::shared_ptr<CBTF_Protocol_Blob>& blob)
{
    emitOutput<boost::shared_ptr<CBTF_Protocol_Blob> >("Data", blob);
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::emit(/*  ... feature */)
{
    // emitOutput<...>("Feature", feature);
}


        
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::observed(const Base::Address& address)
{
    dm_addresses.updateAddressCounts(address, 1);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
const AddressSpaces& FeatureGenerator::spaces() const
{
    return dm_address_spaces;
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::handleData(
    const boost::shared_ptr<CBTF_Protocol_Blob>& message
    )
{
    onReceivedData(message);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::handleEmitAddressBuffer(const bool& value)
{
    emitOutput<AddressBuffer>("AddressBuffer", dm_addresses);    
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::handleEmitData(const ThreadName& thread)
{
    onEmitData(thread);
    
    std::vector<CBTF_Protocol_LinkedObjectGroup> groups = dm_address_spaces;
    
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
            break;
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::handleEmitFeatures(const ThreadName& thread)
{
    onEmitFeatures(thread);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::handleInitialLinkedObjects(
    const boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>& message
    )
{
    dm_address_spaces.apply(*message);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::handleLoadedLinkedObject(
    const boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject>& message
    )
{
    dm_address_spaces.apply(*message);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void FeatureGenerator::handleUnloadedLinkedObject(
    const boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject>& message
    )
{
    dm_address_spaces.apply(*message);
}
