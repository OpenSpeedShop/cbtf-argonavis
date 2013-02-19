////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2005 Silicon Graphics, Inc. All Rights Reserved.
// Copyright (c) 2012 Argo Navis Technologies. All Rights Reserved.
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

#include <boost/assert.hpp>
#include <boost/operators.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_same.hpp>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>

namespace KrellInstitute { namespace SymbolTable {

    /**
     * A open-ended, integer, interval used to represent either address ranges
     * or time intervals.
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
        /**
         * This template isn't truely generic in that it makes assumptions
         * regarding the types used to represent the integer values in the
         * interval and their differences. Check that T is a type that is
         * known to be supported.
         */
        BOOST_STATIC_ASSERT(boost::is_same<T, Address>::value ||
                            boost::is_same<T, Time>::value);
        
    public:
        
        /** Default constructor. */
        Interval() :
            dm_begin(),
            dm_end()
        {
        }
        
        /** Construct an interval from the begin and end values. */
        Interval(const T& begin, const T& end) :
            dm_begin(begin),
            dm_end(end)
        {
            BOOST_ASSERT(dm_begin <= dm_end);
        }
        
        /** Construct an interval containing a single value. */
        Interval(const T& value) :
            dm_begin(value),
            dm_end(value + 1)
        {
            BOOST_ASSERT(dm_begin <= dm_end);
        }
        
        /** Construct an interval from a message. */
        Interval(const M& message) :
            dm_begin(message.begin),
            dm_end(message.end)
        {
            BOOST_ASSERT(dm_begin <= dm_end);
        }
        
        /** Type conversion to a message. */
        operator M() const
        {
            M message;
            message.begin = dm_begin;
            message.end = dm_end;
            return message;
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

        /** Union this interval with another one. */
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
        
        /** Intersect this interval with another one. */
        Interval& operator&=(const Interval& other)
        {
            if (dm_begin < other.dm_begin)
            {
                dm_begin = other.dm_begin;
            }
            if (dm_end > other.dm_end)
            {
                dm_end = other.dm_end;
            }
            if (dm_end < dm_begin)
            {
                dm_begin = dm_end;
            }
            return *this;
        }
        
        /** Get the closed beginning of this interval. */
        const T& getBegin() const
        { 
            return dm_begin;
        }

        /** Get the open end of this interval. */
        const T& getEnd() const
        {
            return dm_end; 
        }

        /** Is this interval empty? */
        bool isEmpty() const
        {
            return dm_begin == dm_end;
        }
        
        /** Get the width of this interval. */
        uint64_t getWidth() const
        {
            return static_cast<uint64_t>(dm_end) - 
                static_cast<uint64_t>(dm_begin);
        }
        
        /** Does this interval contain a value? */
        bool doesContain(const T& value) const
        {
            return (value >= dm_begin) && (value < dm_end);
        }

        /** Does this interval contain another interval? */
        bool doesContain(const Interval& other) const
        {
            return contains(other.dm_begin) && contains(other.dm_end - 1);
        }

        /** Does this interval intersect another interval? */
        bool doesIntersect(const Interval& other) const
        {
            return !(*this & other).empty();
        }
        
        /** Redirection to an output stream. */
        friend std::ostream& operator<<(std::ostream& stream,
                                        const Interval& interval)
        {
            stream << "[ " << interval.dm_begin << ", " 
                   << interval.dm_end << " )";
            return stream;
        }
        
    private:

        /** Closed beginning of this interval. */
        T dm_begin;
        
        /** Open end of this interval. */
        T dm_end;
        
    }; // class Interval<T, S>
        
} } // namespace KrellInstitute::SymbolTable

namespace std {
    
    /**
     * Partial specialization of the less-than functor for intervals. One reason
     * for an interval class is to allow multiple, non-overlapping, intervals to
     * be put into STL associative containers such that they can be searched for
     * a particular value in logarithmic time. But associative containers cannot
     * search for the value directly. They can, however, search for the interval
     * [value, value + 1) containing only the value. Using the "constructor from
     * a single value" is simple and convenient enough. However, the associative
     * containers would still use the default ordering as defined by Interval's
     * overloaded less-than operator. And that operator defines a total, rather
     * than a simply strick weak, ordering. The result would be that overlapping
     * intervals would not be considered equivalent. This template redefines the
     * ordering used by all STL associative containers of Interval to impose a
     * strict weak ordering in which two such intervals <em>are</em> considered
     * equivalent. Doing so causes searches for a value within non-overlapping
     * intervals to behave as desired.
     */
    template <typename T, typename S>
    struct less<KrellInstitute::SymbolTable::Interval<T, S> > :
        binary_function<const KrellInstitute::SymbolTable::Interval<T, S>&,
                        const KrellInstitute::SymbolTable::Interval<T, S>&,
                        bool>
    {
        bool operator()(
            const KrellInstitute::SymbolTable::Interval<T, S>& lhs,
            const KrellInstitute::SymbolTable::Interval<T, S>& rhs
            ) const
        {
            if (lhs.getEnd() <= rhs.getBegin())
            {
                return true;
            }
            return false;
        }
    }; // struct less<Interval<T, S> >
    
} // namespace std
