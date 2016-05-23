////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012-2016 Argo Navis Technologies. All Rights Reserved.
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

/** @file Component performing aggregation for CUDA data. */

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <cstdlib>
#include <typeinfo>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>

#include <KrellInstitute/Core/AddressBuffer.hpp>

#include <KrellInstitute/Messages/Address.h>
#include <KrellInstitute/Messages/Blob.h>
#include <KrellInstitute/Messages/CUDA_data.h>
#include <KrellInstitute/Messages/DataHeader.h>
#include <KrellInstitute/Messages/File.h>
#include <KrellInstitute/Messages/PerformanceData.hpp>
#include <KrellInstitute/Messages/Time.h>

#include <ArgoNavis/CUDA/PerformanceData.hpp>
#include <ArgoNavis/CUDA/stringify.hpp>

using namespace ArgoNavis;
using namespace KrellInstitute::CBTF;
using namespace KrellInstitute::Core;



/**
 * Data aggregator for the performance data blobs generated by the CUDA
 * collector. This is used to perform 2 critical functions for Open|SpeedShop:
 *
 *     -# Inform it what addresses need to be resolved to symbols in order
 *        to actually view the performance data.
 *
 *     -# Rearrange the individual enqueue and completion records for each
 *        CUDA event such that they appear in a single performance data blob
 *        associated with the thread that enqueued the request. This is now
 *        required because newer versions of CUPTI report completions within
 *        a separate CUDA implementation thread. Open|SpeedShop isn't setup
 *        to handle such data itself, and modifying it to do so would have
 *        been a much bigger change than simply reshuffling the data here.
 *
 * @note    This component is <em>not</em> scalable to large thread counts.
 *          It is currently being used as a temporary measure until the CUDA
 *          collector can be adapted to use the scalable components found in
 *          the cbtf-krell repository.
 */
class __attribute__ ((visibility ("hidden"))) DataAggregatorForCUDA :
    public Component
{
    
public:

    /** Factory function for this component type. */
    static Component::Instance factoryFunction()
    {
        return Component::Instance(
            reinterpret_cast<Component*>(new DataAggregatorForCUDA())
            );
    }
    
private:

    /** Default constructor. */
    DataAggregatorForCUDA();
    
    /** Add an address to the address buffer of all observed addresses. */
    bool addAddress(const Base::Address& address);

    /** Emit the specified performance data blob. */
    bool emitBlob(const boost::shared_ptr<CBTF_Protocol_Blob>& blob);
    
    /** Handler for the "Data" input. */
    void handleData(const boost::shared_ptr<CBTF_Protocol_Blob>& blob);
    
    /** Handler for the "TriggerAddressBuffer" input. */
    void handleTriggerAddressBuffer(const bool& value);

    /** Handler for the "TriggerData" input. */
    void handleTriggerData(const bool& value);
    
    /** Visit the performance data blobs for the given thread. */
    bool visitBlobs(const Base::ThreadName& thread);

    /** Flag indicating if debugging is enabled for this component. */
    bool dm_is_debug_enabled;
     
    /** Address buffer containing all of the observed addresses. */
    AddressBuffer dm_addresses;
    
    /** CUDA performance data for all of the observed threads. */
    CUDA::PerformanceData dm_data;

}; // class DataAggregatorForCUDA

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(DataAggregatorForCUDA)



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
DataAggregatorForCUDA::DataAggregatorForCUDA() :
    Component(Type(typeid(DataAggregatorForCUDA)), Version(1, 0, 0)),
    dm_is_debug_enabled(getenv("CBTF_DEBUG_DATA_AGGREGATOR_FOR_CUDA") != NULL),
    dm_addresses(),
    dm_data()
{
    declareInput<boost::shared_ptr<CBTF_Protocol_Blob> >(
        "Data",
        boost::bind(&DataAggregatorForCUDA::handleData, this, _1)
        );
    declareInput<bool>(
        "TriggerAddressBuffer",
        boost::bind(
            &DataAggregatorForCUDA::handleTriggerAddressBuffer, this, _1
            )
        );
    declareInput<bool>(
        "TriggerData",
        boost::bind(&DataAggregatorForCUDA::handleTriggerData, this, _1)
        );
    
    declareOutput<AddressBuffer>("AddressBuffer");
    declareOutput<boost::shared_ptr<CBTF_Protocol_Blob> >("Data");
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool DataAggregatorForCUDA::addAddress(const Base::Address& address)
{
    dm_addresses.updateAddressCounts(address, 1);

    return true; // Continue the iteration
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool DataAggregatorForCUDA::emitBlob(
    const boost::shared_ptr<CBTF_Protocol_Blob>& blob
    )
{
    emitOutput<boost::shared_ptr<CBTF_Protocol_Blob> >("Data", blob);

    if (dm_is_debug_enabled)
    {
        std::pair<
            boost::shared_ptr<CBTF_DataHeader>,
            boost::shared_ptr<CBTF_cuda_data>
            > unpacked = KrellInstitute::Messages::unpack<CBTF_cuda_data>(
                blob, reinterpret_cast<xdrproc_t>(xdr_CBTF_cuda_data)
                );
        
        const CBTF_DataHeader& cuda_data_header = *unpacked.first;
        const CBTF_cuda_data& cuda_data = *unpacked.second;

        std::cout << std::endl
                  << "[CBTF/CUDA] Emitted Blob" << std::endl
		  << std::endl
                  << CUDA::stringify<>(cuda_data_header)
                  << CUDA::stringify<>(cuda_data);
    }
    
    return true; // Continue the iteration
}



//------------------------------------------------------------------------------
// Visit each of the PC addresses in this performance data blob, adding them to
// the address buffer containing all of the observed addresses. Also accumulate
// this performance data blob into our ArgoNavis::CUDA::PerformanceData object.
//
// TODO: Do we need to grab the experiment, collector, and id fields of the data
//       blob, save them, and reapply them when remitting the data blobs later?
//------------------------------------------------------------------------------
void DataAggregatorForCUDA::handleData(
    const boost::shared_ptr<CBTF_Protocol_Blob>& blob
    )
{
    std::pair<
        boost::shared_ptr<CBTF_DataHeader>, boost::shared_ptr<CBTF_cuda_data>
        > unpacked = KrellInstitute::Messages::unpack<CBTF_cuda_data>(
            blob, reinterpret_cast<xdrproc_t>(xdr_CBTF_cuda_data)
            );
    
    const CBTF_DataHeader& cuda_data_header = *unpacked.first;
    const CBTF_cuda_data& cuda_data = *unpacked.second;

    if (dm_is_debug_enabled)
    {
        std::cout << std::endl
                  << "[CBTF/CUDA] Received Blob" << std::endl
		  << std::endl
                  << CUDA::stringify<>(cuda_data_header)
                  << CUDA::stringify<>(cuda_data);
    }

    CUDA::PerformanceData::visitPCs(
        cuda_data, boost::bind(&DataAggregatorForCUDA::addAddress, this, _1)
        );
    
    dm_data.apply(cuda_data_header, cuda_data);
}



//------------------------------------------------------------------------------
// Emit the address buffer containing all of the observed addresses.
//------------------------------------------------------------------------------
void DataAggregatorForCUDA::handleTriggerAddressBuffer(const bool& value)
{
    emitOutput<AddressBuffer>("AddressBuffer", dm_addresses);
}



//------------------------------------------------------------------------------
// Visit all of the threads containing performance data, visiting and emitting
// all of the performance data blobs for each thread.
//------------------------------------------------------------------------------
void DataAggregatorForCUDA::handleTriggerData(const bool& value)
{
    dm_data.visitThreads(
        boost::bind(&DataAggregatorForCUDA::visitBlobs, this, _1)
        );
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool DataAggregatorForCUDA::visitBlobs(const Base::ThreadName& thread)
{
    dm_data.visitBlobs(
        thread, boost::bind(&DataAggregatorForCUDA::emitBlob, this, _1)
        );
    
    return true; // Continue the iteration
}