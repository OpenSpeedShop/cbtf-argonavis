/*******************************************************************************
** Copyright (c) 2012-2017 Argo Navis Technologies. All Rights Reserved.
**
** This program is free software; you can redistribute it and/or modify it under
** the terms of the GNU General Public License as published by the Free Software
** Foundation; either version 2 of the License, or (at your option) any later
** version.
**
** This program is distributed in the hope that it will be useful, but WITHOUT
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
** FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
** details.
**
** You should have received a copy of the GNU General Public License along with
** this program; if not, write to the Free Software Foundation, Inc., 59 Temple
** Place, Suite 330, Boston, MA  02111-1307  USA
*******************************************************************************/

/** @file Implementation of the CUDA collector. */

#include <inttypes.h>
#include <monitor.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <unistd.h>

#include <KrellInstitute/Services/Assert.h>
#include <KrellInstitute/Services/Collector.h>

#include "CUPTI_activities.h"
#include "CUPTI_callbacks.h"
#include "CUPTI_metrics.h"
#include "PAPI.h"
#include "Pthread_check.h"
#include "TLS.h"



/** String uniquely identifying this collector. */
const char* const cbtf_collector_unique_id = "cuda";

/** Flag indicating if debugging is enabled. */
bool IsDebugEnabled = FALSE;

/**
 * Event sampling configuration. Initialized by the process-wide initialization
 * in cbtf_collector_start() through its call to parse_configuration().
 */
CUDA_SamplingConfig TheSamplingConfig;

/**
 * Descriptions of sampled events. Also initialized by the process-wide
 * initialization in cbtf_collector_start() through parse_configuration().
 */
static CUDA_EventDescription EventDescriptions[MAX_EVENTS];

/**
 * The number of threads for which are are collecting data (actively or not).
 * This value is atomically incremented in cbtf_collector_start(), decremented
 * in cbtf_collector_stop(), and is used by those functions to determine when
 * to perform process-wide initialization and finalization.
 */
static struct {
    int value;
    pthread_mutex_t mutex;
} ThreadCount = { 0, PTHREAD_MUTEX_INITIALIZER };


    
/**
 * Parse the configuration string that was passed into this collector.
 *
 * @param configuration    Configuration string passed into this collector.
 */
static void parse_configuration(const char* const configuration)
{
    static const char* const kIntervalPrefix = "interval=";

#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CUDA %d:%d] parse_configuration(\"%s\")\n",
               getpid(), monitor_get_thread_num(), configuration);
    }
#endif

    /* Initialize the event sampling configuration */

    TheSamplingConfig.interval = 10 * 1000 * 1000 /* 10 mS */;
    TheSamplingConfig.events.events_len = 0;
    TheSamplingConfig.events.events_val = EventDescriptions;

    memset(EventDescriptions, 0, MAX_EVENTS * sizeof(CUDA_EventDescription));

    /* Copy the configuration string for parsing */
    
    static char copy[4 * 1024 /* 4 KB */];
    
    if (strlen(configuration) >= sizeof(copy))
    {
        fprintf(stderr, "[CUDA %d:%d] parse_configuration(): "
                "Configuration string \"%s\" exceeds the maximum "
                "support length (%llu)!\n", getpid(), monitor_get_thread_num(),
                configuration, (unsigned long long)(sizeof(copy) - 1));
        fflush(stderr);
        abort();
    }
    
    strcpy(copy, configuration);

    /* Split the configuration string into tokens at each comma */
    char* ptr = NULL;
    for (ptr = strtok(copy, ","); ptr != NULL; ptr = strtok(NULL, ","))
    {
        /*
         * Parse this token into a sampling interval, an event name, or an
         * event name and threshold, depending on whether the token contains
         * an at ('@') character or starts with the string "interval=".
         */
        
        char* interval = strstr(ptr, kIntervalPrefix);
        char* at = strchr(ptr, '@');
        
        /* Token is a sampling interval */
        if (interval != NULL)
        {
            int64_t interval = atoll(ptr + strlen(kIntervalPrefix));

            /* Warn if the samping interval was invalid */
            if (interval <= 0)
            {
                fprintf(stderr, "[CUDA %d:%d] parse_configuration(): "
                        "An invalid sampling interval (\"%s\") was "
                        "specified!\n", getpid(), monitor_get_thread_num(),
                        ptr + strlen(kIntervalPrefix));
                fflush(stderr);
                abort();
            }
            
            TheSamplingConfig.interval = (uint64_t)interval;

#if !defined(NDEBUG)
            if (IsDebugEnabled)
            {
                printf("[CUDA %d:%d] parse_configuration(): "
                       "sampling interval = %llu nS\n",
                       getpid(), monitor_get_thread_num(),
                       (long long unsigned)TheSamplingConfig.interval);
            }
#endif
        }
        else
        {
            /* Warn if the maximum supported sampled events was reached */
            if (TheSamplingConfig.events.events_len == MAX_EVENTS)
            {
                fprintf(stderr, "[CUDA %d:%d] parse_configuration(): "
                        "Maximum supported number of concurrently sampled"
                        "events (%d) was reached!\n",
                        getpid(), monitor_get_thread_num(), MAX_EVENTS);
                fflush(stderr);
                abort();
            }
            
            CUDA_EventDescription* event = &TheSamplingConfig.events.events_val[
                TheSamplingConfig.events.events_len
                ];
            
            event->name = malloc(sizeof(copy));
            
            /* Token is an event name and threshold */
            if (at != NULL)
            {
                strncpy(event->name, ptr, at - ptr);
                event->threshold = atoi(at + 1);
                
#if !defined(NDEBUG)
                if (IsDebugEnabled)
                {
                    printf("[CUDA %d:%d] parse_configuration(): "
                           "event name = \"%s\", threshold = %d\n",
                           getpid(), monitor_get_thread_num(),
                           event->name, event->threshold);
                }
#endif
            }

            /* Token is an event name */
            else
            {
                strcpy(event->name, ptr);
            
#if !defined(NDEBUG)
                if (IsDebugEnabled)
                {
                    printf("[CUDA %d:%d] parse_configuration(): "
                           "event name = \"%s\"\n", 
                           getpid(), monitor_get_thread_num(), event->name);
                }
#endif
            }

            /* Increment the number of sampled events */
            ++TheSamplingConfig.events.events_len;
        }
    }
}



/**
 * Called by the CBTF collector service in order to start data collection.
 */
void cbtf_collector_start(const CBTF_DataHeader* const header)
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CUDA %d:%d] cbtf_collector_start()\n",
               getpid(), monitor_get_thread_num());
    }
#endif

    /* Create and zero-initialize our thread-local storage */
    TLS_initialize();
    
    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Copy the header into our thread-local storage for future use */
    memcpy(&tls->data_header, header, sizeof(CBTF_DataHeader));

    /* Atomically increment the active thread count */

    PTHREAD_CHECK(pthread_mutex_lock(&ThreadCount.mutex));
    
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CUDA %d:%d] cbtf_collector_start(): "
               "ThreadCount.value = %d --> %d\n",
               getpid(), monitor_get_thread_num(),
               ThreadCount.value, ThreadCount.value + 1);
    }
#endif
    
    if (ThreadCount.value == 0)
    {
        /* Should debugging be enabled? */
        IsDebugEnabled = (getenv("CBTF_DEBUG_COLLECTOR") != NULL);
        
#if !defined(NDEBUG)
        if (IsDebugEnabled)
        {
            printf("[CUDA %d:%d] cbtf_collector_start()\n",
                   getpid(), monitor_get_thread_num());
            printf("[CUDA %d:%d] cbtf_collector_start(): "
                   "ThreadCount.value = %d --> %d\n",
                   getpid(), monitor_get_thread_num(),
                   ThreadCount.value, ThreadCount.value + 1);
        }
#endif
        
        /* Obtain our configuration string from the environment and parse it */
        const char* const configuration = getenv("CBTF_CUDA_CONFIG");
        if (configuration != NULL)
        {
            parse_configuration(configuration);
        }

        if (TheSamplingConfig.events.events_len > 0)
        {
            /* Initialize PAPI for this process */
            PAPI_initialize();

            /* Initialize CUPTI metrics data collection for this process */
            CUPTI_metrics_initialize();
        }

        /* Start CUPTI activity data collection for this process */
        CUPTI_activities_start();
        
        /* Subscribe to CUPTI callbacks for this process */
        CUPTI_callbacks_subscribe();
    }

    ThreadCount.value++;

    PTHREAD_CHECK(pthread_mutex_unlock(&ThreadCount.mutex));

    /* Initialize our performance data header and blob */
    TLS_initialize_data(tls);

    if (TheSamplingConfig.events.events_len > 0)
    {
        /* Append event sampling configuration to our performance data blob */

        CBTF_cuda_message* raw_message = TLS_add_message(tls);
        Assert(raw_message != NULL);
        raw_message->type = SamplingConfig;
        
        CUDA_SamplingConfig* message = 
            &raw_message->CBTF_cuda_message_u.sampling_config;
        
        memcpy(message, &TheSamplingConfig, sizeof(CUDA_SamplingConfig));
    }

    /* Resume data collection for this thread */
    cbtf_collector_resume();
    
    if ((TheSamplingConfig.events.events_len > 0) &&
        (monitor_get_addr_thread_start() != CUPTI_metrics_sampling_thread))
    {
        /* Start PAPI data collection for this thread */
        PAPI_start_data_collection();
    }
}



/**
 * Called by the CBTF collector service in order to pause data collection.
 */
void cbtf_collector_pause()
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CUDA %d:%d] cbtf_collector_pause()\n",
               getpid(), monitor_get_thread_num());
    }
#endif

    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Pause data collection for this thread */
    tls->paused = TRUE;
}



/**
 * Called by the CBTF collector service in order to resume data collection.
 */
void cbtf_collector_resume()
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CUDA %d:%d] cbtf_collector_resume()\n",
               getpid(), monitor_get_thread_num());
    }
#endif

    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Resume data collection for this thread */
    tls->paused = FALSE;
}



/**
 * Called by the CBTF collector service in order to stop data collection.
 */
void cbtf_collector_stop()
{
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CUDA %d:%d] cbtf_collector_stop()\n",
               getpid(), monitor_get_thread_num());
    }
#endif

    if ((TheSamplingConfig.events.events_len > 0) &&
        (monitor_get_addr_thread_start() != CUPTI_metrics_sampling_thread))
    {
        /* Stop PAPI data collection for this thread */
        PAPI_stop_data_collection();
    }

    /* Pause data collection for this thread */
    cbtf_collector_pause();

    /* Atomically decrement the active thread count */
    
    PTHREAD_CHECK(pthread_mutex_lock(&ThreadCount.mutex));
    
#if !defined(NDEBUG)
    if (IsDebugEnabled)
    {
        printf("[CUDA %d:%d] cbtf_collector_stop(): "
               "ThreadCount.value = %d --> %d\n",
               getpid(), monitor_get_thread_num(),
               ThreadCount.value, ThreadCount.value - 1);
    }
#endif
    
    ThreadCount.value--;

    if (ThreadCount.value == 0)
    {
        /* Ensure all CUPTI activity data for this process has been flushed. */
        CUPTI_activities_flush();

        /* Stop CUPTI activity data collection for this process */
        CUPTI_activities_stop();
        
        /* Unsubscribe to CUPTI callbacks for this process */
        CUPTI_callbacks_unsubscribe();

        if (TheSamplingConfig.events.events_len > 0)
        {
            /* Finalize CUPTI metrics data collection for this process */
            CUPTI_metrics_finalize();
            
            /* Finalize PAPI for this process */
            PAPI_finalize();
        }
    }
    
    PTHREAD_CHECK(pthread_mutex_unlock(&ThreadCount.mutex));

    /* Access our thread-local storage */
    TLS* tls = TLS_get();

    /* Send any remaining performance data for this thread */
    TLS_send_data(tls);
    
    /* Destroy our thread-local storage */
    TLS_destroy();
}
