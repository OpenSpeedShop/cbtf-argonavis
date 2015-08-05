////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013 Krell Institute. All Rights Reserved.
// Copyright (c) 2015 Argo Navis Technologies. All Rights Reserved.
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

/** @file Declaration and definition of the Time class. */

#pragma once

#include <boost/cstdint.hpp>
#include <boost/operators.hpp>
#include <boost/system/system_error.hpp>
#include <errno.h>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <time.h>

#include <KrellInstitute/Messages/Time.h>

namespace ArgoNavis { namespace Base {

    /**
     * All time values are stored in a single 64-bit unsigned integer. These
     * integers are interpreted as the number of nanoseconds that have passed
     * since midnight (00:00) Coordinated Universal Time (UTC), on January 1,
     * 1970. This system gives nanosecond resolution for representing times
     * while not running out the clock until sometime in the year 2554.
     */
    class Time :
        public boost::additive<Time>,
        public boost::additive<Time, int>,
        public boost::totally_ordered<Time>,
        public boost::unit_steppable<Time>
    {

    public:

        /** Construct the earliest possible time. */
        static Time TheBeginning()
        {
            return Time(std::numeric_limits<boost::uint64_t>::min());
        }

        /** Create the current time. */
        static Time Now()
        {
            struct timespec now;

            if (clock_gettime(CLOCK_REALTIME, &now) != 0)
            {
                using namespace boost::system;
                throw system_error(errno, system_category(), "clock_gettime");
            }

            return Time((static_cast<boost::uint64_t>(now.tv_sec) *
                         static_cast<boost::uint64_t>(1000000000)) +
                        static_cast<boost::uint64_t>(now.tv_nsec));
        }
        
        /** Create the last possible time. */
        static Time TheEnd()
        {
            return Time(std::numeric_limits<boost::uint64_t>::max());
        }
        
        /** Default constructor. */
        Time() :
            dm_value()
        {
        }

        /** Construct a time from a CBTF_Protocol_Time (uint64_t). */
        Time(const CBTF_Protocol_Time& message) :
            dm_value(message)
        {
        }
        
        /** Type conversion to a CBTF_Protocol_Time (uint64_t). */
        operator CBTF_Protocol_Time() const
        {
            return CBTF_Protocol_Time(dm_value);
        }

        /** Type conversion to a string. */
        operator std::string() const
        {
            std::ostringstream stream;
            stream << *this;
            return stream.str();
        }

        /** Is this time less than another one? */
        bool operator<(const Time& other) const
        {
            return dm_value < other.dm_value;
        }

        /** Is this time equal to another one? */
        bool operator==(const Time& other) const
        {
            return dm_value == other.dm_value;
        }

        /** Increment this time. */
        Time& operator++()
        {
            dm_value += 1;
            return *this;
        }

        /** Add another time to this time. */
        Time& operator+=(const Time& other)
        {
            dm_value += other.dm_value;
            return *this;
        }
        
        /** Add a signed integer offset to this time. */
        Time& operator+=(int offset)
        {
            dm_value += offset;
            return *this;
        }
        
        /** Decrement this time. */
        Time& operator--()
        {
            dm_value -= 1;
            return *this;
        }

        /** Subtract another time from this time. */
        Time& operator-=(const Time& other)
        {
            dm_value -= other.dm_value;
            return *this;
        }

        /** Subtract a signed integer offset from this time. */
        Time& operator-=(int offset)
        {
            dm_value -= offset;
            return *this;
        }

        /** Redirection to an output stream. */
        friend std::ostream& operator<<(std::ostream& stream, const Time& time)
        {
            time_t calendar_time = time.dm_value / 1000000000;
            struct tm broken_down_time;

            if (localtime_r(&calendar_time, &broken_down_time) == NULL)
            {
                using namespace boost::system;
                throw system_error(errno, system_category(), "localtime_r");
            }
            
            char buffer[32];
            
            if (strftime(buffer, sizeof(buffer),
                         "%Y/%m/%d %H:%M:%S", &broken_down_time) == 0)
            {
                using namespace boost::system;
                throw system_error(errno, system_category(), "strftime");
            }
            
            stream << buffer;
            return stream;
        }
        
    private:
        
        /** Value of this time. */
        boost::uint64_t dm_value;
        
    }; // class Time

} } // namespace ArgoNavis::Base
