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

#include <boost/format.hpp>
#include <boost/program_options.hpp>
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



/** Display CUPTI events for the specified device. */
void displayEvents(CUdevice device, bool details)
{
    size_t bytes;

    uint32_t num_domains;
    CUPTI_CHECK(cuptiGetNumEventDomains(&num_domains));

    vector<CUpti_EventDomainID> domains(num_domains);
    bytes = num_domains * sizeof(CUpti_EventDomainID);
    CUPTI_CHECK(cuptiEnumEventDomains(&bytes, &domains[0]));
    
    for (uint32_t d = 0; d < num_domains; d++)
    {
        char name[1024];
        bytes = sizeof(name);
        CUPTI_CHECK(cuptiEventDomainGetAttribute(
                        domains[d], CUPTI_EVENT_DOMAIN_ATTR_NAME,
                        &bytes, &name[0]));

        cout << kTab << "Domain " << d << ": " << name << endl;
        
        if (details)
        {
            cout << endl;

            uint32_t count;

            bytes = sizeof(count);
            CUPTI_CHECK(cuptiEventDomainGetAttribute(
                            domains[d],
                            CUPTI_EVENT_DOMAIN_ATTR_INSTANCE_COUNT,
                            &bytes, &count));

            cout << kTab << kTab << "Instance Count: " << count << endl;

            bytes = sizeof(count);
            CUPTI_CHECK(cuptiEventDomainGetAttribute(
                            domains[d],
                            CUPTI_EVENT_DOMAIN_ATTR_TOTAL_INSTANCE_COUNT,
                            &bytes, &count));
            
            cout << kTab << kTab << "Total Instance Count: " << count << endl;
            
            CUpti_EventCollectionMethod method;
            bytes = sizeof(method);
            CUPTI_CHECK(cuptiEventDomainGetAttribute(
                            domains[d],
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
        bytes = num_events * sizeof(CUpti_EventID);
        CUPTI_CHECK(cuptiEventDomainEnumEvents(domains[d], &bytes, &events[0]));

        for (uint32_t e = 0; e < num_events; ++e)
        {
            char name[1024];
            bytes = sizeof(name);
            CUPTI_CHECK(cuptiEventGetAttribute(
                            events[e], CUPTI_EVENT_ATTR_NAME, &bytes, &name[0]
                            ));

            cout << kTab << kTab << "Event " << e << ": " << name << endl;

            if (details)
            {
                cout << endl;
                
                char description[1024];

                bytes = sizeof(description);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_SHORT_DESCRIPTION,
                                &bytes, &description
                                ));
                
                cout << kTab << kTab << kTab << "Short Description: "
                     << description << endl;
            
                bytes = sizeof(description);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_LONG_DESCRIPTION,
                                &bytes, &description
                                ));
                
                cout << kTab << kTab << kTab << "Long Description: "
                     << description << endl;

                CUpti_EventCategory category;
                bytes = sizeof(category);
                CUPTI_CHECK(cuptiEventGetAttribute(
                                events[e], CUPTI_EVENT_ATTR_CATEGORY,
                                &bytes, &category
                                ));
                
                cout << kTab << kTab << kTab << "Event Collection Method: ";
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
    // ...
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
        char name[1024];
        CUDA_CHECK(cuDeviceGetName(name, sizeof(name), devices[d]));
        cout << kTab << d << ": " << name << endl;
    }
    
    for (int d = 0; d < num_devices; ++d)
    {
        if (indicies.empty() || (indicies.find(d) != indicies.end()))
        {
            if (values["events"].as<bool>())
            {
                cout << endl;
                cout << "CUPTI Events for CUDA Device #" << d << endl;
                cout << endl;                
                displayEvents(devices[d], values["details"].as<bool>());
            }

            if (values["metrics"].as<bool>())
            {
                cout << endl;
                cout << "CUPTI Metrics for CUDA Device #" << d << endl;
                cout << endl;                
                displayMetrics(devices[d], values["details"].as<bool>());
            }            
        }
    }
    
    cout << endl;
    return 0;
}
