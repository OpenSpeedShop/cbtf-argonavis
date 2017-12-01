////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2017 Argo Navis Technologies. All Rights Reserved.
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

#include <boost/algorithm/string.hpp>
#include <boost/cstdint.hpp>
#include <boost/format.hpp>
#include <boost/program_options.hpp>
#include <cstddef>
#include <cuda.h>
#include <cupti.h>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

const std::size_t kStringSize = 1024;
const std::string kTab = "    ";



/**
 * Checks that the given CUDA (Driver API) function call returns the value
 * "CUDA_SUCCESS". If the call was unsuccessful, an exception is thrown.
 *
 * @param x    CUDA function call to be checked.
 */
#define CUDA_CHECK(x)                                                         \
    do {                                                                      \
        CUresult RETVAL = x;                                                  \
        if (RETVAL != CUDA_SUCCESS)                                           \
        {                                                                     \
            std::stringstream stream;                                         \
            const char* description = NULL;                                   \
            if (cuGetErrorString(RETVAL, &description) == CUDA_SUCCESS)       \
            {                                                                 \
                stream << #x << " = " << RETVAL << "(" << description << ")"; \
            }                                                                 \
            else                                                              \
            {                                                                 \
                stream << #x << " = " << RETVAL;                              \
            }                                                                 \
            throw std::runtime_error(stream.str());                           \
        }                                                                     \
    } while (0)



/**
 * Checks that the given CUPT function call returns the value "CUPTI_SUCCESS".
 * If the call was unsuccessful, an exception is thrown.
 *
 * @param x    CUPTI function call to be checked.
 */
#define CUPTI_CHECK(x)                                                        \
    do {                                                                      \
        CUptiResult RETVAL = x;                                               \
        if (RETVAL != CUPTI_SUCCESS)                                          \
        {                                                                     \
            std::stringstream stream;                                         \
            const char* description = NULL;                                   \
            if (cuptiGetResultString(RETVAL, &description) == CUPTI_SUCCESS)  \
            {                                                                 \
                stream << #x << " = " << RETVAL << "(" << description << ")"; \
            }                                                                 \
            else                                                              \
            {                                                                 \
                stream << #x << " = " << RETVAL;                              \
            }                                                                 \
            throw std::runtime_error(stream.str());                           \
        }                                                                     \
    } while (0)



/** Format the specified text as an indented, column-limited, block. */
std::string wrap(const std::string& text, std::size_t tabs, std::size_t columns)
{
    std::string wrapped;

    std::vector<std::string> words;
    boost::split(words, text, boost::is_any_of(" "));

    std::size_t w = 0, n = 0;
    while (w < words.size())
    {
        if (n == 0)
        {
            for (std::size_t t = 0; t < tabs; ++t)
            {
                wrapped += kTab;
                n += kTab.size();
            }

            wrapped += words[w];
            n += words[w].size();
            ++w;
        }
        else if ((n + 1 + words[w].size()) < columns)
        {
            wrapped += " " + words[w];
            n += 1 + words[w].size();
            ++w;
        }
        else
        {
            wrapped += "\n";
            n = 0;
        }
    }

    return wrapped;
}



/** Display CUPTI events for the specified device. */
void displayEvents(CUdevice device, bool details)
{
    std::size_t bytes;

    boost::uint32_t num_domains;
    CUPTI_CHECK(cuptiDeviceGetNumEventDomains(device, &num_domains));
    
    std::vector<CUpti_EventDomainID> domains(num_domains);

    if (num_domains > 0)
    {
        bytes = num_domains * sizeof(CUpti_EventDomainID);
        CUPTI_CHECK(cuptiDeviceEnumEventDomains(device, &bytes, &domains[0]));
    }

    for (boost::uint32_t d = 0; d < num_domains; d++)
    {
        char name[kStringSize];
        bytes = sizeof(name);
        CUPTI_CHECK(cuptiEventDomainGetAttribute(
                        domains[d], CUPTI_EVENT_DOMAIN_ATTR_NAME,
                        &bytes, &name[0]));
        
        std::cout << kTab << "Domain " << d << ": " << name  << " (ID "
                  << domains[d] << ")" << std::endl << std::endl;
        
        if (details)
        {
            boost::uint32_t count;
            
            bytes = sizeof(count);
            CUPTI_CHECK(cuptiDeviceGetEventDomainAttribute(
                            device, domains[d],
                            CUPTI_EVENT_DOMAIN_ATTR_INSTANCE_COUNT,
                            &bytes, &count));
            
            std::cout << kTab << kTab << "Instance Count: " << count
                      << std::endl;

            bytes = sizeof(count);
            CUPTI_CHECK(cuptiDeviceGetEventDomainAttribute(
                            device, domains[d],
                            CUPTI_EVENT_DOMAIN_ATTR_TOTAL_INSTANCE_COUNT,
                            &bytes, &count));
            
            std::cout << kTab << kTab << "Total Instance Count: " << count
                      << std::endl;
            
            CUpti_EventCollectionMethod method;
            bytes = sizeof(method);
            CUPTI_CHECK(cuptiDeviceGetEventDomainAttribute(
                            device, domains[d],
                            CUPTI_EVENT_DOMAIN_ATTR_COLLECTION_METHOD,
                            &bytes, &method));
            
            std::cout << kTab << kTab << "Event Collection Method: ";
            switch (method)
            {
            case CUPTI_EVENT_COLLECTION_METHOD_PM:
                std::cout << "Hardware Global Performance Monitor"; break;
            case CUPTI_EVENT_COLLECTION_METHOD_SM:
                std::cout << "Hardware SM Performance Monitor"; break;
            case CUPTI_EVENT_COLLECTION_METHOD_INSTRUMENTED:
                std::cout << "Software Instrumentation"; break;
            default: std::cout << "?";
            }
            std::cout << std::endl;
            
            std::cout << std::endl;
        }
        
        boost::uint32_t num_events;
        CUPTI_CHECK(cuptiEventDomainGetNumEvents(
                        domains[d], &num_events
                        ));
        
        std::vector<CUpti_EventID> events(num_events);
        
        if (num_events > 0)
        {
            bytes = num_events * sizeof(CUpti_EventID);
            CUPTI_CHECK(cuptiEventDomainEnumEvents(
                            domains[d], &bytes, &events[0]
                            ));
        }
        
        for (boost::uint32_t e = 0; e < num_events; ++e)
        {
            char name[kStringSize];
            bytes = sizeof(name);
            CUPTI_CHECK(cuptiEventGetAttribute(
                            events[e], CUPTI_EVENT_ATTR_NAME, &bytes, &name[0]
                            ));

            std::cout << kTab << kTab << "Event " << e << ": " << name
                      << " (ID " << events[e] << ")" << std::endl;

            if (details)
            {
                char description[kStringSize];
                
                bytes = sizeof(description);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_SHORT_DESCRIPTION,
                                &bytes, &description
                                ));
                
                std::cout << std::endl << kTab << kTab << kTab
                          << "Short Description: " << description << std::endl;
            
                bytes = sizeof(description);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_LONG_DESCRIPTION,
                                &bytes, &description
                                ));
                
                std::cout << std::endl << kTab << kTab << kTab
                          << "Long Description: " << std::endl << std::endl
                          << wrap(description, 4, 75) << std::endl;

                CUpti_EventCategory category;
                bytes = sizeof(category);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_CATEGORY,
                                &bytes, &category
                                ));
                
                std::cout << std::endl << kTab << kTab << kTab << "Category: ";
                switch (category)
                {
                case CUPTI_EVENT_CATEGORY_INSTRUCTION:
                    std::cout << "Instruction"; break;
                case CUPTI_EVENT_CATEGORY_MEMORY:
                    std::cout << "Memory"; break;
                case CUPTI_EVENT_CATEGORY_CACHE:
                    std::cout << "Cache"; break;
                case CUPTI_EVENT_CATEGORY_PROFILE_TRIGGER:
                    std::cout << "Profile Trigger"; break;
                default: std::cout << "?";
                }
                std::cout << std::endl;
                
                std::cout << std::endl;
            }
        }
        
        if (!details)
        {
            std::cout << std::endl;
        }
    }
}



/** Display CUPTI metrics for the specified device. */
void displayMetrics(CUdevice device, bool details)
{
    std::size_t bytes;
    
    boost::uint32_t num_metrics;
    CUPTI_CHECK(cuptiDeviceGetNumMetrics(device, &num_metrics));
    
    std::vector<CUpti_MetricID> metrics(num_metrics);
    
    if (num_metrics > 0)
    {
        bytes = num_metrics * sizeof(CUpti_MetricID);
        CUPTI_CHECK(cuptiDeviceEnumMetrics(device, &bytes, &metrics[0]));
    }
    
    for (boost::uint32_t m = 0; m < num_metrics; m++)
    {
        char name[kStringSize];
        bytes = sizeof(name);
        CUPTI_CHECK(cuptiMetricGetAttribute(
                        metrics[m], CUPTI_METRIC_ATTR_NAME, &bytes, &name[0]
                        ));
        
        std::cout << kTab << "Metric " << m << ": " << name  << " (ID "
                  << metrics[m] << ")" << std::endl;
        
        if (details)
        {
            char description[kStringSize];
            
            bytes = sizeof(description);
            CUPTI_CHECK(cuptiMetricGetAttribute(
                            metrics[m], CUPTI_METRIC_ATTR_SHORT_DESCRIPTION,
                            &bytes, &description
                            ));
            
            std::cout << std::endl << kTab << kTab << "Short Description: "
                      << description << std::endl;
            
            bytes = sizeof(description);
            CUPTI_CHECK(cuptiMetricGetAttribute(
                            metrics[m], CUPTI_METRIC_ATTR_LONG_DESCRIPTION,
                            &bytes, &description
                            ));
            
            std::cout << std::endl << kTab << kTab << "Long Description: "
                      << std::endl << std::endl << wrap(description, 3, 75)
                      << std::endl;

            CUpti_MetricCategory category;
            bytes = sizeof(category);
            CUPTI_CHECK(cuptiMetricGetAttribute(
                            metrics[m], CUPTI_METRIC_ATTR_CATEGORY,
                            &bytes, &category
                            ));
            
            std::cout << std::endl << kTab << kTab << "Category: ";
            switch (category)
            {
            case CUPTI_METRIC_CATEGORY_MEMORY:
                std::cout << "Memory"; break;
            case CUPTI_METRIC_CATEGORY_INSTRUCTION:
                std::cout << "Instruction"; break;
            case CUPTI_METRIC_CATEGORY_MULTIPROCESSOR:
                std::cout << "Multiprocessor"; break;
            case CUPTI_METRIC_CATEGORY_CACHE:
                std::cout << "Cache"; break;
            case CUPTI_METRIC_CATEGORY_TEXTURE:
                std::cout << "Texture"; break;
            default: std::cout << "?";
            }
            std::cout << std::endl;
            
            CUpti_MetricValueKind kind;
            bytes = sizeof(kind);
            CUPTI_CHECK(cuptiMetricGetAttribute(
                            metrics[m], CUPTI_METRIC_ATTR_VALUE_KIND,
                            &bytes, &kind
                            ));
            
            std::cout << std::endl << kTab << kTab << "Value Kind: ";
            switch (category)
            {
            case CUPTI_METRIC_VALUE_KIND_DOUBLE:
                std::cout << "Double"; break;
            case CUPTI_METRIC_VALUE_KIND_UINT64:
                std::cout << "UInt64"; break;
            case CUPTI_METRIC_VALUE_KIND_PERCENT:
                std::cout << "Percent"; break;
            case CUPTI_METRIC_VALUE_KIND_THROUGHPUT:
                std::cout << "Throughput"; break;
            case CUPTI_METRIC_VALUE_KIND_INT64:
                std::cout << "Int64"; break;
            case CUPTI_METRIC_VALUE_KIND_UTILIZATION_LEVEL:
                std::cout << "Utilization Level"; break;
            default: std::cout << "?";
            }
            std::cout << std::endl;
            
            CUpti_MetricEvaluationMode mode;
            bytes = sizeof(mode);
            CUPTI_CHECK(cuptiMetricGetAttribute(
                            metrics[m], CUPTI_METRIC_ATTR_EVALUATION_MODE,
                            &bytes, &mode
                            ));
            
            std::cout << std::endl << kTab << kTab << "Evaluation Mode: ";
            switch (mode)
            {
            case CUPTI_METRIC_EVALUATION_MODE_PER_INSTANCE:
                std::cout << "Per Instance"; break;
            case CUPTI_METRIC_EVALUATION_MODE_AGGREGATE:
                std::cout << "Aggregate"; break;
            default: std::cout << "?";
            }
            std::cout << std::endl;
            
            boost::uint32_t num_events;
            CUPTI_CHECK(cuptiMetricGetNumEvents(metrics[m], &num_events));
            
            std::vector<CUpti_EventID> events(num_events);
            
            if (num_events > 0)
            {
                bytes = num_events * sizeof(CUpti_EventID);
                CUPTI_CHECK(cuptiMetricEnumEvents(
                                metrics[m], &bytes, &events[0]
                                ));
                
                std::cout << std::endl;
            }
            
            for (boost::uint32_t e = 0; e < num_events; ++e)
            {
                char name[kStringSize];
                bytes = sizeof(name);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_NAME,
                                &bytes, &name[0]
                                ));
                
                std::cout << kTab << kTab << "Event " << e << ": " << name 
                          << " (ID " << events[e] << ")" << std::endl;
            }
            
            boost::uint32_t num_properties;
            CUPTI_CHECK(cuptiMetricGetNumProperties(
                            metrics[m], &num_properties
                            ));
            
            std::vector<CUpti_MetricPropertyID> properties(num_properties);
            
            if (num_properties > 0)
            {
                bytes = num_properties * sizeof(CUpti_MetricPropertyID);
                CUPTI_CHECK(cuptiMetricEnumProperties(
                                metrics[m], &bytes, &properties[0]
                                ));
                
                std::cout << std::endl;
            }
            
            for (boost::uint32_t p = 0; p < num_properties; ++p)
            {
                std::cout << kTab << kTab << "Property " << p << ": ";
                
                switch (properties[p])
                {
                case CUPTI_METRIC_PROPERTY_MULTIPROCESSOR_COUNT:
                    std::cout << "Multiprocessor Count"; break;
                case CUPTI_METRIC_PROPERTY_WARPS_PER_MULTIPROCESSOR:
                    std::cout << "Warps/Multiprocessor"; break;
                case CUPTI_METRIC_PROPERTY_KERNEL_GPU_TIME:
                    std::cout << "Kernel GPU Time"; break;
                case CUPTI_METRIC_PROPERTY_CLOCK_RATE:
                    std::cout << "Clock Rate"; break;
                case CUPTI_METRIC_PROPERTY_FRAME_BUFFER_COUNT:
                    std::cout << "Frame Buffer Count"; break;
                case CUPTI_METRIC_PROPERTY_GLOBAL_MEMORY_BANDWIDTH:
                    std::cout << "Global Memory Bandwidth"; break;
                case CUPTI_METRIC_PROPERTY_PCIE_LINK_RATE:
                    std::cout << "PCIe Link Rate"; break;
                case CUPTI_METRIC_PROPERTY_PCIE_LINK_WIDTH:
                    std::cout << "PCIe Link Width"; break;
                case CUPTI_METRIC_PROPERTY_PCIE_GEN:
                    std::cout << "PCIe Generation"; break;
                case CUPTI_METRIC_PROPERTY_DEVICE_CLASS:
                    std::cout << "Device Class"; break;
                    
#if (CUPTI_API_VERSION >= 6)
                case CUPTI_METRIC_PROPERTY_FLOP_SP_PER_CYCLE:
                    std::cout << "Single-Precision FLOPS/Cycle"; break;
                case CUPTI_METRIC_PROPERTY_FLOP_DP_PER_CYCLE:
                    std::cout << "Double-Precision FLOPS/Cycle"; break;
                case CUPTI_METRIC_PROPERTY_L2_UNITS:
                    std::cout << "L2 Unit Count"; break;
#endif
                    
#if (CUPTI_API_VERSION >= 8)
                case CUPTI_METRIC_PROPERTY_ECC_ENABLED:
                    std::cout << "ECC Enabled"; break;
#endif
                    
                default: std::cout << "?";
                }
                std::cout << std::endl;
            }
            
            std::cout << std::endl;
        }
    }
}



/**
 * Display the number of event data collection passes required to
 * compute all of the given CUPTI metrics for the specified device.
 */
void displayPasses(int d, CUdevice device,
                   const std::set<boost::uint32_t>& metrics)
{
    std::size_t bytes;
    
    std::cout << "On CUDA Device " << d
              << " computing the following CUPTI Metrics:" << std::endl
              << std::endl;
    
    std::vector<CUpti_MetricID> ids;
    
    boost::uint32_t m = 0;
    for (std::set<boost::uint32_t>::const_iterator
             i = metrics.begin(); i != metrics.end(); ++i, ++m)
    {
        CUpti_MetricID id = *i;
        ids.push_back(id);
        
        char name[kStringSize];
        bytes = sizeof(name);
        CUPTI_CHECK(cuptiMetricGetAttribute(
                        id, CUPTI_METRIC_ATTR_NAME, &bytes, &name[0]
                        ));
        
        std::cout << kTab << "Metric " << m << ": " << name << " (ID " << *i
                  << ")" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "requires the following event collection passes:" << std::endl;
    
    CUcontext context;
    CUDA_CHECK(cuCtxCreate(&context, 0, device));
    
    CUpti_EventGroupSets* passes;
    CUPTI_CHECK(cuptiMetricCreateEventGroupSets(
                    context, ids.size() * sizeof(CUpti_MetricID),
                    &ids[0], &passes)); 
    
    for (boost::uint32_t p = 0; p < passes->numSets; ++p)
    {
        std::cout << std::endl << kTab << "Pass " << p << std::endl;
        
        for (boost::uint32_t g = 0; g < passes->sets[p].numEventGroups; ++g)
        {
            std::cout << std::endl << kTab << kTab << "Event Group " << g
                      << std::endl;

            CUpti_EventGroup group = passes->sets[p].eventGroups[g];
            
            boost::uint32_t num_events;
            bytes = sizeof(num_events);
            CUPTI_CHECK(cuptiEventGroupGetAttribute(
                            group, CUPTI_EVENT_GROUP_ATTR_NUM_EVENTS,
                            &bytes, &num_events
                            ));
            
            std::vector<CUpti_EventID> events(num_events);
            
            if (num_events > 0)
            {
                bytes = num_events * sizeof(CUpti_EventID);
                CUPTI_CHECK(cuptiEventGroupGetAttribute(
                                group, CUPTI_EVENT_GROUP_ATTR_EVENTS,
                                &bytes, &events[0]
                                ));
                
                std::cout << std::endl;
            }
            
            for (boost::uint32_t e = 0; e < num_events; ++e)
            {
                char name[kStringSize];
                bytes = sizeof(name);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_NAME,
                                &bytes, &name[0]
                                ));
                
                std::cout << kTab << kTab << kTab << "Event " << e << ": "
                          << name << " (ID " << events[e] << ")" << std::endl;
            }
        }
    }
    
    CUPTI_CHECK(cuptiEventGroupSetsDestroy(passes));
    CUDA_CHECK(cuCtxDestroy(context));
}



/**
 * Parse the command-line arguments and dump the requested CUPTI event and/or
 * metrics information.
 *
 * @param argc    Number of command-line arguments.
 * @param argv    Command-line arguments.
 * @return        Exit code. Either 1 if a failure occurred, or 0 otherwise.
 */
int main(int argc, char* argv[])
{
    std::stringstream stream;
    stream << std::endl << "This tool shows the available CUDA devices and "
           << "information about the CUPTI" << std::endl << "events and "
           << "metrics supported by each device. It can also determine how "
           << "many" << std::endl << "data collection passes are required to "
           << "gather a given set of CUPTI metrics." << std::endl
           << std::endl;

    std::string kExtraHelp = stream.str();

    // Parse and validate the command-line arguments

    boost::program_options::options_description kNonPositionalOptions(
        "cupti_avail options"
        );
    
    kNonPositionalOptions.add_options()
        
        ("details",
         boost::program_options::bool_switch()->default_value(false),
         "Display detailed information about the available CUPTI events "
         "and/or metrics.")
        
        ("device",
         boost::program_options::value< std::vector<int> >(),
         "Restrict display to the CUDA device with this index. Multiple "
         "indicies may be specified. The default is to display for all "
         "devices.")
        
        ("events",
         boost::program_options::bool_switch()->default_value(false),
         "Display the available CUPTI events.")
        
        ("metrics",
         boost::program_options::bool_switch()->default_value(false),
         "Display the available CUPTI metrics.")
        
        ("passes",
         boost::program_options::value< std::vector<boost::uint32_t> >(),
         "Determine the number of event data collection passes required "
         "to compute all of the specified (by ID) CUPTI metrics. Multiple "
         "indicies may be specified.")
        
        ("help", "Display this help message.")
        
        ;
    
    boost::program_options::positional_options_description kPositionalOptions;

    kPositionalOptions.add("database", 1);
    
    boost::program_options::variables_map values;
    
    try
    {
        boost::program_options::store(
            boost::program_options::command_line_parser(argc, argv).
            options(kNonPositionalOptions).run(), values
            );       
        boost::program_options::notify(values);
    }
    catch (const std::exception& error)
    {
        std::cout << std::endl << "ERROR: " << error.what() << std::endl 
                  << std::endl << kNonPositionalOptions << kExtraHelp;
        return 1;
    }
    
    if (values.count("help") > 0)
    {
        std::cout << std::endl << kNonPositionalOptions << kExtraHelp;
        return 1;
    }
    
    std::set<int> indicies;
    if (values.count("device") > 0)
    {
        std::vector<int> temp = values["device"].as< std::vector<int> >();
        indicies = std::set<int>(temp.begin(), temp.end());
    }
    
    std::set<boost::uint32_t> metrics;
    if (values.count("passes") > 0)
    {
        std::vector<boost::uint32_t> temp =
            values["passes"].as< std::vector<boost::uint32_t> >();
        metrics = std::set<boost::uint32_t>(temp.begin(), temp.end());
    }
    
    // Display the requested information
    
    CUDA_CHECK(cuInit(0));
    
    int num_devices;
    CUDA_CHECK(cuDeviceGetCount(&num_devices));
    
    if (num_devices == 0)
    {
        std::cout << std::endl << "ERROR: There are no devices supporting CUDA."
                  << std::endl;
        return 1;
    }
    
    std::vector<CUdevice> devices;
    
    for (int d = 0; d < num_devices; ++d)
    {
        CUdevice device;
        CUDA_CHECK(cuDeviceGet(&device, d));
        devices.push_back(device);
    }
    
    std::cout << std::endl << "Devices" << std::endl << std::endl;
    
    for (int d = 0; d < num_devices; ++d)
    {
        char name[kStringSize];
        CUDA_CHECK(cuDeviceGetName(name, sizeof(name), devices[d]));
        std::cout << kTab << d << ": " << name << std::endl;
    }
    
    for (int d = 0; d < num_devices; ++d)
    {
        if (indicies.empty() || (indicies.find(d) != indicies.end()))
        {
            if (values["events"].as<bool>())
            {
                std::cout << std::endl << "CUPTI Events for CUDA Device " << d
                          << std::endl << std::endl;

                displayEvents(devices[d], values["details"].as<bool>());
            }

            if (values["metrics"].as<bool>())
            {
                std::cout << std::endl << "CUPTI Metrics for CUDA Device " << d
                          << std::endl << std::endl;
                
                displayMetrics(devices[d], values["details"].as<bool>());
            }
        }
        if (!metrics.empty())
        {
            std::cout << std::endl;
            displayPasses(d, devices[d], metrics);
        }
    }
    
    std::cout << std::endl;
    return 0;
}
