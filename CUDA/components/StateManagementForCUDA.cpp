////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014 Argo Navis Technologies. All Rights Reserved.
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

#include "KrellInstitute/Core/LinkedObjectEntry.hpp"
#include "KrellInstitute/Core/ThreadName.hpp"

#include <KrellInstitute/Messages/LinkedObjectEvents.h>
#include <KrellInstitute/Messages/ThreadEvents.h>

#include "SimpleThreadName.hpp"

using namespace KrellInstitute::CBTF;
using namespace KrellInstitute::Core;



/**
 * Simple (thread and linked object) state management for the experiments that
 * use the CUDA collector. Mostly this component simply forwards messages on to
 * Open|SpeedShop. But it also determines when all of the attached threads have
 * finished and aggregates the list of all linked objects.
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

    /** Handler for the "CreatedProcess" input. */
    void handleCreatedProcess(
        const boost::shared_ptr<CBTF_Protocol_CreatedProcess>& message
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

    /** Names of all active (non-terminated) threads. */
    std::set<SimpleThreadName> dm_threads;

}; // class StateManagementForCUDA

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(StateManagementForCUDA)



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
StateManagementForCUDA::StateManagementForCUDA() :
    Component(Type(typeid(StateManagementForCUDA)), Version(0, 0, 0)),
    dm_threads()
{
    declareInput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
        "AttachedToThreads",
        boost::bind(&StateManagementForCUDA::handleAttachedToThreads, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_CreatedProcess> >(
        "CreatedProcess",
        boost::bind(&StateManagementForCUDA::handleCreatedProcess, this, _1)
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
    declareOutput<boost::shared_ptr<CBTF_Protocol_CreatedProcess> >(
        "CreatedProcess"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "InitialLinkedObjects"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject> >(
        "LoadedLinkedObject"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged> >(
        "ThreadsStateChanged"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject> >(
        "UnloadedLinkedObject"
        );

    declareOutput<LinkedObjectEntryVec>("LinkedObjectEntryVec");
    declareOutput<bool>("ThreadsFinished");
}



//------------------------------------------------------------------------------
// Re-emit the original message unchanged. Update the list of active threads.
//------------------------------------------------------------------------------
void StateManagementForCUDA::handleAttachedToThreads(
    const boost::shared_ptr<CBTF_Protocol_AttachedToThreads>& message
    )
{
    emitOutput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
        "AttachedToThreads", message
        );

    for (u_int i = 0; i < message->threads.names.names_len; ++i)
    {
        dm_threads.insert(
            SimpleThreadName(message->threads.names.names_val[i])
            );
    }
}



//------------------------------------------------------------------------------
// Re-emit the original message unchanged.
//------------------------------------------------------------------------------
void StateManagementForCUDA::handleCreatedProcess(
    const boost::shared_ptr<CBTF_Protocol_CreatedProcess>& message
    )
{
    emitOutput<boost::shared_ptr<CBTF_Protocol_CreatedProcess> >(
        "CreatedProcess", message
        );
}



//------------------------------------------------------------------------------
// Re-emit the original message unchanged. Construct a LinkedObjectEntryVec from
// the original message and emit that object too. Not sure why that is necessary
// but it is what is done by the LinkedObject component in cbtf-krell.
//------------------------------------------------------------------------------
void StateManagementForCUDA::handleInitialLinkedObjects(
    const boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>& message
    )
{
    emitOutput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "InitialLinkedObjects", message
        );

    ThreadName thread(message->thread);

    LinkedObjectEntryVec linked_objects;
    for (int i = 0; i < message->linkedobjects.linkedobjects_len; ++i)
    {
        const CBTF_Protocol_LinkedObject& entry =
            message->linkedobjects.linkedobjects_val[i];
        
        LinkedObjectEntry linked_object;

        linked_object.tname = thread;
        linked_object.path = entry.linked_object.path;
        linked_object.addr_begin = entry.range.begin;
        linked_object.addr_end = entry.range.end;
        linked_object.is_executable = entry.is_executable;
        linked_object.time_loaded = entry.time_begin;
        linked_object.time_unloaded = entry.time_end;
        
        linked_objects.push_back(linked_object);
    }

    emitOutput<LinkedObjectEntryVec>("LinkedObjectEntryVec", linked_objects);
}



//------------------------------------------------------------------------------
// Re-emit the original message unchanged.
//------------------------------------------------------------------------------
void StateManagementForCUDA::handleLoadedLinkedObject(
    const boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject>& message
    )
{
    emitOutput<boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject> >(
        "LoadedLinkedObject", message
        );
}



//------------------------------------------------------------------------------
// Re-emit the original message unchanged. If the threads are being terminated,
// update the list of active threads and, if that list is empty, emit a message
// indicating that all threads have finished.
//------------------------------------------------------------------------------
void StateManagementForCUDA::handleThreadsStateChanged(
    const boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged>& message
    )
{
    emitOutput<boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged> >(
        "ThreadsStateChanged", message
        );

    if (message->state == Terminated)
    {
        for (u_int i = 0; i < message->threads.names.names_len; ++i)
        {
            dm_threads.erase(
                SimpleThreadName(message->threads.names.names_val[i])
                );
        }
        
        if (dm_threads.empty())
        {
            emitOutput<bool>("ThreadsFinished", true);
        }
    }
}



//------------------------------------------------------------------------------
// Re-emit the original message unchanged.
//------------------------------------------------------------------------------
void StateManagementForCUDA::handleUnloadedLinkedObject(
    const boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject>& message
    )
{
    emitOutput<boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject> >(
        "UnloadedLinkedObject", message
        );
}
