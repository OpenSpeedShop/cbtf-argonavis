////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2012-2013 Argo Navis Technologies. All Rights Reserved.
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

/** @file Component for converting CUDA data to pseudo I/O data. */

#include <algorithm>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/operators.hpp>
#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <cmath>
#include <cxxabi.h>
#include <list>
#include <map>
#include <string>
#include <typeinfo>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>

#include <KrellInstitute/Core/AddressBuffer.hpp>
#include <KrellInstitute/Core/ExtentGroup.hpp>
#include <KrellInstitute/Core/Path.hpp>

#include <KrellInstitute/Messages/Address.h>
#include <KrellInstitute/Messages/Blob.h>
#include <KrellInstitute/Messages/File.h>
#include <KrellInstitute/Messages/IO_data.h>
#include <KrellInstitute/Messages/LinkedObjectEvents.h>
#include <KrellInstitute/Messages/PerformanceData.hpp>
#include <KrellInstitute/Messages/Symbol.h>
#include <KrellInstitute/Messages/Thread.h>
#include <KrellInstitute/Messages/ThreadEvents.h>
#include <KrellInstitute/Messages/Time.h>

#include "CUDA_data.h"

using namespace KrellInstitute::CBTF;
using namespace KrellInstitute::Core;



/** Anonymous namespace hiding implementation details. */
namespace {

    /**
     * Constant specifying the (fake) address range of the pseudo linked object
     * containing the dynamically generated symbol table of CUDA operations.
     *
     * @note    This address range is intentionally placed within the kernel's
     *          portion of the process' 64-bit virtual address space. By doing
     *          this, we avoid having to track the location of all of the real
     *          linked objects in the process in order to find a safe location
     *          for the pseudo linked object. I.e. this fixed location is, in
     *          theory, always safe to use.
     */
    const CBTF_Protocol_AddressRange kAddressRange = {
        0xF0BADC00DA000000,
        0xF0BADC00DAFFFFFF
    };

    /**
     * Constant specifying the (fake) checksum of the pseduo linked object
     * containing the dynamically generated symbol table of CUDA operations.
     */
    const uint64_t kChecksum = 0x00000BADC00DAFAD;

    /**
     * Constant specifying the regular expression that identifies the linked
     * objects that are parat of the CUDA collector or CUDA implementation.
     * Used to filter any frames belonging to those entities from stack traces.
     */
    boost::regex kExcludedLinkedObjects(
        "lib(cudart|cublas|cufft|cuinj|curand|cusparse)\\..*\\.so"
        );
    
    /**
     * Constant specifying the (fake) file name of the pseudo linked object
     * containing the dynamically generated symbol table of CUDA operations.
     */
    const char* const kFileName = "CUDA";

    /**
     * Add the given call site to the specified existing stack traces.
     *
     * @param call_site               Call site to be added.
     * @param[in,out] stack_traces    Existing stack traces to which the
     *                                call site is to be added.
     * @return                        Index of the call site within the
     *                                existing stack traces.
     */
    uint32_t addCallSite(
        const std::vector<CBTF_Protocol_Address>& call_site,
        std::vector<CBTF_Protocol_Address>& stack_traces
        )
    {
        uint32_t i, j;
        
        // Iterate over the addresses in the existing stack traces
        for (i = 0, j = 0; i < stack_traces.size(); ++i)
        {
            // Is this the terminating null of an existing stack trace?
            if (stack_traces[i] == 0)
            {
                //
                // Terminate the search if a complete match has been found
                // between the call site and the existing stack trace.
                //

                if (j == call_site.size())
                {
                    break;
                }

                // Otherwise reset the pointer within the call site
                else
                {
                    j = 0;
                }
            }
            else
            {
                //
                // Advance the pointer within the call site if the current
                // address within the call site matches the current address
                // within the existing stack traces. Otherwise reset the
                // pointer.
                //

                j = (call_site[j] == stack_traces[i]) ? (j + 1) : 0;
            }
        }

        //
        // Add the call site to the existing stack traces if the end of the
        // existing stack traces was reached without finding a match.
        // 

        if (i == stack_traces.size())
        {
            stack_traces.insert(
                stack_traces.end(), call_site.begin(), call_site.end()
                );
            stack_traces.push_back(0);

            i = stack_traces.size() - 1;
        }
        
        // Return the index of the call site within the existing stack traces
        return i - call_site.size();
    }

    /**
     * Formats the specified byte count as a string with accompanying units.
     * E.g. an input of 1,614,807 returns "1.5 MB".
     *
     * @param value    Byte count to format.
     * @return         String with accompanying units.
     */
    std::string formatByteCount(const uint64_t& value)
    {
        const struct { const double value; const char* label; } kUnits[] =
        {
            { 1024.0 * 1024.0 * 1024.0 * 1024.0, "TB" },
            {          1024.0 * 1024.0 * 1024.0, "GB" },
            {                   1024.0 * 1024.0, "MB" },
            {                            1024.0, "KB" },
            {                               0.0, NULL } // End-Of-Table
        };
            
        double x = static_cast<double>(value);
        std::string label = "bytes";
            
        for (int i = 0; kUnits[i].label != NULL; ++i)
        {
            if (static_cast<double>(value) >= kUnits[i].value)
            {
                x = static_cast<double>(value) / kUnits[i].value;
                label = kUnits[i].label;
                break;
            }
        }
        
        return boost::str(
            (x == floor(x)) ?
            (boost::format("%1% %2%") % static_cast<uint64_t>(x) % label) :
            (boost::format("%1$0.1f %2%") % x % label)
            );
    }

    /**
     * Convert the specified CUDA_CopiedMemory message to an operation string.
     *
     * @param message    Message to be converted.
     * @return           Operation string describing this message.
     */
    std::string toOperation(const CUDA_CopiedMemory& message)
    {
        const char* kCopyKind[] = {
            "",                      // InvalidCopyKind (0)
            "",                      // UnknownCopyKind (1)
            "from host to device",   // HostToDevice (2)
            "from device to host",   // DeviceToHost (3)
            "from host to array",    // HostToArray (4)
            "from array to host",    // ArrayToHost (5)
            "from array to array",   // ArrayToArray (6)
            "from array to device",  // ArrayToDevice (7)
            "from device to array",  // DeviceToArray (8)
            "from device to device", // DeviceToDevice (9)
            "from host to host"      // HostToHost (10)
        };

        return boost::str(
            boost::format("copy %1% %2%") %
            formatByteCount(message.size) %
            ((message.kind <= 10) ? kCopyKind[message.kind] : "")
            );
    }

    /**
     * Convert the specified CUDA_ExecutedKernel message to an operation string.
     *
     * @param message    Message to be converted.
     * @return           Operation string describing this message.
     */
    std::string toOperation(const CUDA_ExecutedKernel& message)
    {
        std::string function = message.function;
            
        int status = -2;
        char* demangled = abi::__cxa_demangle(
            function.c_str(), NULL, NULL, &status
            );

        if (demangled != NULL)
        {
            if (status == 0)
            {
                function = std::string(demangled);
            }
            free(demangled);
        }

        if (message.dynamic_shared_memory > 0)
        {
            return boost::str(
                boost::format("%1%<<<[%2%,%3%,%4%], [%5%,%6%,%7%], %8%>>>") %
                function %
                message.grid[0] % message.grid[1] % message.grid[2] %
                message.block[0] % message.block[1] % message.block[2] %
                message.dynamic_shared_memory
                );
        }
        else
        {
            return boost::str(
                boost::format("%1%<<<[%2%,%3%,%4%], [%5%,%6%,%7%]>>>") %
                function %
                message.grid[0] % message.grid[1] % message.grid[2] %
                message.block[0] % message.block[1] % message.block[2]
                );
        }
    }
    
    /**
     * Convert the specified CUDA_SetMemory message to an operation string.
     *
     * @param message    Message to be converted.
     * @return           Operation string describing this message.
     */
    std::string toOperation(const CUDA_SetMemory& message)
    {
        return boost::str(
            boost::format("set %1% on device") % formatByteCount(message.size)
            );
    }

} // namespace <anonymous>



/**
 * Converter that translates the performance data blobs generated by the CUDA
 * collector into pseudo performance data blobs that appear as if they had been
 * generated by the I/O collector. This allows Open|SpeedShop's existing views
 * for the I/O collector to work directly with the performance data generated
 * by the CUDA collector.
 *
 * @note    Along the way, this converter must create pseudo address space
 *          mappings, linked objects, and functions so that the various CUDA
 *          kernel invocations, memory copies, and memory sets can all appear
 *          as if they were I/O functions.
 */
class __attribute__ ((visibility ("hidden"))) CUDAToIO :
    public Component
{
    
public:

    /** Factory function for this component type. */
    static Component::Instance factoryFunction()
    {
        return Component::Instance(
            reinterpret_cast<Component*>(new CUDAToIO())
            );
    }
    
private:

    /**
     * Type of associative container used to map the names of individual
     * CUDA operations to their address within the process' address space.
     */
    typedef std::map<std::string, CBTF_Protocol_Address> OperationTable;

    /**
     * Plain old data (POD) structure describing a single pending request.
     *
     * @sa http://en.wikipedia.org/wiki/Plain_old_data_structure
     */
    struct Request
    {
        /** Original message describing the enqueued request. */
        CUDA_EnqueueRequest message;
        
        /** Expanded call site of the request. */
        std::vector<CBTF_Protocol_Address> call_site;
    };

    /**
     * Simplification of the ThreadName class, limited to the host
     * name, process identifier, and POSIX thread identifier only.
     */
    class SimpleThreadName :
        private boost::equivalent<SimpleThreadName>,
        private boost::totally_ordered<SimpleThreadName>
    {
        
    public:
        
        /** Constructor from individual fields. */
        SimpleThreadName(const std::string& host,
                         const int64_t& pid,
                         const int64_t& tid) :
            dm_host(host),
            dm_pid(pid),
            dm_tid(tid)
        {            
        }
        
        /** Constructor from a CBTF_Protocol_ThreadName. */
        SimpleThreadName(const CBTF_Protocol_ThreadName& name) :
            dm_host(name.host),
            dm_pid(name.pid),
            dm_tid(name.has_posix_tid ? name.posix_tid : 0)
        {
        }
        
        /** Is this simple thread name less than another one? */
        bool operator<(const SimpleThreadName& other) const
        {
            if(dm_host < other.dm_host)
                return true;
            if(dm_host > other.dm_host)
                return false;
            if(dm_pid < other.dm_pid)
                return true;
            if(dm_pid > other.dm_pid)
                return false;
            return dm_tid < other.dm_tid;
        }
        
    private:
        
        /** Name of the host on which this thread is located. */
        std::string dm_host;
        
        /** Identifier of the process containing this thread. */
        int64_t dm_pid;
        
        /** POSIX identifier of the thread. */
        int64_t dm_tid;
        
    }; // class SimpleThreadName

    /**
     * Plain old data (POD) structure holding thread-specific data.
     *
     * @sa http://en.wikipedia.org/wiki/Plain_old_data_structure
     */
    struct ThreadSpecificData
    {
        /** Table of pending requests. */
        std::list<Request> requests;

        /** Table of I/O events for completed CUDA operations. */
        std::vector<CBTF_io_event> events;

        /** Table of I/O stack traces for completed CUDA operations. */
        std::vector<CBTF_Protocol_Address> stack_traces;

        /**
         * Group of extents for all linked objects that are not part of the
         * CUDA implementation or CUDA collector. Used to filter any frames
         * belonging to those entities from stack traces.
         */
        ExtentGroup extents;
        
        /**
         * Map of linked object paths to their corresponding (most recent)
         * entry in the table above. Needed in order to find the entry to
         * update when linked objects are unloaded.
         */
        std::vector<boost::filesystem::path> extents_paths;
    };

    /**
     * Type of associative container used to map simple thread names to
     * their thread-specific data.
     */
    typedef std::map<SimpleThreadName, ThreadSpecificData> ThreadTable;

    /** Default constructor. */
    CUDAToIO();
    
    /** Complete the specified CUDA operation. */
    template <typename T> void complete(
        ThreadSpecificData& tsd, const CUDA_RequestTypes& type, const T& message
        );
    
    /** Handler for the "AttachedToThreads" input. */
    void handleAttachedToThreads(
        const boost::shared_ptr<CBTF_Protocol_AttachedToThreads>& message
        );
    
    /** Handler for the "Data" input. */
    void handleData(
        const boost::shared_ptr<CBTF_Protocol_Blob>& message
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
    
    /** Address buffer containing all addresses seen within stack traces. */
    AddressBuffer dm_addresses;
    
    /**
     * Next available (unused) address within the dynamically generated
     * symbol table of CUDA operations.
     */
    CBTF_Protocol_Address dm_next_address;

    /** Table of individual CUDA operations. */
    OperationTable dm_operations;

    /** Thread-specific data for all active (non-terminated) threads. */
    ThreadTable dm_threads;
    
}; // class CUDAToIO

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(CUDAToIO)



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CUDAToIO::CUDAToIO() :
    Component(Type(typeid(CUDAToIO)), Version(0, 0, 0)),
    dm_addresses(),
    dm_next_address(kAddressRange.begin),
    dm_operations(),
    dm_threads()
{
    declareInput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
        "AttachedToThreads",
        boost::bind(&CUDAToIO::handleAttachedToThreads, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_Blob> >(
        "Data",
        boost::bind(&CUDAToIO::handleData, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "InitialLinkedObjects",
        boost::bind(&CUDAToIO::handleInitialLinkedObjects, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject> >(
        "LoadedLinkedObject",
        boost::bind(&CUDAToIO::handleLoadedLinkedObject, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged> >(
        "ThreadsStateChanged",
        boost::bind(&CUDAToIO::handleThreadsStateChanged, this, _1)
        );
    declareInput<boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject> >(
        "UnloadedLinkedObject",
        boost::bind(&CUDAToIO::handleUnloadedLinkedObject, this, _1)
        );

    declareOutput<AddressBuffer>(
        "AddressBuffer"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
        "AttachedToThreads"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_Blob> >(
        "Data"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "InitialLinkedObjects"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject> >(
        "LoadedLinkedObject"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_SymbolTable> >(
        "SymbolTable"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged> >(
        "ThreadsStateChanged"
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject> >(
        "UnloadedLinkedObject"
        );
}



//------------------------------------------------------------------------------
// Complete the specified CUDA operation by matching it with the corresponding
// previously-queued request, constructing a pseudo I/O event for it, and then
// appending that event to our tables.
//------------------------------------------------------------------------------
template <typename T>
void CUDAToIO::complete(ThreadSpecificData& tsd,
                        const CUDA_RequestTypes& type, const T& message)
{
    //
    // Locate the next queued request of the same request type, CUDA context,
    // and CUDA stream as the completed CUDA operation.
    //
    
    for (std::list<Request>::iterator i = tsd.requests.begin();
         i != tsd.requests.end();
         ++i)
    {
        if ((i->message.type == type) &&
            (i->message.context == message.context) &&
            (i->message.stream == message.stream))
        {
            //
            // Iterate over the frames in the stack trace of the call site,
            // working from main() torwards the final call site, looking for
            // the first frame that is contained in a linked object that is
            // part of the CUDA collector or CUDA implementation. Trim the
            // stack trace from there all the way to the final call site.
            //
            
            for (int j = i->call_site.size() - 1; j > 0; --j)
            {
                Extent extent(
                    TimeInterval(i->message.time, i->message.time + 1),
                    AddressRange(i->call_site[j], i->call_site[j] + 1)
                    );
                
                if (tsd.extents.getIntersectionWith(extent).empty())
                {
                    std::vector<CBTF_Protocol_Address> new_call_site;
                    
                    for (int k = j + 1; k < i->call_site.size(); ++k)
                    {
                        new_call_site.push_back(i->call_site[k]);
                    }
                    
                    i->call_site = new_call_site;
                }
            }
            
            // Create the operation string for this completed CUDA operation
            std::string operation = toOperation(message);
            
            //
            // Find the existing address corresponding to this CUDA operation
            // or, if no such address exists yet, assign it the next available
            // address. In either case, prepend the address to the beginning of
            // the request's call site.
            //
            
            OperationTable::const_iterator j = dm_operations.find(operation);

            if (j == dm_operations.end())
            {
                j = dm_operations.insert(
                    OperationTable::value_type(operation, dm_next_address++)
                    ).first;
            }

            i->call_site.insert(i->call_site.begin(), j->second);
            
            //
            // Construct a CBTF_io_event entry for this completed CUDA operation
            // and then push it onto our table of I/O events. The modified call
            // site is added (when necessary) to our table of I/O stack traces.
            // Finally, the address buffer is updated with the contents of the
            // stack trace.
            //
            
            CBTF_io_event event;
            
            event.start_time = message.time_begin;
            event.stop_time = message.time_end;
            event.stacktrace = addCallSite(i->call_site, tsd.stack_traces);

            for (std::vector<CBTF_Protocol_Address>::size_type
                     x = event.stacktrace;
                 tsd.stack_traces[x] != 0;
                 ++x)
            {
                dm_addresses.updateAddressCounts(tsd.stack_traces[x], 1);
            }

            tsd.events.push_back(event);
            
            // Remove this request from the queue of pending requests
            tsd.requests.erase(i);
            
            // And now exit the search...
            break;
        }
    }
}



//------------------------------------------------------------------------------
// Update the list of active threads.
//------------------------------------------------------------------------------
void CUDAToIO::handleAttachedToThreads(
    const boost::shared_ptr<CBTF_Protocol_AttachedToThreads>& message
    )
{
    // Re-emit the original message unchanged
    emitOutput<boost::shared_ptr<CBTF_Protocol_AttachedToThreads> >(
        "AttachedToThreads", message
        );
    
    // Update the list of active threads appropriately
    for (u_int i = 0; i < message->threads.names.names_len; ++i)
    {
        dm_threads.insert(std::make_pair(
            SimpleThreadName(message->threads.names.names_val[i]),
            ThreadSpecificData()
            ));
    }
}



//------------------------------------------------------------------------------
// Translate between the CBTF_cuda_data and CBTF_io_trace_data performance data
// blobs and emit the resulting, translated, blob.
//------------------------------------------------------------------------------
void CUDAToIO::handleData(
    const boost::shared_ptr<CBTF_Protocol_Blob>& message
    )
{
    std::pair<
        boost::shared_ptr<CBTF_DataHeader>, boost::shared_ptr<CBTF_cuda_data>
        > unpacked_message = KrellInstitute::Messages::unpack<CBTF_cuda_data>(
            message, reinterpret_cast<xdrproc_t>(xdr_CBTF_cuda_data)
            );

    const CBTF_DataHeader& cuda_data_header = *unpacked_message.first;
    const CBTF_cuda_data& cuda_data = *unpacked_message.second;

    //
    // Locate the thread-specific data belonging to the thread for which
    // this data was gathered. Ignore the data if no such thread exists.
    //

    ThreadTable::iterator t = dm_threads.find(SimpleThreadName(
        cuda_data_header.host, cuda_data_header.pid, cuda_data_header.posix_tid
        ));

    if (t == dm_threads.end())
    {
        return;
    }

    ThreadSpecificData& tsd = t->second;
     
    //
    // Iterate over each of the individual CUDA messages "packed" into this
    // performance data blob. Queued requests are pushed onto our own queue
    // of such requests after first expanding the request's call site. When
    // requests are completed they are pushed onto our appropriate tables.
    //
    
    for (u_int i = 0; i < cuda_data.messages.messages_len; ++i)
    {
        const CBTF_cuda_message& cuda_message =
            cuda_data.messages.messages_val[i];
        
        switch (cuda_message.type)
        {
            
        case CopiedMemory:
            {
                const CUDA_CopiedMemory& msg =
                    cuda_message.CBTF_cuda_message_u.copied_memory;
                
                complete(tsd, MemoryCopy, msg);
            }
            break;
            
        case EnqueueRequest:
            {
                const CUDA_EnqueueRequest& msg = 
                    cuda_message.CBTF_cuda_message_u.enqueue_request;

                Request request;
                request.message = msg;
                
                for (uint32_t i = msg.call_site;
                     (i < cuda_data.stack_traces.stack_traces_len) &&
                         (cuda_data.stack_traces.stack_traces_val[i] != 0);
                     ++i)
                {
                    request.call_site.push_back(
                        cuda_data.stack_traces.stack_traces_val[i]
                        );
                }
                
                tsd.requests.push_back(request);
            }
            break;
            
        case ExecutedKernel:
            {
                const CUDA_ExecutedKernel& msg =
                    cuda_message.CBTF_cuda_message_u.executed_kernel;
                
                complete(tsd, LaunchKernel, msg);
            }
            break;
            
        case SetMemory:
            {
                const CUDA_SetMemory& msg =
                    cuda_message.CBTF_cuda_message_u.set_memory;
                
                complete(tsd, MemorySet, msg);
            }
            break;
            
        default:
            break;
        }
    }
    
    //
    // Construct a new CBTF_io_trace_data from the two tables of completed
    // CUDA operations. Direct memory copies are possible here because the
    // only difference between our internal tables and the performance data
    // blob is the use of (expandable) vectors for the former.
    //

    std::pair<
        boost::shared_ptr<CBTF_DataHeader>,
        boost::shared_ptr<CBTF_io_trace_data>
        > pack_message(
            boost::shared_ptr<CBTF_DataHeader>(new CBTF_DataHeader()),
            boost::shared_ptr<CBTF_io_trace_data>(new CBTF_io_trace_data())
            );

    CBTF_DataHeader& io_data_header = *pack_message.first;
    CBTF_io_trace_data& io_data = *pack_message.second;

    io_data_header.experiment = cuda_data_header.experiment;
    io_data_header.collector = cuda_data_header.collector;
    io_data_header.id = strdup("io");
    memcpy(&io_data_header.host, &cuda_data_header.host,
           sizeof(io_data_header.host));
    io_data_header.pid = cuda_data_header.pid;
    io_data_header.posix_tid = cuda_data_header.posix_tid;
    io_data_header.rank = cuda_data_header.rank;
    io_data_header.time_begin = cuda_data_header.time_begin;
    io_data_header.time_end = cuda_data_header.time_end;
    io_data_header.addr_begin = -1;
    io_data_header.addr_end = 0;

    for (std::vector<CBTF_Protocol_Address>::const_iterator
             i = tsd.stack_traces.begin(); i != tsd.stack_traces.end(); ++i)
    {
        if (*i != 0)
        {
            io_data_header.addr_begin = std::min(
                io_data_header.addr_begin, *i
                );
            io_data_header.addr_end = 1 + std::max(
                io_data_header.addr_end, *i
                );
        }
    }
    
    io_data.stacktraces.stacktraces_len = tsd.stack_traces.size();
    
    io_data.stacktraces.stacktraces_val =
        reinterpret_cast<CBTF_Protocol_Address*>(
            malloc(std::max(1U, io_data.stacktraces.stacktraces_len) *
                   sizeof(CBTF_Protocol_Address))
            );
    
    if (!tsd.stack_traces.empty())
    {
        memcpy(io_data.stacktraces.stacktraces_val, &tsd.stack_traces[0],
               tsd.stack_traces.size() * sizeof(CBTF_Protocol_Address));        
    }
    
    io_data.events.events_len = tsd.events.size();
    
    io_data.events.events_val = reinterpret_cast<CBTF_io_event*>(
        malloc(std::max(1U, io_data.events.events_len) * sizeof(CBTF_io_event))
        );
    
    if (!tsd.events.empty())
    {
        memcpy(io_data.events.events_val, &tsd.events[0],
               tsd.events.size() * sizeof(CBTF_io_event));        
    }
    
    // Empty the two tables of completed CUDA operations
    tsd.events.clear();
    tsd.stack_traces.clear();
    
    // Emit the CBTF_io_trace_data on our appropriate output
    emitOutput<boost::shared_ptr<CBTF_Protocol_Blob> >(
        "Data", 
        KrellInstitute::Messages::pack<CBTF_io_trace_data>(
            pack_message, reinterpret_cast<xdrproc_t>(xdr_CBTF_io_trace_data)
            )
        );
}



//------------------------------------------------------------------------------
// Append an entry for the dynamically generated symbol table of CUDA operations
// to the list of initial linked objects within this thread.
//------------------------------------------------------------------------------
void CUDAToIO::handleInitialLinkedObjects(
    const boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup>& message
    )
{
    //
    // Locate the thread-specific data belonging to the thread for which this
    // message was generated. Ignore the message if no such thread exists.
    //

    ThreadTable::iterator t = dm_threads.find(
        SimpleThreadName(message->thread)
        );
    
    if (t == dm_threads.end())
    {
        return;
    }
    
    ThreadSpecificData& tsd = t->second;
    
    //
    // Build the initial group of extents for all linked objects that are
    // not part of the CUDA collector or CUDA implementation.
    // 
    
    for (int i = 0; i < message->linkedobjects.linkedobjects_len; ++i)
    {
        const CBTF_Protocol_LinkedObject& entry =
            message->linkedobjects.linkedobjects_val[i];
        
        boost::filesystem::path path(entry.linked_object.path);
        
        if (!boost::regex_match(path.filename(), kExcludedLinkedObjects))
        {
            tsd.extents.push_back(
                Extent(TimeInterval(Time::TheBeginning(), Time::TheEnd()),
                       AddressRange(entry.range.begin, entry.range.end))
                );
            
            tsd.extents_paths.push_back(path);
        }
    }
    
    //
    // Create a deep copy of the incoming CBTF_Protocol_LinkedObjectGroup
    // message, allocating an extra linked object entry in the table that
    // will refer to our dynamically generated symbol table.
    //

    boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> group(
        new CBTF_Protocol_LinkedObjectGroup()
        );
    
    memcpy(&group->thread, &message->thread, sizeof(CBTF_Protocol_ThreadName));
    group->thread.host = strdup(message->thread.host);
    
    group->linkedobjects.linkedobjects_len = 
        message->linkedobjects.linkedobjects_len + 1;
    
    group->linkedobjects.linkedobjects_val = 
        reinterpret_cast<CBTF_Protocol_LinkedObject*>(            
            malloc(group->linkedobjects.linkedobjects_len * 
                   sizeof(CBTF_Protocol_LinkedObject))
            );
    
    for (int i = 0; i < message->linkedobjects.linkedobjects_len; ++i)
    {
        const CBTF_Protocol_LinkedObject* source =
            &message->linkedobjects.linkedobjects_val[i];
        
        CBTF_Protocol_LinkedObject* destination =
            &group->linkedobjects.linkedobjects_val[i];
        
        memcpy(destination, source, sizeof(CBTF_Protocol_LinkedObject));
        destination->linked_object.path = strdup(source->linked_object.path);
    }
    
    //
    // Create the linked object entry for our dynamically generated symbol
    // table. The entry is placed at the same, fixed, address range in all
    // threads, for the entire possible time interval.
    //
    
    CBTF_Protocol_LinkedObject* linked_object =
        &group->linkedobjects.linkedobjects_val[
            group->linkedobjects.linkedobjects_len - 1
            ];
    
    linked_object->linked_object.path = strdup(kFileName);
    linked_object->linked_object.checksum = kChecksum;
    linked_object->range.begin = kAddressRange.begin;
    linked_object->range.end = kAddressRange.end;
    linked_object->time_begin = 0;
    linked_object->time_end = -1;
    linked_object->is_executable = FALSE;
    
    //
    // Emit the resulting, modified, CBTF_Protocol_LinkedObjectGroup message
    // on our appropriate output. The modified message should take the place
    // of the original message in any upstream processing.
    //
    
    emitOutput<boost::shared_ptr<CBTF_Protocol_LinkedObjectGroup> >(
        "InitialLinkedObjects", group
        );
}



//------------------------------------------------------------------------------
// Update the group of extents for all linked objects that are not part of the
// CUDA implementation or CUDA collector.
//------------------------------------------------------------------------------
void CUDAToIO::handleLoadedLinkedObject(
    const boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject>& message
    )
{
    // Re-emit the original message unchanged
    emitOutput<boost::shared_ptr<CBTF_Protocol_LoadedLinkedObject> >(
        "LoadedLinkedObject", message
        );

    //
    // There is no need to update anything if this linked
    // object is one of the linked objects being excluded.
    //
    
    boost::filesystem::path path(message->linked_object.path);
    
    if (boost::regex_match(path.filename(), kExcludedLinkedObjects))
    {
        return;
    }

    // Iterate over each thread for which this linked object is unloaded
    for (u_int i = 0; i < message->threads.names.names_len; ++i)
    {
        // 
        // Locate the thread-specific data belonging to this
        // thread. Ignore this thread if it cannot be located.
        //
        
        ThreadTable::iterator t = dm_threads.find(
            SimpleThreadName(message->threads.names.names_val[i])
            );
        
        if (t == dm_threads.end())
        {
            continue;
        }
        
        ThreadSpecificData& tsd = t->second;

        //
        // Add this linked object to the group of extents
        // 
        
        tsd.extents.push_back(
            Extent(TimeInterval(message->time, Time::TheEnd()),
                   AddressRange(message->range.begin, message->range.end))
            );
        
        tsd.extents_paths.push_back(path);
    }
}



//------------------------------------------------------------------------------
// Update the list of active threads and emit our dynamically generated symbol
// table of CUDA operations if the last thread has been terminated.
//
// It is extremely important that the final thread termination message not be
// re-emitted before the CBTF_Protocol_SymbolTable and AddressBuffer messages
// are emitted. When it is, the frontend sees the final thread terminating and
// immediately exits with predictably poor results. Hence the multiple copies
// of the re-emit below to make sure they are done in the correct order...
//------------------------------------------------------------------------------
void CUDAToIO::handleThreadsStateChanged(
    const boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged>& message
    )
{
    // We only care when threads are terminated
    if (message->state != Terminated)
    {
        // Re-emit the original message unchanged
        emitOutput<boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged> >(
            "ThreadsStateChanged", message
            );

        return;
    }
    
    // Update the list of active threads appropriately
    for (u_int i = 0; i < message->threads.names.names_len; ++i)
    {
        dm_threads.erase(SimpleThreadName(message->threads.names.names_val[i]));
    }
    
    // Do not proceed further unless the last thread has now been terminated
    if (!dm_threads.empty())
    {
        // Re-emit the original message unchanged
        emitOutput<boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged> >(
            "ThreadsStateChanged", message
            );
        
        return;
    }
    
    //
    // Construct a new CBTF_Protocol_SymbolTable from the table of individual
    // CUDA operations. Each of the individual CUDA operations is represented
    // as a single function. No statements are generated. The code below is a
    // bit longer than might otherwise be expected because XDR does not allow
    // the value pointer of variable-length arrays to be NULL. Thus we must
    // generated dummy entries in a couple cases.
    //
    
    boost::shared_ptr<CBTF_Protocol_SymbolTable> table(
        new CBTF_Protocol_SymbolTable()
        );
    
    table->linked_object.path = strdup(kFileName);
    table->linked_object.checksum = kChecksum;
    
    table->functions.functions_len = dm_operations.size();
    
    table->functions.functions_val = 
        reinterpret_cast<CBTF_Protocol_FunctionEntry*>(
            malloc(std::max(1U, table->functions.functions_len) *
                   sizeof(CBTF_Protocol_FunctionEntry))
            );

    int n = 0;
    for (OperationTable::const_iterator
             i = dm_operations.begin(); i != dm_operations.end(); ++i, ++n)
    {
        CBTF_Protocol_FunctionEntry* function = 
            &table->functions.functions_val[n];
        
        function->name = strdup(i->first.c_str());        
        function->bitmaps.bitmaps_len = 1;
        
        function->bitmaps.bitmaps_val =
            reinterpret_cast<CBTF_Protocol_AddressBitmap*>(
                malloc(sizeof(CBTF_Protocol_AddressBitmap))
                );
        
        CBTF_Protocol_AddressBitmap* bitmap = &function->bitmaps.bitmaps_val[0];
        
        bitmap->range.begin = i->second - kAddressRange.begin;
        bitmap->range.end = i->second - kAddressRange.begin + 1;

        bitmap->bitmap.data.data_len = 1;
        bitmap->bitmap.data.data_val = reinterpret_cast<uint8_t*>(malloc(1));
        bitmap->bitmap.data.data_val[0] = 0xFF;
    }

    table->statements.statements_len = 0;

    table->statements.statements_val = 
        reinterpret_cast<CBTF_Protocol_StatementEntry*>(
            malloc(sizeof(CBTF_Protocol_StatementEntry))
            );
    
    CBTF_Protocol_StatementEntry* statement =
        &table->statements.statements_val[0];

    statement->path.path = strdup("");
    statement->path.checksum = 0;
    statement->line = 0;
    statement->column = 0;
    
    statement->bitmaps.bitmaps_len = 1;
        
    statement->bitmaps.bitmaps_val =
        reinterpret_cast<CBTF_Protocol_AddressBitmap*>(
            malloc(sizeof(CBTF_Protocol_AddressBitmap))
            );
    
    CBTF_Protocol_AddressBitmap* bitmap = &statement->bitmaps.bitmaps_val[0];
    
    bitmap->range.begin = 0;
    bitmap->range.end = 0;

    bitmap->bitmap.data.data_len = 1;
    bitmap->bitmap.data.data_val = reinterpret_cast<uint8_t*>(malloc(1));
    bitmap->bitmap.data.data_val[0] = 0;
    
    // Emit the CBTF_Protocol_SymbolTable on our appropriate output
    emitOutput<boost::shared_ptr<CBTF_Protocol_SymbolTable> >(
        "SymbolTable", table
        );
    
    // Emit the AddressBuffer for all addresses seen within stack traces
    emitOutput<AddressBuffer>("AddressBuffer", dm_addresses);

    // Re-emit the original message unchanged
    emitOutput<boost::shared_ptr<CBTF_Protocol_ThreadsStateChanged> >(
        "ThreadsStateChanged", message
        );
}



//------------------------------------------------------------------------------
// Update the group of extents for all linked objects that are not part of the
// CUDA implementation or CUDA collector.
//------------------------------------------------------------------------------
void CUDAToIO::handleUnloadedLinkedObject(
    const boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject>& message
    )
{
    // Re-emit the original message unchanged
    emitOutput<boost::shared_ptr<CBTF_Protocol_UnloadedLinkedObject> >(
        "UnloadedLinkedObject", message
        );

    //
    // There is no need to update anything if this linked
    // object is one of the linked objects being excluded.
    //
    
    boost::filesystem::path path(message->linked_object.path);
    
    if (boost::regex_match(path.filename(), kExcludedLinkedObjects))
    {
        return;
    }

    // Iterate over each thread for which this linked object is unloaded
    for (u_int i = 0; i < message->threads.names.names_len; ++i)
    {
        // 
        // Locate the thread-specific data belonging to this
        // thread. Ignore this thread if it cannot be located.
        //
        
        ThreadTable::iterator t = dm_threads.find(
            SimpleThreadName(message->threads.names.names_val[i])
            );
        
        if (t == dm_threads.end())
        {
            continue;
        }
        
        ThreadSpecificData& tsd = t->second;
        
        //
        // Remove this linked object from the group of extents
        // 

        for (ExtentGroup::size_type i = 0; i < tsd.extents.size(); ++i)
        {
            if (tsd.extents_paths[i] == path)
            {
                tsd.extents[i] = Extent(
                    TimeInterval(tsd.extents[i].getTimeInterval().getBegin(),
                                 message->time),
                    tsd.extents[i].getAddressRange()
                    );
                
                tsd.extents_paths[i].clear();

                break;
            }
        }
        
        //
        // The above is all that SHOULD be needed. Unfortunately, however,
        // the group of extents won't rebuild its internal kD-tree because
        // the number of entries in the group hasn't changed. So we recopy
        // the group here to force the update.
        //

        ExtentGroup new_extents;
        std::copy(new_extents.begin(), tsd.extents.begin(), tsd.extents.end());
        tsd.extents = new_extents;        
    }
}
