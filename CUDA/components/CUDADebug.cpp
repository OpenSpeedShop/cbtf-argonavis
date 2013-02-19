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

/** @file Component for debugging CUDA performance data blobs. */

#include <algorithm>
#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <cmath>
#include <cxxabi.h>
#include <iostream>
#include <list>
#include <rpc/rpc.h>
#include <sstream>
#include <string>
#include <typeinfo>
#include <utility>

#include <KrellInstitute/CBTF/Component.hpp>
#include <KrellInstitute/CBTF/Type.hpp>
#include <KrellInstitute/CBTF/Version.hpp>

#include <KrellInstitute/Messages/Address.h>
#include <KrellInstitute/Messages/Blob.h>
#include <KrellInstitute/Messages/DataHeader.h>
#include <KrellInstitute/Messages/File.h>
#include <KrellInstitute/Messages/PerformanceData.hpp>
#include <KrellInstitute/Messages/Time.h>

#include "CUDA_data.h"

using namespace KrellInstitute::CBTF;



/** Anonymous namespace hiding implementation details. */
namespace {
    
    /** Type of container used to store ordered fields (key/value pairs). */
    typedef std::list<boost::tuples::tuple<std::string, std::string> > Fields;

    /**
     * Type representing a byte count. Its only reason for existence is to
     * allow the Format<> template below to be specialized for byte counts
     * versus other integer values.
     */
    class ByteCount
    {
    public:
        ByteCount(const uint64_t& value) : dm_value(value) { }
        operator uint64_t() const { return dm_value; }
    private:
        uint64_t dm_value;
    };

    /**
     * Type representing a clock rate. Its only reason for existence is to
     * allow the Format<> template below to be specialized for clock rates
     * versus other integer values.
     */
    class ClockRate
    {
    public:
        ClockRate(const uint64_t& value) : dm_value(value) { }
        operator uint64_t() const { return dm_value; }
    private:
        uint64_t dm_value;
    };

    /**
     * Type representing a function name. Its only reason for existence is
     * to allow the Format<> template below to be specialized for function
     * names versus other strings.
     */
    class FunctionName
    {
    public:
        FunctionName(const char* value) : dm_value(value) { }
        FunctionName(const std::string& value) : dm_value(value) { }
        operator std::string() const { return dm_value; }
    private:
        std::string dm_value;
    };
    
    /**
     * Implementation of the format() function. This template is used to get
     * around the C++ prohibition against partial template specialization of
     * functions.
     */
    template <typename T>
    struct Format
    {
        static std::string impl(const T& value)
        {
            return boost::str(boost::format("%1%") % value);
        }
    };
    
    template <>
    struct Format<bool>
    {
        static std::string impl(const bool& value)
        {
            return value ? "true" : "false";
        }
    };
    
    template <>
    struct Format<uint64_t>
    {
        static std::string impl(const uint64_t& value)
        {
            return boost::str(boost::format("0x%016X") % value);
        }
    };

    template <typename T>
    struct Format<std::vector<T> >
    {
        static std::string impl(const std::vector<T>& value)
        {
            std::ostringstream stream;
            
            stream << "[";
            
            for (typename std::vector<T>::size_type 
                     i = 0; i < value.size(); ++i)
            {
                stream << value[i];
                
                if (i < (value.size() - 1))
                {
                    stream << ", ";
                }
            }
            
            stream << "]";
            
            return stream.str();
        }
    };
    
    template <>
    struct Format<CBTF_Protocol_FileName>
    {
        static std::string impl(const CBTF_Protocol_FileName& value)
        {
            return boost::str(boost::format("%1% (%2%)") % 
                value.path % Format<uint64_t>::impl(value.checksum)
                );
        }
    };

    template <>
    struct Format<CUDA_CachePreference>
    {
        static std::string impl(const CUDA_CachePreference& value)
        {
            switch (value)
            {
            case InvalidCachePreference: return "InvalidCachePreference";
            case NoPreference: return "NoPreference";
            case PreferShared: return "PreferShared";
            case PreferCache: return "PreferCache";
            case PreferEqual: return "PreferEqual";
            }
            return "?";
        }
    };

    template <>
    struct Format<CUDA_CopyKind>
    {
        static std::string impl(const CUDA_CopyKind& value)
        {
            switch (value)
            {
            case InvalidCopyKind: return "InvalidCopyKind";
            case UnknownCopyKind: return "UnknownCopyKind";
            case HostToDevice: return "HostToDevice";
            case DeviceToHost: return "DeviceToHost";
            case HostToArray: return "HostToArray";
            case ArrayToHost: return "ArrayToHost";
            case ArrayToArray: return "ArrayToArray";
            case ArrayToDevice: return "ArrayToDevice";
            case DeviceToArray: return "DeviceToArray";
            case DeviceToDevice: return "DeviceToDevice";
            case HostToHost: return "HostToHost";
            }
            return "?";
        }
    };

    template <>
    struct Format<CUDA_MemoryKind>
    {
        static std::string impl(const CUDA_MemoryKind& value)
        {
            switch (value)
            {
            case InvalidMemoryKind: return "InvalidMemoryKind";
            case UnknownMemoryKind: return "UnknownMemoryKind";
            case Pageable: return "Pageable";
            case Pinned: return "Pinned";
            case Device: return "Device";
            case Array: return "Array";
            }
            return "?";
        }
    };

    template <>
    struct Format<CUDA_MessageTypes>
    {
        static std::string impl(const CUDA_MessageTypes& value)
        {
            switch (value)
            {
            case ContextInfo: return "ContextInfo";
            case CopiedMemory: return "CopiedMemory";
            case DeviceInfo: return "DeviceInfo";
            case EnqueueRequest: return "EnqueueRequest";
            case ExecutedKernel: return "ExecutedKernel";
            case LoadedModule: return "LoadedModule";
            case ResolvedFunction: return "ResolvedFunction";
            case SetMemory: return "SetMemory";
            case UnloadedModule: return "UnloadedModule";
            }
            return "?";
        }
    };

    template <>
    struct Format<CUDA_RequestTypes>
    {
        static std::string impl(const CUDA_RequestTypes& value)
        {
            switch (value)
            {
            case LaunchKernel: return "LaunchKernel";
            case MemoryCopy: return "MemoryCopy";
            case MemorySet: return "MemorySet";
            }
            return "?";
        }
    };

    template <>
    struct Format<ByteCount>
    {
        static std::string impl(const ByteCount& value)
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
            std::string label = "Bytes";
            
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
    };

    template <>
    struct Format<ClockRate>
    {
        static std::string impl(const ClockRate& value)
        {
            const struct { const double value; const char* label; } kUnits[] =
            {
                { 1024.0 * 1024.0 * 1024.0 * 1024.0, "THz" },
                {          1024.0 * 1024.0 * 1024.0, "GHz" },
                {                   1024.0 * 1024.0, "MHz" },
                {                            1024.0, "KHz" },
                {                               0.0, NULL  } // End-Of-Table
            };
            
            double x = static_cast<double>(value);
            std::string label = "Hz";
            
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
    };

    template <>
    struct Format<FunctionName>
    {
        static std::string impl(const FunctionName& value)
        {
            std::string retval = value;
            
            int status = -2;
            char* demangled = abi::__cxa_demangle(
                retval.c_str(), NULL, NULL, &status
                );

            if (demangled != NULL)
            {
                if (status == 0)
                {
                    retval = std::string(demangled);
                }
                free(demangled);
            }

            return retval;
        }
    };
    
    template <>
    struct Format<Fields>
    {
        static std::string impl(const Fields& value)
        {
            std::ostringstream stream;
            
            int n = 0;
            for (Fields::const_iterator 
                     i = value.begin(); i != value.end(); ++i)
            {
                n = std::max<int>(n, i->get<0>().size());
            }
            
            for (Fields::const_iterator 
                     i = value.begin(); i != value.end(); ++i)
            {
                stream << "        ";
                for (int j = 0; j < (n - i->get<0>().size()); ++j)
                {
                    stream << " ";
                }
                stream << i->get<0>() << " = " << i->get<1>() << std::endl;
            }
            
            return stream.str();                
        }
    };

    /** Format the specified value as a string. */
    template <typename T>
    std::string format(const T& value)
    {
        return Format<T>::impl(value);
    }

} // namespace <anonymous>



/**
 * Component that displays the performance data blobs generated by the CUDA
 * collector directly (unprocessed) on the standard C++ output stream. This
 * is used for debugging purposes.
 */
class __attribute__ ((visibility ("hidden"))) CUDADebug :
    public Component
{
    
public:

    /** Factory function for this component type. */
    static Component::Instance factoryFunction()
    {
        return Component::Instance(
            reinterpret_cast<Component*>(new CUDADebug())
            );
    }
    
private:

    /** Default constructor. */
    CUDADebug();

    /** Handler for the "Data" input. */
    void handleData(
        const boost::shared_ptr<CBTF_Protocol_Blob>& message
        );
    
}; // class CUDAToIO

KRELL_INSTITUTE_CBTF_REGISTER_FACTORY_FUNCTION(CUDADebug)



//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
CUDADebug::CUDADebug() :
    Component(Type(typeid(CUDADebug)), Version(0, 0, 0))
{
    declareInput<boost::shared_ptr<CBTF_Protocol_Blob> >(
        "Data", boost::bind(&CUDADebug::handleData, this, _1)
        );
    declareOutput<boost::shared_ptr<CBTF_Protocol_Blob> >("Data");
}



//------------------------------------------------------------------------------
// Iterate over the messages in the incoming CBTF_cuda_data and display them
// to the standard C++ output stream. Then interate over the stack traces and
// do the same.
//------------------------------------------------------------------------------
void CUDADebug::handleData(
    const boost::shared_ptr<CBTF_Protocol_Blob>& message
    )
{
    emitOutput<boost::shared_ptr<CBTF_Protocol_Blob> >("Data", message);
    
    std::pair<
        boost::shared_ptr<CBTF_DataHeader>, boost::shared_ptr<CBTF_cuda_data>
        > unpacked_message = KrellInstitute::Messages::unpack<CBTF_cuda_data>(
            message, reinterpret_cast<xdrproc_t>(xdr_CBTF_cuda_data)
            );

    const CBTF_DataHeader& header = *unpacked_message.first;
    const CBTF_cuda_data& data = *unpacked_message.second;
    
    std::cout << std::endl
              << "CUDA Performance Data Blob" << std::endl
              << std::endl
              << "    experiment = " << header.experiment << std::endl
              << "    collector  = " << header.collector << std::endl
              << "    id         = " << header.id << std::endl
              << "    host       = " << header.host << std::endl
              << "    pid        = " << header.pid << std::endl
              << "    posix_tid  = " << header.posix_tid << std::endl
              << "    rank       = " << header.rank << std::endl
              << "    time_begin = " << format(header.time_begin) << std::endl
              << "    time_end   = " << format(header.time_end) << std::endl
              << "    addr_begin = " << format(header.addr_begin) << std::endl
              << "    addr_end   = " << format(header.addr_end) << std::endl;
    
    for (u_int i = 0; i < data.messages.messages_len; ++i)
    {
        const CBTF_cuda_message& cuda_message = data.messages.messages_val[i];
        
        std::list<boost::tuples::tuple<std::string, std::string> > fields;
        
        switch (cuda_message.type)
        {
            
        case ContextInfo:
            {
                const CUDA_ContextInfo& msg = 
                    cuda_message.CBTF_cuda_message_u.context_info;
                
                fields = boost::assign::tuple_list_of
                    ("context", format(msg.context))
                    ("device", format(msg.device))
                    ("compute_api", format(msg.compute_api));
            }
            break;
            
        case CopiedMemory:
            {
                const CUDA_CopiedMemory& msg = 
                    cuda_message.CBTF_cuda_message_u.copied_memory;
                
                fields = boost::assign::tuple_list_of
                    ("context", format(msg.context))
                    ("stream", format(msg.stream))
                    ("time_begin", format(msg.time_begin))
                    ("time_end", format(msg.time_end))
                    ("size", format<ByteCount>(msg.size))
                    ("kind", format(msg.kind))
                    ("source_kind", format(msg.source_kind))
                    ("destination_kind", format(msg.destination_kind))
                    ("asynchronous", format<bool>(msg.asynchronous));
            }
            break;
            
        case DeviceInfo:
            {
                const CUDA_DeviceInfo& msg = 
                    cuda_message.CBTF_cuda_message_u.device_info;
                
                fields = boost::assign::tuple_list_of
                    ("device", format(msg.device))
                    ("name", format(msg.name))
                    ("compute_capability", 
                     format<std::vector<uint32_t> >(
                         boost::assign::list_of
                         (msg.compute_capability[0])
                         (msg.compute_capability[1])
                         ))
                    ("max_grid", 
                     format<std::vector<uint32_t> >(
                         boost::assign::list_of
                         (msg.max_grid[0])
                         (msg.max_grid[1])
                         (msg.max_grid[2])
                         ))
                    ("max_block",
                     format<std::vector<uint32_t> >(
                         boost::assign::list_of
                         (msg.max_block[0])
                         (msg.max_block[1])
                         (msg.max_block[2])
                         ))
                    ("global_memory_bandwidth", 
                     format<ByteCount>(1024ULL * msg.global_memory_bandwidth) +
                     "/Second")
                    ("global_memory_size",
                     format<ByteCount>(msg.global_memory_size))
                    ("constant_memory_size",
                     format<ByteCount>(msg.constant_memory_size))
                    ("l2_cache_size", format<ByteCount>(msg.l2_cache_size))
                    ("threads_per_warp", format(msg.threads_per_warp))
                    ("core_clock_rate",
                     format<ClockRate>(1024ULL * msg.core_clock_rate))
                    ("memcpy_engines", format(msg.memcpy_engines))
                    ("multiprocessors", format(msg.multiprocessors))
                    ("max_ipc", format(msg.max_ipc))
                    ("max_warps_per_multiprocessor",
                     format(msg.max_warps_per_multiprocessor))
                    ("max_blocks_per_multiprocessor",
                     format(msg.max_blocks_per_multiprocessor))
                    ("max_registers_per_block",
                     format(msg.max_registers_per_block))
                    ("max_shared_memory_per_block",
                     format<ByteCount>(msg.max_shared_memory_per_block))
                    ("max_threads_per_block",
                     format(msg.max_threads_per_block));
            }
            break;
            
        case EnqueueRequest:
            {
                const CUDA_EnqueueRequest& msg = 
                    cuda_message.CBTF_cuda_message_u.enqueue_request;
                
                fields = boost::assign::tuple_list_of
                    ("type", format(msg.type))
                    ("time", format(msg.time))
                    ("context", format(msg.context))
                    ("stream", format(msg.stream))
                    ("call_site", format(msg.call_site));
            }
            break;
            
        case ExecutedKernel:
            {
                const CUDA_ExecutedKernel& msg = 
                    cuda_message.CBTF_cuda_message_u.executed_kernel;
                
                fields = boost::assign::tuple_list_of
                    ("context", format(msg.context))
                    ("stream", format(msg.stream))
                    ("time_begin", format(msg.time_begin))
                    ("time_end", format(msg.time_end))
                    ("function", format<FunctionName>(msg.function))
                    ("grid", 
                     format<std::vector<int32_t> >(
                         boost::assign::list_of
                         (msg.grid[0])
                         (msg.grid[1])
                         (msg.grid[2])
                         ))
                    ("block", 
                     format<std::vector<int32_t> >(
                         boost::assign::list_of
                         (msg.block[0])
                         (msg.block[1])
                         (msg.block[2])
                         ))
                    ("cache_preference", format(msg.cache_preference))
                    ("registers_per_thread", format(msg.registers_per_thread))
                    ("static_shared_memory", 
                     format<ByteCount>(msg.static_shared_memory))
                    ("dynamic_shared_memory",
                     format<ByteCount>(msg.dynamic_shared_memory))
                    ("local_memory",
                     format<ByteCount>(msg.local_memory));
            }
            break;
            
        case LoadedModule:
            {
                const CUDA_LoadedModule& msg = 
                    cuda_message.CBTF_cuda_message_u.loaded_module;
                
                fields = boost::assign::tuple_list_of
                    ("time", format(msg.time))
                    ("module", format(msg.module))
                    ("handle", format(msg.handle));
            }
            break;
            
        case ResolvedFunction:
            {
                const CUDA_ResolvedFunction& msg = 
                    cuda_message.CBTF_cuda_message_u.resolved_function;
                
                fields = boost::assign::tuple_list_of
                    ("time", format(msg.time))
                    ("module_handle", format(msg.module_handle))
                    ("function", format<FunctionName>(msg.function))
                    ("handle", format(msg.handle));
            }
            break;
            
        case SetMemory:
            {
                const CUDA_SetMemory& msg = 
                    cuda_message.CBTF_cuda_message_u.set_memory;
                
                fields = boost::assign::tuple_list_of
                    ("context", format(msg.context))
                    ("stream", format(msg.stream))
                    ("time_begin", format(msg.time_begin))
                    ("time_end", format(msg.time_end))
                    ("size", format<ByteCount>(msg.size));
            }
            break;
            
        case UnloadedModule:
            {
                const CUDA_UnloadedModule& msg = 
                    cuda_message.CBTF_cuda_message_u.unloaded_module;
                
                fields = boost::assign::tuple_list_of
                    ("time", format(msg.time))
                    ("handle", format(msg.handle));
            }
            break;
            
        default:
            break;
        }

        std::cout << std::endl
                  << (boost::format("    [%1$3d] %2%") %
                      i % format(static_cast<CUDA_MessageTypes>(
                                     cuda_message.type
                                     ))) << std::endl
                  << format(fields);
    }
    
    for (u_int i = 0; i < data.stack_traces.stack_traces_len; ++i)
    {
        if ((i % 4) == 0)
        {
            std::cout << std::endl << (boost::format("    [%1$4d] ") % i);
        }
        
        std::cout << format(data.stack_traces.stack_traces_val[i]) << " ";
    }

    if ((data.stack_traces.stack_traces_len % 4) != 0)
    {
        std::cout << std::endl;
    }
}
