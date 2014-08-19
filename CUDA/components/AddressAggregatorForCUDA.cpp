////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012-2014 Argo Navis Technologies. All Rights Reserved.
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

/** @file Component performing address aggregation for CUDA data. */

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <typeinfo>
#include <utility>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>

#include <KrellInstitute/Core/AddressBuffer.hpp>

#include <KrellInstitute/Messages/Address.h>
#include <KrellInstitute/Messages/Blob.h>
#include <KrellInstitute/Messages/File.h>
#include <KrellInstitute/Messages/PerformanceData.hpp>
#include <KrellInstitute/Messages/Time.h>

#include "CUDA_data.h"

using namespace KrellInstitute::CBTF;
using namespace KrellInstitute::Core;



/**
 * Address aggregator for the performance data blobs generated by the CUDA
 * collector. This is used to inform Open|SpeedShop what addresses need to
 * be resolved to symbols in order to view the performance data.
 */
class __attribute__ ((visibility ("hidden"))) AddressAggregatorForCUDA :
    public Component
{
    
public:

    /** Factory function for this component type. */
    static Component::Instance factoryFunction()
    {
        return Component::Instance(
            reinterpret_cast<Component*>(new AddressAggregatorForCUDA())
            );
    }
    
private:

    /** Default constructor. */
    AddressAggregatorForCUDA();
    
    /** Handler for the "Data" input. */
    void handleData(const boost::shared_ptr<CBTF_Protocol_Blob>& message);
    
    /** Handler for the "ThreadsFinished" input. */
    void handleThreadsFinished(const bool& value);

    /** Address buffer containing all of the observed addresses. */
    AddressBuffer dm_addresses;
    
}; // class AddressAggregatorForCUDA

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(AddressAggregatorForCUDA)



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
AddressAggregatorForCUDA::AddressAggregatorForCUDA() :
    Component(Type(typeid(AddressAggregatorForCUDA)), Version(0, 0, 0)),
    dm_addresses()
{
    declareInput<boost::shared_ptr<CBTF_Protocol_Blob> >(
        "Data",
        boost::bind(&AddressAggregatorForCUDA::handleData, this, _1)
        );
    declareInput<bool>(
        "ThreadsFinished",
        boost::bind(&AddressAggregatorForCUDA::handleThreadsFinished, this, _1)
        );
    
    declareOutput<AddressBuffer>("AddressBuffer");
    declareOutput<boost::shared_ptr<CBTF_Protocol_Blob> >("Data");
    declareOutput<bool>("ThreadsFinished");
}



//------------------------------------------------------------------------------
// Re-emit the original message unchanged. Iterate over each of the individual
// CUDA messages that are "packed" into this performance data blob. For all
// of the messages containing stack traces or sampled PC addresses, add those
// addresses to the address buffer containing all of the observed addresses.
//------------------------------------------------------------------------------
void AddressAggregatorForCUDA::handleData(
    const boost::shared_ptr<CBTF_Protocol_Blob>& message
    )
{
    emitOutput<boost::shared_ptr<CBTF_Protocol_Blob> >("Data", message);

    std::pair<
        boost::shared_ptr<CBTF_DataHeader>, boost::shared_ptr<CBTF_cuda_data>
        > unpacked_message = KrellInstitute::Messages::unpack<CBTF_cuda_data>(
            message, reinterpret_cast<xdrproc_t>(xdr_CBTF_cuda_data)
            );

    const CBTF_DataHeader& cuda_data_header = *unpacked_message.first;
    const CBTF_cuda_data& cuda_data = *unpacked_message.second;

    for (u_int i = 0; i < cuda_data.messages.messages_len; ++i)
    {
        const CBTF_cuda_message& cuda_message =
            cuda_data.messages.messages_val[i];
        
        switch (cuda_message.type)
        {
            
        case EnqueueRequest:
            {
                const CUDA_EnqueueRequest& msg = 
                    cuda_message.CBTF_cuda_message_u.enqueue_request;

                for (uint32_t i = msg.call_site;
                     (i < cuda_data.stack_traces.stack_traces_len) &&
                         (cuda_data.stack_traces.stack_traces_val[i] != 0);
                     ++i)
                {
                    dm_addresses.updateAddressCounts(
                        cuda_data.stack_traces.stack_traces_val[i], 1
                        );
                }
            }
            break;
            
        case OverflowSamples:
            {
                const CUDA_OverflowSamples& msg =
                    cuda_message.CBTF_cuda_message_u.overflow_samples;

                for (uint32_t i = 0; i < msg.pcs.pcs_len; ++i)
                {
                    dm_addresses.updateAddressCounts(msg.pcs.pcs_val[i], 1);
                }
            }
            break;
            
        default:
            break;
        }
    }
}



//------------------------------------------------------------------------------
// Emit the address buffer containing all of the observed addresses if the
// threads have actually finished. Re-emit the original message unchanged.
//
// It is extremely important that the ThreadsFinished message not be re-
// emitted before the AddressBuffer is emitted. If it is, the frontend sees
// the ThreadsFinished and immediately exits with predictably poor results.
//------------------------------------------------------------------------------
void AddressAggregatorForCUDA::handleThreadsFinished(const bool& value)
{
    if (value)
    {
        emitOutput<AddressBuffer>("AddressBuffer", dm_addresses);
    }

    emitOutput<bool>("ThreadsFinished", value);
}