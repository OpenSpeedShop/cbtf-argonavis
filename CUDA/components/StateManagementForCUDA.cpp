////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2016 Argo Navis Technologies. All Rights Reserved.
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

/** @file Component performing simple state management for CUDA experiments. */

#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <set>
#include <typeinfo>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>

#include <KrellInstitute/Messages/LinkedObjectEvents.h>
#include <KrellInstitute/Messages/ThreadEvents.h>

#include <ArgoNavis/Base/AddressSpaces.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>

using namespace ArgoNavis::Base;
using namespace KrellInstitute::CBTF;



/**
 * Simple (thread and linked object) state management for the experiments that
 * use the CUDA collector. This is used to aggregate the attached threads and
 * their address spaces, forwarding them on to Open|SpeedShop only once all of
 * the attached threads have finished.
 *
 * @note    This component is <em>not</em> scalable to large thread counts.
 *          It is currently being used as a temporary measure until the CUDA
 *          collector can be adapted to use the scalable components found in
 *          the cbtf-krell repository.
 */
class __attribute__ ((visibility ("hidden"))) StateManagementForCUDA :
    public Component
{
    
public:

    /** Factory function for this component type. */
    static Component::Instance factoryFunction()
    {
        return Component::Instance(
            reinterpret_cast<Component*>(new StateManagementForCUDA())
            );
    }
    
private:

    /** Default constructor. */
    StateManagementForCUDA();

    /** Handler for the "AttachedToThreads" input. */
    void handleAttachedToThreads(
        const boost::shared_ptr<CBTF_Protocol_AttachedToThreads>& message
        );

    /** Handler for the "InitialLinkedObjects" input. */
    void handleInitialLinkedObjects(
        const boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>& message
        );
    
    /** Handler for the "LoadedLinkedObject" input. */
    void handleLoadedLinkedObject(
        const boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject>& message
        );

    /** Handler for the "ThreadsStateChanged" input. */
    void handleThreadsStateChanged(
        const boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged>& message
        );
    
    /** Handler for the "UnloadedLinkedObject" input. */
    void handleUnloadedLinkedObject(
        const boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject>& message
        );

    /** Address spaces of all (including terminated) threads. */
    AddressSpaces dm_address_spaces;

    /** Names of all active (non-terminated) threads. */
    std::set<ThreadName> dm_threads;

}; // class StateManagementForCUDA

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(StateManagementForCUDA)



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
StateManagementForCUDA::StateManagementForCUDA() :
    Component(Type(typeid(StateManagementForCUDA)), Version(1, 0, 0)),
    dm_address_spaces(),
    dm_threads()
{
    declareInput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
        "AttachedToThreads",
        boost::bind(&StateManagementForCUDA::handleAttachedToThreads, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "InitialLinkedObjects",
        boost::bind(&StateManagementForCUDA::handleInitialLinkedObjects,
                    this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject> >(
        "LoadedLinkedObject",
        boost::bind(&StateManagementForCUDA::handleLoadedLinkedObject, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged> >(
        "ThreadsStateChanged",
        boost::bind(&StateManagementForCUDA::handleThreadsStateChanged,
                    this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject> >(
        "UnloadedLinkedObject",
        boost::bind(&StateManagementForCUDA::handleUnloadedLinkedObject,
                    this, _1)
        );
        
    declareOutput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
        "AttachedToThreads"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "LinkedObjectGroup"
        );

    declareOutput<bool>("ThreadsFinished");
    declareOutput<bool>("TriggerAddressBuffer");
    declareOutput<bool>("TriggerData");
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void StateManagementForCUDA::handleAttachedToThreads(
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
void StateManagementForCUDA::handleInitialLinkedObjects(
    const boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>& message
    )
{
    dm_address_spaces.apply(*message);
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void StateManagementForCUDA::handleLoadedLinkedObject(
    const boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject>& message
    )
{
    dm_address_spaces.apply(*message);
}



//------------------------------------------------------------------------------
// If threads are terminating, update the list of active threads and, if that
// list is empty, emit a flurry of messages in the proper order.
//------------------------------------------------------------------------------
void StateManagementForCUDA::handleThreadsStateChanged(
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
            emitOutput<bool>("TriggerData", true);
            
            CBTF_Protocol_AttachedToThreads threads = dm_address_spaces;

            emitOutput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
                "AttachedToThreads",
                boost::shared_ptr<CBTF_Protocol_AttachedToThreads>(
                    new CBTF_Protocol_AttachedToThreads(threads)
                    )
                );

            emitOutput<bool>("TriggerAddressBuffer", true);

            std::vector<
                CBTF_Protocol_LinkedObjectGroup
                > groups = dm_address_spaces;

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
        }
    }
}



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void StateManagementForCUDA::handleUnloadedLinkedObject(
    const boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject>& message
    )
{
    dm_address_spaces.apply(*message);
}
