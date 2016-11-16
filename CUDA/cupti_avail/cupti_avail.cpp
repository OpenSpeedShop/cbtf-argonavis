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

#include <boost/algorithm/string.hpp>
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

using namespace boost;
using namespace std;

const std::size_t kStringSize = 1024;
const std::string kTab = "  ";



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
            stringstream stream;                                              \
            const char* description = NULL;                                   \
            if (cuGetErrorString(RETVAL, &description) == CUDA_SUCCESS)       \
            {                                                                 \
                stream << #x << " = " << RETVAL << "(" << description << ")"; \
            }                                                                 \
            else                                                              \
            {                                                                 \
                stream << #x << " = " << RETVAL;                              \
            }                                                                 \
            throw runtime_error(stream.str());                                \
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
            stringstream stream;                                              \
            const char* description = NULL;                                   \
            if (cuptiGetResultString(RETVAL, &description) == CUPTI_SUCCESS)  \
            {                                                                 \
                stream << #x << " = " << RETVAL << "(" << description << ")"; \
            }                                                                 \
            else                                                              \
            {                                                                 \
                stream << #x << " = " << RETVAL;                              \
            }                                                                 \
            throw runtime_error(stream.str());                                \
        }                                                                     \
    } while (0)



/** Format the specified text as an indented, column-limited, block. */
string wrap(const string& text, size_t tabs, size_t columns)
{
    string wrapped;

    vector<string> words;
    split(words, text, is_any_of(" "));

    size_t w = 0, n = 0;
    while (w < words.size())
    {
        if (n == 0)
        {
            for (size_t t = 0; t < tabs; ++t)
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
    size_t bytes;

    uint32_t num_domains;
    CUPTI_CHECK(cuptiDeviceGetNumEventDomains(device, &num_domains));

    vector<CUpti_EventDomainID> domains(num_domains);

    if (num_domains > 0)
    {
        bytes = num_domains * sizeof(CUpti_EventDomainID);
        CUPTI_CHECK(cuptiDeviceEnumEventDomains(device, &bytes, &domains[0]));
    }

    for (uint32_t d = 0; d < num_domains; d++)
    {
        char name[kStringSize];
        bytes = sizeof(name);
        CUPTI_CHECK(cuptiEventDomainGetAttribute(
                        domains[d], CUPTI_EVENT_DOMAIN_ATTR_NAME,
                        &bytes, &name[0]));
        
        cout << endl << kTab << "Domain " << d << ": " << name << endl;
        
        if (details)
        {
            cout << endl;

            uint32_t count;

            bytes = sizeof(count);
            CUPTI_CHECK(cuptiDeviceGetEventDomainAttribute(
                            device, domains[d],
                            CUPTI_EVENT_DOMAIN_ATTR_INSTANCE_COUNT,
                            &bytes, &count));

            cout << kTab << kTab << "Instance Count: " << count << endl;

            bytes = sizeof(count);
            CUPTI_CHECK(cuptiDeviceGetEventDomainAttribute(
                            device, domains[d],
                            CUPTI_EVENT_DOMAIN_ATTR_TOTAL_INSTANCE_COUNT,
                            &bytes, &count));
            
            cout << kTab << kTab << "Total Instance Count: " << count << endl;
            
            CUpti_EventCollectionMethod method;
            bytes = sizeof(method);
            CUPTI_CHECK(cuptiDeviceGetEventDomainAttribute(
                            device, domains[d],
                            CUPTI_EVENT_DOMAIN_ATTR_COLLECTION_METHOD,
                            &bytes, &method));

            cout << kTab << kTab << "Event Collection Method: ";
            switch (method)
            {
            case CUPTI_EVENT_COLLECTION_METHOD_PM:
                cout << "Hardware Global Performance Monitor"; break;
            case CUPTI_EVENT_COLLECTION_METHOD_SM:
                cout << "Hardware SM Performance Monitor"; break;
            case CUPTI_EVENT_COLLECTION_METHOD_INSTRUMENTED:
                cout << "Software Instrumentation"; break;
            default: cout << "?";
            }
            cout << endl;

            cout << endl;
        }

        uint32_t num_events;
        CUPTI_CHECK(cuptiEventDomainGetNumEvents(
                        domains[d], &num_events
                        ));

        vector<CUpti_EventID> events(num_events);

        if (num_events > 0)
        {
            bytes = num_events * sizeof(CUpti_EventID);
            CUPTI_CHECK(cuptiEventDomainEnumEvents(
                            domains[d], &bytes, &events[0]
                            ));
        }

        for (uint32_t e = 0; e < num_events; ++e)
        {
            char name[kStringSize];
            bytes = sizeof(name);
            CUPTI_CHECK(cuptiEventGetAttribute(
                            events[e], CUPTI_EVENT_ATTR_NAME, &bytes, &name[0]
                            ));

            cout << kTab << kTab << "Event " << e << ": " << name << endl;

            if (details)
            {
                char description[kStringSize];

                bytes = sizeof(description);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_SHORT_DESCRIPTION,
                                &bytes, &description
                                ));
                
                cout << endl 
                     << kTab << kTab << kTab << "Short Description: "
                     << description << endl;
            
                bytes = sizeof(description);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_LONG_DESCRIPTION,
                                &bytes, &description
                                ));
                
                cout << endl 
                     << kTab << kTab << kTab << "Long Description: " << endl
                     << endl
                     << wrap(description, 4, 75) << endl;

                CUpti_EventCategory category;
                bytes = sizeof(category);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_CATEGORY,
                                &bytes, &category
                                ));
                
                cout << endl
                     << kTab << kTab << kTab << "Category: ";
                switch (category)
                {
                case CUPTI_EVENT_CATEGORY_INSTRUCTION:
                    cout << "Instruction"; break;
                case CUPTI_EVENT_CATEGORY_MEMORY:
                    cout << "Memory"; break;
                case CUPTI_EVENT_CATEGORY_CACHE:
                    cout << "Cache"; break;
                case CUPTI_EVENT_CATEGORY_PROFILE_TRIGGER:
                    cout << "Profile Trigger"; break;
                default: cout << "?";
                }
                cout << endl;
                
                cout << endl;
            }
        }
    }
}



/** Display CUPTI metrics for the specified device. */
void displayMetrics(CUdevice device, bool details)
{
    size_t bytes;

    uint32_t num_metrics;
    CUPTI_CHECK(cuptiDeviceGetNumMetrics(device, &num_metrics));

    vector<CUpti_MetricID> metrics(num_metrics);

    if (num_metrics > 0)
    {
        bytes = num_metrics * sizeof(CUpti_MetricID);
        CUPTI_CHECK(cuptiDeviceEnumMetrics(device, &bytes, &metrics[0]));
    }
    
    for (uint32_t m = 0; m < num_metrics; m++)
    {
        char name[kStringSize];
        bytes = sizeof(name);
        CUPTI_CHECK(cuptiMetricGetAttribute(
                        metrics[m], CUPTI_METRIC_ATTR_NAME, &bytes, &name[0]
                        ));
        
        cout << kTab << "Metric " << m << ": " << name << endl;
        
        if (details)
        {
            char description[kStringSize];
            
            bytes = sizeof(description);
            CUPTI_CHECK(cuptiMetricGetAttribute(
                            metrics[m], CUPTI_METRIC_ATTR_SHORT_DESCRIPTION,
                            &bytes, &description
                            ));
            
            cout << endl
                 << kTab << kTab << "Short Description: "
                 << description << endl;
            
            bytes = sizeof(description);
            CUPTI_CHECK(cuptiMetricGetAttribute(
                            metrics[m], CUPTI_METRIC_ATTR_LONG_DESCRIPTION,
                            &bytes, &description
                            ));

            cout << endl 
                 << kTab << kTab << "Long Description: " << endl
                 << endl
                 << wrap(description, 3, 75) << endl;

            CUpti_MetricCategory category;
            bytes = sizeof(category);
            CUPTI_CHECK(cuptiMetricGetAttribute(
                            metrics[m], CUPTI_METRIC_ATTR_CATEGORY,
                            &bytes, &category
                            ));
            
            cout << endl
                 << kTab << kTab << "Category: ";
            switch (category)
            {
            case CUPTI_METRIC_CATEGORY_MEMORY:
                cout << "Memory"; break;
            case CUPTI_METRIC_CATEGORY_INSTRUCTION:
                cout << "Instruction"; break;
            case CUPTI_METRIC_CATEGORY_MULTIPROCESSOR:
                cout << "Multiprocessor"; break;
            case CUPTI_METRIC_CATEGORY_CACHE:
                cout << "Cache"; break;
            case CUPTI_METRIC_CATEGORY_TEXTURE:
                cout << "Texture"; break;
            default: cout << "?";
            }
            cout << endl;

            CUpti_MetricValueKind kind;
            bytes = sizeof(kind);
            CUPTI_CHECK(cuptiMetricGetAttribute(
                            metrics[m], CUPTI_METRIC_ATTR_VALUE_KIND,
                            &bytes, &kind
                            ));
            
            cout << endl
                 << kTab << kTab << "Value Kind: ";
            switch (category)
            {
            case CUPTI_METRIC_VALUE_KIND_DOUBLE:
                cout << "Double"; break;
            case CUPTI_METRIC_VALUE_KIND_UINT64:
                cout << "UInt64"; break;
            case CUPTI_METRIC_VALUE_KIND_PERCENT:
                cout << "Percent"; break;
            case CUPTI_METRIC_VALUE_KIND_THROUGHPUT:
                cout << "Throughput"; break;
            case CUPTI_METRIC_VALUE_KIND_INT64:
                cout << "Int64"; break;
            case CUPTI_METRIC_VALUE_KIND_UTILIZATION_LEVEL:
                cout << "Utilization Level"; break;
            default: cout << "?";
            }
            cout << endl;
                
            CUpti_MetricEvaluationMode mode;
            bytes = sizeof(mode);
            CUPTI_CHECK(cuptiMetricGetAttribute(
                            metrics[m], CUPTI_METRIC_ATTR_EVALUATION_MODE,
                            &bytes, &mode
                            ));

            cout << endl
                 << kTab << kTab << "Evaluation Mode: ";
            switch (mode)
            {
            case CUPTI_METRIC_EVALUATION_MODE_PER_INSTANCE:
                cout << "Per Instance"; break;
            case CUPTI_METRIC_EVALUATION_MODE_AGGREGATE:
                cout << "Aggregate"; break;
            default: cout << "?";
            }
            cout << endl;
            
            uint32_t num_events;
            CUPTI_CHECK(cuptiMetricGetNumEvents(metrics[m], &num_events));
            
            vector<CUpti_EventID> events(num_events);

            if (num_events > 0)
            {
                bytes = num_events * sizeof(CUpti_EventID);
                CUPTI_CHECK(cuptiMetricEnumEvents(
                                metrics[m], &bytes, &events[0]
                                ));
            }
            else
            {            
                cout << endl;
            }

            for (uint32_t e = 0; e < num_events; ++e)
            {
                char name[kStringSize];
                bytes = sizeof(name);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_NAME,
                                &bytes, &name[0]
                                ));
                
                cout << kTab << kTab << "Event " << e << ": " << name << endl;
            }

            uint32_t num_properties;
            CUPTI_CHECK(cuptiMetricGetNumProperties(
                            metrics[m], &num_properties
                            ));

            vector<CUpti_MetricPropertyID> properties(num_properties);

            if (num_properties > 0)
            {
                bytes = num_properties * sizeof(CUpti_MetricPropertyID);
                CUPTI_CHECK(cuptiMetricEnumProperties(
                                metrics[m], &bytes, &properties[0]
                                ));
            }
            else
            {
                cout << endl;
            }

            for (uint32_t p = 0; p < num_properties; ++p)
            {
                cout << kTab << kTab << "Property " << p << ": ";

                switch (properties[p])
                {
                case CUPTI_METRIC_PROPERTY_MULTIPROCESSOR_COUNT:
                    cout << "Multiprocessor Count" << endl; break;
                case CUPTI_METRIC_PROPERTY_WARPS_PER_MULTIPROCESSOR:
                    cout << "Warps/Multiprocessor" << endl; break;
                case CUPTI_METRIC_PROPERTY_KERNEL_GPU_TIME:
                    cout << "Kernel GPU Time" << endl; break;
                case CUPTI_METRIC_PROPERTY_CLOCK_RATE:
                    cout << "Clock Rate" << endl; break;
                case CUPTI_METRIC_PROPERTY_FRAME_BUFFER_COUNT:
                    cout << "Frame Buffer Count" << endl; break;
                case CUPTI_METRIC_PROPERTY_GLOBAL_MEMORY_BANDWIDTH:
                    cout << "Global Memory Bandwidth" << endl; break;
                case CUPTI_METRIC_PROPERTY_PCIE_LINK_RATE:
                    cout << "PCIe Link Rate" << endl; break;
                case CUPTI_METRIC_PROPERTY_PCIE_LINK_WIDTH:
                    cout << "PCIe Link Width" << endl; break;
                case CUPTI_METRIC_PROPERTY_PCIE_GEN:
                    cout << "PCIe Generation" << endl; break;
                case CUPTI_METRIC_PROPERTY_DEVICE_CLASS:
                    cout << "Device Class" << endl; break;
                case CUPTI_METRIC_PROPERTY_FLOP_SP_PER_CYCLE:
                    cout << "Single-Precision FLOPS/Cycle" << endl; break;
                case CUPTI_METRIC_PROPERTY_FLOP_DP_PER_CYCLE:
                    cout << "Double-Precision FLOPS/Cycle" << endl; break;
                case CUPTI_METRIC_PROPERTY_L2_UNITS:
                    cout << "L2 Unit Count" << endl; break;
                case CUPTI_METRIC_PROPERTY_ECC_ENABLED:
                    cout << "ECC Enabled" << endl; break;
                default: cout << "?";
                }
                cout << endl;
            }
            
            cout << endl;
        }
    }
}



/**
 * Display the number of event data collection passes required to
 * compute all of the given CUPTI metrics for the specified device.
 */
void displayPasses(int d, CUdevice device, const set<uint32_t>& metrics)
{
    size_t bytes;

    cout << "On CUDA Device " << d << " computing the following CUPTI Metrics:"
         << endl;
    cout << endl;

    vector<CUpti_MetricID> ids;

    uint32_t m = 0;
    for (set<uint32_t>::const_iterator
             i = metrics.begin(); i != metrics.end(); ++i, ++m)
    {
        CUpti_MetricID id = *i;
        ids.push_back(id);

        char name[kStringSize];
        bytes = sizeof(name);
        CUPTI_CHECK(cuptiMetricGetAttribute(
                        id, CUPTI_METRIC_ATTR_NAME, &bytes, &name[0]
                        ));
        
        cout << kTab << "Metric " << m << ": " << name << endl;
    }

    cout << endl;
    cout << "requires the following event collection passes:" << endl;
    cout << endl;

    CUcontext context;
    CUDA_CHECK(cuCtxCreate(&context, 0, device));

    CUpti_EventGroupSets* passes;
    CUPTI_CHECK(cuptiMetricCreateEventGroupSets(
                    context, ids.size() * sizeof(CUpti_MetricID),
                    &ids[0], &passes)); 
    
    for (uint32_t p = 0; p < passes->numSets; ++p)
    {
        cout << kTab << "Pass " << p << endl;

        for (uint32_t g = 0; g < passes->sets[p].numEventGroups; ++g)
        {
            cout << kTab << kTab << "Event Group " << g << endl;

            CUpti_EventGroup group = passes->sets[p].eventGroups[g];

            uint32_t num_events;
            bytes = sizeof(num_events);
            CUPTI_CHECK(cuptiEventGroupGetAttribute(
                            group, CUPTI_EVENT_GROUP_ATTR_NUM_EVENTS,
                            &bytes, &num_events
                            ));
            
            vector<CUpti_EventID> events(num_events);
            bytes = num_events * sizeof(CUpti_EventID);
            CUPTI_CHECK(cuptiEventGroupGetAttribute(
                            group, CUPTI_EVENT_GROUP_ATTR_EVENTS,
                            &bytes, &events[0]
                            ));

            for (uint32_t e = 0; e < num_events; ++e)
            {
                char name[kStringSize];
                bytes = sizeof(name);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_NAME,
                                &bytes, &name[0]
                                ));
                
                cout << kTab << kTab << kTab 
                     << "Event " << e << ": " << name << endl;
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
    using namespace boost::program_options;

    stringstream stream;
    stream << endl
           << "This tool ..." << endl;
    string kExtraHelp = stream.str();

    // Parse and validate the command-line arguments

    options_description kNonPositionalOptions("cupti_avail options");
    kNonPositionalOptions.add_options()

        ("details", bool_switch()->default_value(false),
         "Display detailed information about the available CUPTI events "
         "and/or metrics.")

        ("device", value< vector<int> >(),
         "Restrict display to the CUDA device with this index. Multiple "
         "indicies may be specified. The default is to display for all "
         "devices.")

        ("events", bool_switch()->default_value(false),
         "Display the available CUPTI events.")
        
        ("metrics", bool_switch()->default_value(false),
         "Display the available CUPTI metrics.")

        ("passes", value< vector<uint32_t> >(),
         "Determine the number of event data collection passes required "
         "to compute all of the specified (by ID) CUPTI metrics. Multiple "
         "indicies may be specified.")
        
        ("help", "Display this help message.")
        
        ;
    
    positional_options_description kPositionalOptions;
    kPositionalOptions.add("database", 1);
    
    variables_map values;
    
    try
    {
        store(command_line_parser(argc, argv).options(kNonPositionalOptions).
              run(), values);       
        notify(values);
    }
    catch (const std::exception& error)
    {
        cout << endl << "ERROR: " << error.what() << endl 
             << endl << kNonPositionalOptions << kExtraHelp;
        return 1;
    }

    if (values.count("help") > 0)
    {
        cout << endl << kNonPositionalOptions << kExtraHelp;
        return 1;
    }

    set<int> indicies;
    if (values.count("device") > 0)
    {
        vector<int> temp = values["device"].as< vector<int> >();
        indicies = set<int>(temp.begin(), temp.end());
    }

    set<uint32_t> metrics;
    if (values.count("passes") > 0)
    {
        vector<uint32_t> temp = values["passes"].as< vector<uint32_t> >();
        metrics = set<uint32_t>(temp.begin(), temp.end());
    }

    // Display the requested information
    
    CUDA_CHECK(cuInit(0));

    int num_devices;
    CUDA_CHECK(cuDeviceGetCount(&num_devices));
    
    if (num_devices == 0)
    {
        cout << endl << "ERROR: There are no devices supporting CUDA." << endl;
        return 1;
    }

    vector<CUdevice> devices;
    
    for (int d = 0; d < num_devices; ++d)
    {
        CUdevice device;
        CUDA_CHECK(cuDeviceGet(&device, d));
        devices.push_back(device);
    }

    cout << endl << "Devices" << endl << endl;

    for (int d = 0; d < num_devices; ++d)
    {
        char name[kStringSize];
        CUDA_CHECK(cuDeviceGetName(name, sizeof(name), devices[d]));
        cout << kTab << d << ": " << name << endl;
    }
    
    for (int d = 0; d < num_devices; ++d)
    {
        if (indicies.empty() || (indicies.find(d) != indicies.end()))
        {
            if (values["events"].as<bool>())
            {
                cout << endl 
                     << "CUPTI Events for CUDA Device " << d << endl;
                displayEvents(devices[d], values["details"].as<bool>());
            }

            if (values["metrics"].as<bool>())
            {
                cout << endl 
                     << "CUPTI Metrics for CUDA Device " << d << endl
                     << endl;
                displayMetrics(devices[d], values["details"].as<bool>());
            }
        }
        if (!metrics.empty())
        {
            cout << endl;
            displayPasses(d, devices[d], metrics);
        }
    }
    
    cout << endl;
    return 0;
}
