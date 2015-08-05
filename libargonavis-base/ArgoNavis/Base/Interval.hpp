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

/** @file Declaration and definition of the Interval class. */

#pragma once

#include <boost/cstdint.hpp>
#include <boost/operators.hpp>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace ArgoNavis { namespace Base {

    /**
     * A closed-ended, integer, interval used to represent either address
     * ranges or time intervals.
     *
     * @param T    Type of the interval's (integer) values.
     * @param M    Type of message used to store an interval of this type.
     * 
     * @sa http://en.wikipedia.org/wiki/Interval_(mathematics)
     */
    template <typename T, typename M>
    class Interval :
        public boost::andable<Interval<T, M> >,
        public boost::orable<Interval<T, M> >,
        public boost::totally_ordered<Interval<T, M> >
    {
        
    public:
        
        /** Default constructor. */
        Interval() :
            dm_begin(T() - 1),
            dm_end(T())
        {
        }
        
        /** Construct an interval from the begin and end values. */
        Interval(const T& begin, const T& end) :
            dm_begin(begin),
            dm_end(end)
        {
        }
        
        /** Construct an interval containing a single value. */
        Interval(const T& value) :
            dm_begin(value),
            dm_end(value)
        {
        }
        
        /** Construct an interval from a message. */
        Interval(const M& message) :
            dm_begin(message.begin),
            dm_end(message.end - 1)
        {
        }
        
        /** Type conversion to a message. */
        operator M() const
        {
            M message;
            message.begin = dm_begin;
            message.end = dm_end + 1;
            return message;
        }
        
        /** Type conversion to a string. */
        operator std::string() const
        {
            std::ostringstream stream;
            stream << *this;
            return stream.str();
        }
        
        /** Is this interval less than another one? */
        bool operator<(const Interval& other) const
        {
            if (dm_begin < other.dm_begin)
            {
                return true;
            }
            if (dm_begin > other.dm_begin)
            {
                return false;
            }
            return dm_end < other.dm_end;
        }
        
        /** Is this interval equal to another one? */
        bool operator==(const Interval& other) const
        {
            return (dm_begin == other.dm_begin) && (dm_end == other.dm_end);
        }

        /** Unite another interval with this one. */
        Interval& operator|=(const Interval& other)
        {
            if (!other.empty())
            {
                if (!empty())
                {
                    if (dm_begin > other.dm_begin)
                    {
                        dm_begin = other.dm_begin;
                    }
                    if (dm_end < other.dm_end)
                    {
                        dm_end = other.dm_end;
                    }
                }
                else
                {
                    *this = other;
                }
            }
            return *this;
        }

        /** Intersect another interval with this one. */
        Interval& operator&=(const Interval& other)
        {
            if (!empty())
            {
                if (!other.empty())
                {
                    if (dm_begin < other.dm_begin)
                    {
                        dm_begin = other.dm_begin;
                    }
                    if (dm_end > other.dm_end)
                    {
                        dm_end = other.dm_end;
                    }
                }
                else
                {
                    *this = other;
                }
            }
            return *this;
        }
        
        /** Get the beginning of this interval. */
        const T& begin() const
        { 
            return dm_begin;
        }
        
        /** Get the end of this interval. */
        const T& end() const
        {
            return dm_end;
        }

        /** Is this interval empty? */
        bool empty() const
        {
            return dm_end < dm_begin;
        }
        
        /** Get the width of this interval. */
        T width() const
        {
            return empty() ? T(0) : (dm_end - dm_begin + 1);
        }
        
        /** Does this interval contain a value? */
        bool contains(const T& value) const
        {
            return (value >= dm_begin) && (value <= dm_end);
        }
        
        /** Does this interval contain another one? */
        bool contains(const Interval& other) const
        {
            return !other.empty() &&
                contains(other.dm_begin) && contains(other.dm_end);
        }
        
        /** Does this interval intersect another one? */
        bool intersects(const Interval& other) const
        {
            return !(*this & other).empty();
        }
        
        /** Redirection to an output stream. */
        friend std::ostream& operator<<(std::ostream& stream,
                                        const Interval& interval)
        {
            stream << "[" << interval.dm_begin << ", " 
                   << interval.dm_end << "]";
            return stream;
        }
        
    private:

        /** Closed beginning of this interval. */
        T dm_begin;
        
        /** Closed end of this interval. */
        T dm_end;
        
    }; // class Interval<T, S>
        
} } // namespace ArgoNavis::Base

namespace std {
    
    /**
     * Partial specialization of the less-than functor for intervals. One
     * reason for an interval class is to allow multiple, non-overlapping,
     * intervals to be put into STL associative containers such that they
     * can be searched for a particular value in logarithmic time. But
     * associative containers cannot search for the value directly. They
     * can, however, search for the interval [value, value] containing
     * only the value. Using the "constructor from a single value" is simple
     * and convenient enough. However, the associative containers would
     * still use the default ordering as defined by Interval's overloaded
     * less-than operator. And that operator defines a total, rather than
     * a simply strick weak, ordering. The result would be that overlapping
     * intervals would not be considered equivalent. This template redefines
     * the ordering used by all STL associative containers of Interval to
     * impose a strict weak ordering in which two such intervals <em>are</em>
     * considered equivalent. Doing so causes searches for a value within
     * non-overlapping intervals to behave as desired.
     */
    template <typename T, typename S>
    struct less<ArgoNavis::Base::Interval<T, S> > :
        binary_function<const ArgoNavis::Base::Interval<T, S>&,
                        const ArgoNavis::Base::Interval<T, S>&,
                        bool>
    {
        bool operator()(const ArgoNavis::Base::Interval<T, S>& lhs,
                        const ArgoNavis::Base::Interval<T, S>& rhs) const
        {
            if (lhs.end() < rhs.begin())
            {
                return true;
            }
            return false;
        }
    }; // struct less<Interval<T, S> >

} // namespace std
