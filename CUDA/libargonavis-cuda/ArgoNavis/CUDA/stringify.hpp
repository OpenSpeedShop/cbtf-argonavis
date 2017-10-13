////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2017 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration and definition of the stringify() function. */

#pragma once

#include <algorithm>
#include <boost/assign/list_of.hpp>
#include <boost/cstdint.hpp>
#include <boost/format.hpp>
#include <boost/tuple/tuple.hpp>
#include <cmath>
#include <cstdlib>
#include <cxxabi.h>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <KrellInstitute/Messages/DataHeader.h>
#include <KrellInstitute/Messages/CUDA_data.h>

#include <ArgoNavis/CUDA/CachePreference.hpp>
#include <ArgoNavis/CUDA/CopyKind.hpp>
#include <ArgoNavis/CUDA/CounterKind.hpp>
#include <ArgoNavis/CUDA/MemoryKind.hpp>

namespace ArgoNavis { namespace CUDA { namespace Impl {

    /**
     * Implementation of the stringify() function. This template is used to
     * circumvent the C++ prohibition against partial template specialization
     * of functions.
     */
    template <typename T>
    struct Stringify
    {
        static std::string impl(const T& value)
        {
            return boost::str(boost::format("%1%") % value);
        }
    };

} } } // namespace ArgoNavis::CUDA::Impl 

namespace ArgoNavis { namespace CUDA {
    
    /** Convert the specified value to a string. */
    template <typename T>
    std::string stringify(const T& value)
    {
        return Impl::Stringify<T>::impl(value);
    }

    /**
     * Type representing a byte count. Its only reason for existence is to
     * allow the Stringify<> template below to be specialized for byte counts
     * versus other integer values.
     */
    class ByteCount
    {
    public:
        ByteCount(const boost::uint64_t& value) : dm_value(value) { }
        operator boost::uint64_t() const { return dm_value; }
    private:
        boost::uint64_t dm_value;
    };
        
    /**
     * Type representing a clock rate. Its only reason for existence is to
     * allow the Stringify<> template below to be specialized for clock rates
     * versus other integer values.
     */
    class ClockRate
    {
    public:
        ClockRate(const boost::uint64_t& value) : dm_value(value) { }
        operator boost::uint64_t() const { return dm_value; }
    private:
        boost::uint64_t dm_value;
    };

    /**
     * Type representing a (long) hardware performance counter name. Typically
     * the name used by either CUPTI or PAPI. Its only reason for existence is
     * to allow the Stringify<> template below to be specialized for counter
     * names versus other strings.
     *
     * @note    The string returned by stringify<CounterName>() may or may
     *          not be identical to the name returned by CUPTI or PAPI for
     *          their similar counter name query functions.
     *
     * @sa http://docs.nvidia.com/cuda/cupti/r_main.html#r_metric_api
     * @sa http://icl.cs.utk.edu/projects/papi/wiki/PAPI3:PAPI_presets.3
     */
    class LongCounterName
    {
    public:
        LongCounterName(const char* value) : dm_value(value ? value : "") { }
        LongCounterName(const std::string& value) : dm_value(value) { }
        operator std::string() const { return dm_value; }
    private:
        std::string dm_value;
    };

    /**
     * Type representing a (short) hardware performance counter name. Typically
     * the name used by either CUPTI or PAPI. Its only reason for existence is
     * to allow the Stringify<> template below to be specialized for counter
     * names versus other strings.
     *
     * @note    The string returned by stringify<CounterName>() may or may
     *          not be identical to the name returned by CUPTI or PAPI for
     *          their similar counter name query functions.
     *
     * @sa http://docs.nvidia.com/cuda/cupti/r_main.html#r_metric_api
     * @sa http://icl.cs.utk.edu/projects/papi/wiki/PAPI3:PAPI_presets.3
     */
    class ShortCounterName
    {
    public:
        ShortCounterName(const char* value) : dm_value(value ? value : "") { }
        ShortCounterName(const std::string& value) : dm_value(value) { }
        operator std::string() const { return dm_value; }
    private:
        std::string dm_value;
    };

    typedef ShortCounterName CounterName;

    /** Type of container used to store ordered fields (key/value pairs). */
    typedef std::list<boost::tuples::tuple<std::string, std::string> > Fields;
        
    /**
     * Type representing a function name. Its only reason for existence is to
     * allow the Stringify<> template below to be specialized for function names
     * versus other strings.
     */
    class FunctionName
    {
    public:
        FunctionName(const char* value) : dm_value(value ? value : "") { }
        FunctionName(const std::string& value) : dm_value(value) { }
        operator std::string() const { return dm_value; }
    private:
        std::string dm_value;
    };
    
} } // namespace ArgoNavis::CUDA
    
namespace ArgoNavis { namespace CUDA { namespace Impl {

    template <>
    struct Stringify<bool>
    {
        static std::string impl(const bool& value)
        {
            return value ? "true" : "false";
        }
    };
    
    template <>
    struct Stringify<boost::uint64_t>
    {
        static std::string impl(const boost::uint64_t& value)
        {
            return boost::str(boost::format("%016X") % value);
        }
    };
    
    template <typename T>
    struct Stringify<std::vector<T> >
    {
        static std::string impl(const std::vector<T>& value)
        {
            std::stringstream stream;
            
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
    struct Stringify<Fields>
    {
        static std::string impl(const Fields& value)
        {
            std::stringstream stream;

            std::string::size_type n = 0;
            
            for (Fields::const_iterator 
                     i = value.begin(); i != value.end(); ++i)
            {
                n = std::max<std::string::size_type>(n, i->get<0>().size());
            }
            
            for (Fields::const_iterator
                     i = value.begin(); i != value.end(); ++i)
            {
                stream << "    ";
                for (std::string::size_type
                         j = 0; j < (n - i->get<0>().size()); ++j)
                {
                    stream << " ";
                }
                stream << i->get<0>() << " = " << i->get<1>() << std::endl;
            }
            
            return stream.str();
        }
    };

    template <>
    struct Stringify<ByteCount>
    {
        static std::string impl(const ByteCount& value)
        {
            const struct { const double value; const char* label; } kUnits[] = {
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
                (x == std::floor(x)) ?
                (boost::format("%1% %2%") % static_cast<uint64_t>(x) % label) :
                (boost::format("%1$0.1f %2%") % x % label)
                );
        }
    };
    
    template <>
    struct Stringify<ClockRate>
    {
        static std::string impl(const ClockRate& value)
        {
            const struct { const double value; const char* label; } kUnits[] = {
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
                (x == std::floor(x)) ?
                (boost::format("%1% %2%") % static_cast<uint64_t>(x) % label) :
                (boost::format("%1$0.1f %2%") % x % label)
                );
        }
    };

    extern const std::map<std::string, std::string> kLongCounterNames;
    extern const std::map<std::string, std::string> kShortCounterNames;
    
    template <>
    struct Stringify<LongCounterName>
    {
        static std::string impl(const CounterName& value)
        {
            std::map<std::string, std::string>::const_iterator i =
                kLongCounterNames.find(value);

            return (i == kLongCounterNames.end()) ? 
                static_cast<std::string>(value) : i->second;
        }
    };

    template <>
    struct Stringify<ShortCounterName>
    {
        static std::string impl(const CounterName& value)
        {
            std::map<std::string, std::string>::const_iterator i =
                kShortCounterNames.find(value);

            return (i == kShortCounterNames.end()) ? 
                static_cast<std::string>(value) : i->second;
        }
    };

    template <>
    struct Stringify<FunctionName>
    {
        static std::string impl(const FunctionName& value)
        {
            std::string retval = value;
            
            int status = -2;
            char* demangled =  abi::__cxa_demangle(
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
    struct Stringify<CUDA_CachePreference>
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
    struct Stringify<CUDA_CopyKind>
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
    struct Stringify<CUDA_EventKind>
    {
        static std::string impl(const CUDA_EventKind& value)
        {
            switch (value)
            {
            case UnknownEventKind: return "UnknownEventKind";
            case Count: return "Count";
            case Percentage: return "Percentage";
            case Rate: return "Rate";
            }
            return "?";
        }
    };
    
    template <>
    struct Stringify<CUDA_MemoryKind>
    {
        static std::string impl(const CUDA_MemoryKind& value)
        {
            switch (value)
            {
            case InvalidMemoryKind: return "InvalidMemoryKind";
            case UnknownMemoryKind: return "UnknownMemoryKind";
            case Pageable: return "Pageable";
            case Pinned: return "Pinned";
            case ::Device: return "Device";
            case Array: return "Array";
            }
            return "?";
        }
    };

    template <>
    struct Stringify<CUDA_MessageTypes>
    {
        static std::string impl(const CUDA_MessageTypes& value)
        {
            switch (value)
            {
            case CompletedExec: return "CompletedExec";
            case CompletedXfer: return "CompletedXfer";
            case ContextInfo: return "ContextInfo";
            case DeviceInfo: return "DeviceInfo";
            case EnqueueExec: return "EnqueueExec";
            case EnqueueXfer: return "EnqueueXfer";
            case OverflowSamples: return "OverflowSamples";
            case PeriodicSamples: return "PeriodicSamples";
            case SamplingConfig: return "SamplingConfig";
            case ExecClass : return "ExecClass";
            case ExecInstance : return "ExecInstance";
            case XferClass : return "XferClass";
            case XferInstance : return "XferInstance";
            }
            return "?";
        }
    };

    template<>
    struct Stringify<CUDA_EventDescription>
    {
        static std::string impl(const CUDA_EventDescription& value)
        {
            return boost::str((value.threshold == 0) ?
                boost::format("%1% (kind=%2%)") %
                              value.name %
                              stringify(value.kind) :
                boost::format("%1% (kind=%2%, threshold=%3%)") %
                              value.name %
                              stringify(value.kind)
                              % value.threshold
                );
        }
    };

    template <>
    struct Stringify<CUDA_CompletedExec>
    {
        static std::string impl(const CUDA_CompletedExec& value)
        {
            return stringify<Fields>(
                boost::assign::tuple_list_of
                ("id", stringify(value.id))
                ("time_begin", stringify(value.time_begin))
                ("time_end", stringify(value.time_end))
                ("function", stringify<FunctionName>(value.function))
                ("grid", 
                 stringify<std::vector<boost::int32_t> >(
                     boost::assign::list_of
                     (value.grid[0])
                     (value.grid[1])
                     (value.grid[2])
                     ))
                ("block",
                 stringify<std::vector<boost::int32_t> >(
                     boost::assign::list_of
                     (value.block[0])
                     (value.block[1])
                     (value.block[2])
                     ))
                ("cache_preference", stringify(value.cache_preference))
                ("registers_per_thread", stringify(value.registers_per_thread))
                ("static_shared_memory", 
                 stringify<ByteCount>(value.static_shared_memory))
                ("dynamic_shared_memory",
                 stringify<ByteCount>(value.dynamic_shared_memory))
                ("local_memory", stringify<ByteCount>(value.local_memory))
                );
        }
    };

    template <>
    struct Stringify<CUDA_CompletedXfer>
    {
        static std::string impl(const CUDA_CompletedXfer& value)
        {
            return stringify<Fields>(
                boost::assign::tuple_list_of
                ("id", stringify(value.id))
                ("time_begin", stringify(value.time_begin))
                ("time_end", stringify(value.time_end))
                ("size", stringify<ByteCount>(value.size))
                ("kind", stringify(value.kind))
                ("source_kind", stringify(value.source_kind))
                ("destination_kind", stringify(value.destination_kind))
                ("asynchronous", stringify<bool>(value.asynchronous))
                );
        }
    };

    template <>
    struct Stringify<CUDA_ContextInfo>
    {
        static std::string impl(const CUDA_ContextInfo& value)
        {
            return stringify<Fields>(
                boost::assign::tuple_list_of
                ("context", stringify(value.context))
                ("device", stringify(value.device))
                );
        }
    };
    
    template <>
    struct Stringify<CUDA_DeviceInfo>
    {
        static std::string impl(const CUDA_DeviceInfo& value)
        {
            return stringify<Fields>(
                boost::assign::tuple_list_of
                ("device", stringify(value.device))
                ("name", stringify(value.name))
                ("compute_capability",
                 stringify<std::vector<boost::uint32_t> >(
                     boost::assign::list_of
                     (value.compute_capability[0])
                     (value.compute_capability[1])
                     ))
                ("max_grid",
                 stringify<std::vector<boost::uint32_t> >(
                     boost::assign::list_of
                     (value.max_grid[0])
                     (value.max_grid[1])
                     (value.max_grid[2])
                     ))
                ("max_block",
                 stringify<std::vector<boost::uint32_t> >(
                     boost::assign::list_of
                     (value.max_block[0])
                     (value.max_block[1])
                     (value.max_block[2])
                     ))
                ("global_memory_bandwidth", 
                 stringify<ByteCount>(
                     1024ULL * value.global_memory_bandwidth
                     ) + "/Second")
                ("global_memory_size",
                 stringify<ByteCount>(value.global_memory_size))
                ("constant_memory_size",
                 stringify<ByteCount>(value.constant_memory_size))
                ("l2_cache_size", stringify<ByteCount>(value.l2_cache_size))
                ("threads_per_warp", stringify(value.threads_per_warp))
                ("core_clock_rate", 
                 stringify<ClockRate>(1024ULL * value.core_clock_rate))
                ("memcpy_engines", stringify(value.memcpy_engines))
                ("multiprocessors", stringify(value.multiprocessors))
                ("max_ipc", stringify(value.max_ipc))
                ("max_warps_per_multiprocessor",
                 stringify(value.max_warps_per_multiprocessor))
                ("max_blocks_per_multiprocessor",
                 stringify(value.max_blocks_per_multiprocessor))
                ("max_registers_per_block",
                 stringify(value.max_registers_per_block))
                ("max_shared_memory_per_block",
                 stringify<ByteCount>(value.max_shared_memory_per_block))
                ("max_threads_per_block",
                 stringify(value.max_threads_per_block))
                );
        }
    };
    
    template <>
    struct Stringify<CUDA_EnqueueExec>
    {
        static std::string impl(const CUDA_EnqueueExec& value)
        {
            return stringify<Fields>(
                boost::assign::tuple_list_of
                ("id", stringify(value.id))
                ("context", stringify(value.context))
                ("stream", stringify(value.stream))
                ("time", stringify(value.time))
                ("call_site", stringify(value.call_site))
                );
        }
    };

    template <>
    struct Stringify<CUDA_EnqueueXfer>
    {
        static std::string impl(const CUDA_EnqueueXfer& value)
        {
            return stringify<Fields>(
                boost::assign::tuple_list_of
                ("id", stringify(value.id))
                ("context", stringify(value.context))
                ("stream", stringify(value.stream))
                ("time", stringify(value.time))
                ("call_site", stringify(value.call_site))
                );
        }
    };

    template <>
    struct Stringify<CUDA_OverflowSamples>
    {
        static std::string impl(const CUDA_OverflowSamples& value)
        {
            std::stringstream stream;
            
            stream << stringify<Fields>(
                boost::assign::tuple_list_of
                ("time_begin", stringify(value.time_begin))
                ("time_end", stringify(value.time_end))
                );
            
            stream << std::endl << "    pcs = ";
            for (u_int i = 0; i < value.pcs.pcs_len; ++i)
            {
                if ((i % 4) == 0)
                {
                    stream << std::endl << (boost::format("[%1$4d] ") % i);
                }
                
                stream << stringify(value.pcs.pcs_val[i]) << " ";
            }
            if ((value.pcs.pcs_len % 4) != 0)
            {
                stream << std::endl;
            }
            
            stream << std::endl << "    counts = ";
            for (u_int i = 0; i < value.counts.counts_len; ++i)
            {
                if ((i % 4) == 0)
                {
                    stream << std::endl << (boost::format("[%1$4d] ") % i);
                }
                
                stream << stringify(value.counts.counts_val[i]) << " ";
            }
            if ((value.counts.counts_len % 4) != 0)
            {
                stream << std::endl;
            }

            return stream.str();
        }
    };
        
    template <>
    struct Stringify<CUDA_PeriodicSamples>
    {
        static std::string impl(const CUDA_PeriodicSamples& value)
        {
            static int N[4] = { 0, 2, 3, 8 };
            
            std::stringstream stream;
            stream << "    deltas = " << std::endl;
            
            for (u_int i = 0; i < value.deltas.deltas_len;)
            {
                boost::uint8_t* ptr = &value.deltas.deltas_val[i];
                
                boost::uint64_t delta = 0;
                boost::uint8_t encoding = ptr[0] >> 6;
                if (encoding < 3)
                {
                    delta = static_cast<boost::uint64_t>(ptr[0]) & 0x3F;
                }
                else
                {
                    delta = 0;
                }            
                for (int j = 0; j < N[encoding]; ++j)
                {
                    delta <<= 8;
                    delta |= static_cast<boost::uint64_t>(ptr[1 + j]);
                }
                
                std::stringstream bytes;
                for (int j = 0; j < (1 + N[encoding]); ++j)
                {
                    bytes << (boost::format("%02X ") % 
                              static_cast<unsigned int>(ptr[j]));
                }
                
                stream << (boost::format("    [%4d] %-27s (%s)") %
                           i % bytes.str() % stringify(delta)) << std::endl;
                
                i += 1 + N[encoding];
            }
            
            return stream.str();
        }
    };

    template <>
    struct Stringify<CUDA_SamplingConfig>
    {
        static std::string impl(const CUDA_SamplingConfig& value)
        {
            Fields fields = 
                boost::assign::tuple_list_of
                ("interval", stringify(value.interval));
            
            for (u_int i = 0; i < value.events.events_len; ++i)
            {
                fields.push_back(
                    boost::tuples::tuple<std::string, std::string>(
                        boost::str(boost::format("event %1%") % i),
                        stringify(value.events.events_val[i])
                        )
                    );
            }
            
            return stringify(fields);
        }
    };

    template <>
    struct Stringify<CUDA_ExecClass>
    {
        static std::string impl(const CUDA_ExecClass& value)
        {
            return stringify<Fields>(
                boost::assign::tuple_list_of
                ("clas", stringify(value.clas))
                ("context", stringify(value.context))
                ("stream", stringify(value.stream))
                ("call_site", stringify(value.call_site))
                ("function", stringify<FunctionName>(value.function))
                ("grid", 
                 stringify<std::vector<boost::int32_t> >(
                     boost::assign::list_of
                     (value.grid[0])
                     (value.grid[1])
                     (value.grid[2])
                     ))
                ("block",
                 stringify<std::vector<boost::int32_t> >(
                     boost::assign::list_of
                     (value.block[0])
                     (value.block[1])
                     (value.block[2])
                     ))
                ("cache_preference", stringify(value.cache_preference))
                ("registers_per_thread", stringify(value.registers_per_thread))
                ("static_shared_memory", 
                 stringify<ByteCount>(value.static_shared_memory))
                ("dynamic_shared_memory",
                 stringify<ByteCount>(value.dynamic_shared_memory))
                ("local_memory", stringify<ByteCount>(value.local_memory))
                );
        }
    };
    
    template <>
    struct Stringify<CUDA_ExecInstance>
    {
        static std::string impl(const CUDA_ExecInstance& value)
        {
            return stringify<Fields>(
                boost::assign::tuple_list_of
                ("clas", stringify(value.clas))
                ("id", stringify(value.id))
                ("time", stringify(value.time))
                ("time_begin", stringify(value.time_begin))
                ("time_end", stringify(value.time_end))
                );
        }
    };
    
    template <>
    struct Stringify<CUDA_XferClass>
    {
        static std::string impl(const CUDA_XferClass& value)
        {
            return stringify<Fields>(
                boost::assign::tuple_list_of
                ("clas", stringify(value.clas))
                ("context", stringify(value.context))
                ("stream", stringify(value.stream))
                ("call_site", stringify(value.call_site))
                ("size", stringify<ByteCount>(value.size))
                ("kind", stringify(value.kind))
                ("source_kind", stringify(value.source_kind))
                ("destination_kind", stringify(value.destination_kind))
                ("asynchronous", stringify<bool>(value.asynchronous))
                );
        }
    };
    
    template <>
    struct Stringify<CUDA_XferInstance>
    {
        static std::string impl(const CUDA_XferInstance& value)
        {
            return stringify<Fields>(
                boost::assign::tuple_list_of
                ("clas", stringify(value.clas))
                ("id", stringify(value.id))
                ("time", stringify(value.time))
                ("time_begin", stringify(value.time_begin))
                ("time_end", stringify(value.time_end))
                );
        }
    };
    
    template <>
    struct Stringify<CBTF_cuda_message>
    {
        static std::string impl(const CBTF_cuda_message& value)
        {
            switch (value.type)
            {
            case CompletedExec:
                return stringify(value.CBTF_cuda_message_u.completed_exec);
            case CompletedXfer:
                return stringify(value.CBTF_cuda_message_u.completed_xfer);
            case ContextInfo:
                return stringify(value.CBTF_cuda_message_u.context_info);
            case DeviceInfo:
                return stringify(value.CBTF_cuda_message_u.device_info);
            case EnqueueExec:
                return stringify(value.CBTF_cuda_message_u.enqueue_exec);
            case EnqueueXfer:
                return stringify(value.CBTF_cuda_message_u.enqueue_xfer);
            case OverflowSamples:
                return stringify(value.CBTF_cuda_message_u.overflow_samples);
            case PeriodicSamples:
                return stringify(value.CBTF_cuda_message_u.periodic_samples);
            case SamplingConfig:
                return stringify(value.CBTF_cuda_message_u.sampling_config);
            case ExecClass:
                return stringify(value.CBTF_cuda_message_u.exec_class);
            case ExecInstance:
                return stringify(value.CBTF_cuda_message_u.exec_instance);
            case XferClass:
                return stringify(value.CBTF_cuda_message_u.xfer_class);
            case XferInstance:
                return stringify(value.CBTF_cuda_message_u.xfer_instance);
            }
            
            return std::string();
        }
    };
    
    template <>
    struct Stringify<CBTF_cuda_data>
    {
        static std::string impl(const CBTF_cuda_data& value)
        {
            std::stringstream stream;
            
            for (u_int i = 0; i < value.messages.messages_len; ++i)
            {
                const CBTF_cuda_message& msg = value.messages.messages_val[i];
                    
                stream << std::endl
                       << (boost::format("[%1$3d] %2%") % i % 
                           stringify(static_cast<CUDA_MessageTypes>(msg.type)))
                       << std::endl << std::endl << stringify(msg);
            }
            
            stream << std::endl << "stack_traces = ";
            for (u_int i = 0, n = 0;
                 i < value.stack_traces.stack_traces_len;
                 ++i, ++n)
            {
                if (((n % 4) == 0) ||
                    ((i > 0) && 
                     (value.stack_traces.stack_traces_val[i - 1] == 0)))
                {
                    stream << std::endl << (boost::format("[%1$4d] ") % i);
                    n = 0;
                }
                
                stream << stringify(value.stack_traces.stack_traces_val[i])
                       << " ";
            }
            stream << std::endl;
            
            return stream.str();
        }
    };

    template <>
    struct Stringify<CBTF_DataHeader>
    {
        static std::string impl(const CBTF_DataHeader& value)
        {
            return stringify<Fields>(
                boost::assign::tuple_list_of
                ("experiment", stringify(value.experiment))
                ("collector", stringify(value.collector))
                ("id", stringify(value.id))
                ("host", stringify(value.host))
                ("pid", stringify(value.pid))
                ("posix_tid",
                 stringify(static_cast<boost::uint64_t>(value.posix_tid)))
                ("rank", stringify(value.rank))
                ("omp_tid", stringify(value.omp_tid))
                ("time_begin", stringify(value.time_begin))
                ("time_end", stringify(value.time_end))
                ("addr_begin", stringify(value.addr_begin))
                ("addr_end", stringify(value.addr_end))
                );
        }
    };

    template <>
    struct Stringify<CachePreference>
    {
        static std::string impl(const CachePreference& value)
        {
            return stringify(static_cast<CUDA_CachePreference>(value));
        }
    };

    template <>
    struct Stringify<CopyKind>
    {
        static std::string impl(const CopyKind& value)
        {
            return stringify(static_cast<CUDA_CopyKind>(value));
        }
    };

    template <>
    struct Stringify<CounterKind>
    {
        static std::string impl(const CounterKind& value)
        {
            return stringify(static_cast<CUDA_EventKind>(value));
        }
    };

    template <>
    struct Stringify<MemoryKind>
    {
        static std::string impl(const MemoryKind& value)
        {
            return stringify(static_cast<CUDA_MemoryKind>(value));
        }
    };
    
} } } // namespace ArgoNavis::CUDA::Impl 
