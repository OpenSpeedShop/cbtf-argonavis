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

/** @file Unit tests for the ArgoNavis base library. */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE ArgoNavis-Base

#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/AddressBitmap.hpp>
#include <ArgoNavis/Base/AddressRange.hpp>
#include <ArgoNavis/Base/AddressSpaces.hpp>
#include <ArgoNavis/Base/FileName.hpp>
#include <ArgoNavis/Base/Function.hpp>
#include <ArgoNavis/Base/LinkedObject.hpp>
#include <ArgoNavis/Base/Loop.hpp>
#include <ArgoNavis/Base/Statement.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>
#include <ArgoNavis/Base/Time.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

using namespace ArgoNavis::Base;
using namespace ArgoNavis::Base::Impl;



/** Anonymous namespace hiding implementation details. */
namespace {

    /** Templated visitor for accumulating functions, etc. */
    template <typename T>
    bool accumulate(const T& x, std::set<T>& set)
    {
        set.insert(x);
        return true;
    }

    /** Templated visitor for accumulating mappings. */
    bool accumulateMappings(
        const ThreadName& thread,
        const LinkedObject& linked_object,
        const AddressRange& range,
        const TimeInterval& interval,
        std::map<LinkedObject, std::vector<boost::uint64_t> >& set
        )
    {
        std::vector<boost::uint64_t> where = boost::assign::list_of
            (static_cast<boost::uint64_t>(range.begin()))
            (static_cast<boost::uint64_t>(range.end()))
            (static_cast<boost::uint64_t>(interval.begin()))
            (static_cast<boost::uint64_t>(interval.end()));
        
        set.insert(std::make_pair(linked_object, where));
        return true;
    }
    
} // namespace <anonymous>



/**
 * Unit test for the Address class.
 */
BOOST_AUTO_TEST_CASE(TestAddress)
{
    BOOST_CHECK_EQUAL(Address(), Address::TheLowest());
    BOOST_CHECK_NE(Address::TheLowest(), Address::TheHighest());
    BOOST_CHECK_LT(Address::TheLowest(), Address::TheHighest());
    BOOST_CHECK_GT(Address::TheHighest(), Address::TheLowest());

    BOOST_CHECK_EQUAL(Address(static_cast<CBTF_Protocol_Address>(Address(27))),
                      Address(27));

    BOOST_CHECK_EQUAL(--Address::TheLowest(), Address::TheHighest());
    BOOST_CHECK_EQUAL(Address::TheLowest() - 1, Address::TheHighest());

    BOOST_CHECK_EQUAL(Address::TheLowest(), ++Address::TheHighest());
    BOOST_CHECK_EQUAL(Address::TheLowest(), Address::TheHighest() + 1);

    BOOST_CHECK_EQUAL(Address(27), Address(4) + Address(23));
    BOOST_CHECK_EQUAL(Address(27), Address(2) + 25);

    BOOST_CHECK_EQUAL(Address(27), Address(30) - Address(3));
    BOOST_CHECK_EQUAL(Address(27), Address(29) - 2);

    BOOST_CHECK_EQUAL(static_cast<std::string>(Address(13*27)),
                      "0x000000000000015F");
}



/**
 * Unit test for the AddressBitmap class.
 */
BOOST_AUTO_TEST_CASE(TestAddressBitmap)
{
    AddressBitmap bitmap(AddressRange(0, 13));

    BOOST_CHECK_EQUAL(bitmap.range(), AddressRange(0, 13));
    
    BOOST_CHECK(!bitmap.get(0));
    BOOST_CHECK(!bitmap.get(7));
    BOOST_CHECK(!bitmap.get(13));
    BOOST_CHECK_THROW(bitmap.get(27), std::invalid_argument);
    bitmap.set(7, true);
    BOOST_CHECK(!bitmap.get(0));
    BOOST_CHECK(bitmap.get(7));
    BOOST_CHECK(!bitmap.get(13));
    BOOST_CHECK_THROW(bitmap.set(27, true), std::invalid_argument);
    bitmap.set(7, false);
    BOOST_CHECK(!bitmap.get(0));
    BOOST_CHECK(!bitmap.get(7));
    BOOST_CHECK(!bitmap.get(13));

    bitmap.set(7, true);
    std::set<AddressRange> ranges = bitmap.ranges(false);
    BOOST_CHECK_EQUAL(ranges.size(), 2);
    BOOST_CHECK_EQUAL(*(ranges.begin()), AddressRange(0, 6));
    BOOST_CHECK_EQUAL(*(++ranges.begin()), AddressRange(8, 13));
    ranges = bitmap.ranges(true);
    BOOST_CHECK_EQUAL(ranges.size(), 1);
    BOOST_CHECK_EQUAL(*(ranges.begin()), AddressRange(7, 7));
    bitmap.set(12, true);
    bitmap.set(13, true);
    ranges = bitmap.ranges(true);
    BOOST_CHECK_EQUAL(ranges.size(), 2);
    BOOST_CHECK_EQUAL(*(ranges.begin()), AddressRange(7, 7));
    BOOST_CHECK_EQUAL(*(++ranges.begin()), AddressRange(12, 13));

    std::set<Address> addresses = boost::assign::list_of(0)(7)(13)(27);
    bitmap = AddressBitmap(addresses);
    BOOST_CHECK_EQUAL(bitmap.range(), AddressRange(0, 27));
    BOOST_CHECK(bitmap.get(0));
    BOOST_CHECK(bitmap.get(7));
    BOOST_CHECK(bitmap.get(13));
    BOOST_CHECK(bitmap.get(27));
    BOOST_CHECK(!bitmap.get(1));

    BOOST_CHECK_EQUAL(
        static_cast<std::string>(bitmap),
        "[0x0000000000000000, 0x000000000000001B]: 1000000100000100000000000001"
        );

    BOOST_CHECK_EQUAL(
        AddressBitmap(static_cast<CBTF_Protocol_AddressBitmap>(bitmap)),
        bitmap
        );
}



/**
 * Unit test for the AddressRange class.
 */
BOOST_AUTO_TEST_CASE(TestAddressRange)
{
    BOOST_CHECK(AddressRange().empty());
    BOOST_CHECK(!AddressRange(0).empty());
    BOOST_CHECK_EQUAL(AddressRange(27).begin(), AddressRange(27).end());
    BOOST_CHECK(!AddressRange(0, 1).empty());
    BOOST_CHECK(AddressRange(1, 0).empty());

    BOOST_CHECK_EQUAL(
        AddressRange(
            static_cast<CBTF_Protocol_AddressRange>(AddressRange(0, 27))
            ),
        AddressRange(0, 27)
        );

    BOOST_CHECK_LT(AddressRange(0, 13), AddressRange(1, 13));
    BOOST_CHECK_LT(AddressRange(0, 13), AddressRange(0, 27));
    BOOST_CHECK_GT(AddressRange(1, 13), AddressRange(0, 13));
    BOOST_CHECK_GT(AddressRange(0, 27), AddressRange(0, 13));
    BOOST_CHECK_EQUAL(AddressRange(0, 13), AddressRange(0, 13));
    BOOST_CHECK_NE(AddressRange(0, 13), AddressRange(0, 27));

    BOOST_CHECK_EQUAL(AddressRange(0, 13) | AddressRange(7, 27),
                      AddressRange(0, 27));
    BOOST_CHECK_EQUAL(AddressRange(7, 27) | AddressRange(0, 13),
                      AddressRange(0, 27));
    BOOST_CHECK_EQUAL(AddressRange(0, 7) | AddressRange(13, 27),
                      AddressRange(0, 27));
    BOOST_CHECK_EQUAL(AddressRange(13, 27) | AddressRange(0, 7),
                      AddressRange(0, 27));
    BOOST_CHECK_EQUAL(AddressRange(0, 7) | AddressRange(),
                      AddressRange(0, 7));
    BOOST_CHECK_EQUAL(AddressRange() | AddressRange(13, 27),
                      AddressRange(13, 27));

    BOOST_CHECK_EQUAL(AddressRange(0, 13) & AddressRange(7, 27),
                      AddressRange(7, 13));
    BOOST_CHECK_EQUAL(AddressRange(7, 27) & AddressRange(0, 13),
                      AddressRange(7, 13));
    BOOST_CHECK((AddressRange(0, 7) & AddressRange(13, 27)).empty());
    BOOST_CHECK((AddressRange(13, 27) & AddressRange(0, 7)).empty());
    BOOST_CHECK((AddressRange(0, 13) & AddressRange()).empty());
    //
    // GCC 4.8.1 and 4.9.0 contain a bug that prevents them from compiling
    // the following, perfectly legal, code. The only workaround is to not
    // compile this particular check for those compilers.
    //
    // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=57532
    //   
#if !((__GNUC__ == 4 && __GNUC_MINOR__ == 8 && __GNUC_PATCHLEVEL__ == 1) || \
      (__GNUC__ == 4 && __GNUC_MINOR__ == 9 && __GNUC_PATCHLEVEL__ == 0))    
    BOOST_CHECK((AddressRange() & AddressRange(0, 13)).empty());
#endif
    
    BOOST_CHECK_EQUAL(AddressRange(0, 13).width(), Address(14));
    BOOST_CHECK_EQUAL(AddressRange(13, 0).width(), Address(0));

    BOOST_CHECK(AddressRange(0, 13).contains(7));
    BOOST_CHECK(!AddressRange(0, 13).contains(27));
    BOOST_CHECK(!AddressRange(13, 0).contains(7));
    BOOST_CHECK(!AddressRange(13, 0).contains(27));

    BOOST_CHECK(AddressRange(0, 27).contains(AddressRange(7, 13)));
    BOOST_CHECK(!AddressRange(0, 13).contains(AddressRange(7, 27)));
    BOOST_CHECK(!AddressRange(27, 0).contains(AddressRange(7, 13)));
    BOOST_CHECK(!AddressRange(13, 0).contains(AddressRange(7, 27)));
    BOOST_CHECK(!AddressRange(0, 27).contains(AddressRange(13, 7)));
    BOOST_CHECK(!AddressRange(0, 13).contains(AddressRange(27, 7)));    
    BOOST_CHECK(!AddressRange(27, 0).contains(AddressRange(13, 7)));
    BOOST_CHECK(!AddressRange(13, 0).contains(AddressRange(27, 7)));

    BOOST_CHECK(AddressRange(0, 27).intersects(AddressRange(7, 13)));
    BOOST_CHECK(AddressRange(0, 13).intersects(AddressRange(7, 27)));
    BOOST_CHECK(!AddressRange(27, 0).intersects(AddressRange(7, 13)));
    BOOST_CHECK(!AddressRange(13, 0).intersects(AddressRange(7, 27)));
    BOOST_CHECK(!AddressRange(0, 27).intersects(AddressRange(13, 7)));
    BOOST_CHECK(!AddressRange(0, 13).intersects(AddressRange(27, 7)));    
    BOOST_CHECK(!AddressRange(27, 0).intersects(AddressRange(13, 7)));
    BOOST_CHECK(!AddressRange(13, 0).intersects(AddressRange(27, 7)));
    
    BOOST_CHECK_EQUAL(static_cast<std::string>(AddressRange(13, 27)),
                      "[0x000000000000000D, 0x000000000000001B]");

    BOOST_CHECK(!std::less<AddressRange>()(
                    AddressRange(0, 13), AddressRange(7, 27)
                    ));
    BOOST_CHECK(!std::less<AddressRange>()(
                    AddressRange(7, 27), AddressRange(0, 13)
                    ));
    BOOST_CHECK(!std::less<AddressRange>()(
                    AddressRange(7, 13), AddressRange(7, 27)
                    ));
    BOOST_CHECK(!std::less<AddressRange>()(
                    AddressRange(7, 27), AddressRange(7, 13)
                    ));
    BOOST_CHECK(!std::less<AddressRange>()(
                    AddressRange(0, 7), AddressRange(7, 27)
                    ));
    BOOST_CHECK(!std::less<AddressRange>()(
                    AddressRange(7, 27), AddressRange(0, 7)
                    ));
    BOOST_CHECK(std::less<AddressRange>()(
                    AddressRange(0, 7), AddressRange(13, 27)
                    ));
    BOOST_CHECK(!std::less<AddressRange>()(
                    AddressRange(13, 27), AddressRange(0, 7)
                    ));
}



/**
 * Unit test for the AddressSpace class.
 */
BOOST_AUTO_TEST_CASE(TestAddressSpace)
{
    ThreadName thread1("nonexistenthost.com", 27);
    ThreadName thread2("anothernonexistenthost.com", 13);

    LinkedObject linked_object1(FileName("/path/to/nonexistent/executable/1"));
    LinkedObject linked_object2(FileName("/path/to/nonexistent/dso/1"));
    LinkedObject linked_object3(FileName("/path/to/nonexistent/dso/2"));
    LinkedObject linked_object4(FileName("/path/to/nonexistent/dso/3"));
    LinkedObject linked_object5(FileName("/path/to/nonexistent/executable/2"));

    AddressSpaces address_spaces;

    std::set<LinkedObject> linked_objects;
    address_spaces.visitLinkedObjects(
        boost::bind(accumulate<LinkedObject>, _1, boost::ref(linked_objects))
        );
    BOOST_CHECK(linked_objects.empty());

    std::map<LinkedObject, std::vector<boost::uint64_t> > mappings;
    address_spaces.visitMappings(
        boost::bind(accumulateMappings, _1, _2, _3, _4, boost::ref(mappings))
        );
    BOOST_CHECK(mappings.empty());

    //
    // The following linked objects (along with their corresponding address
    // ranges and time intervals) are added to this address space for the test:
    //
    //                               AddressRange  TimeInterval
    //     thread1, linked_object1:    [  0,   7]    [ TB,  TE]
    //     thread1, linked_object2:    [ 13,  27]    [ TB,   7]
    //     thread1, linked_object3:    [ 13, 113]    [ 13,  27]
    //     thread1, linked_object4:    [213, 227]    [ 13,  TE]
    //     thread2, linked_object5:    [  0,   7]    [ 13,  27]
    //
    // where "TB" is the earliest possible time, and "TE" is the latest possible
    // time.

    //
    // Test loading linked objects and the AddressSpace::visitLinkedObjects
    // and visitMappings queries.
    //
    
    address_spaces.load(
        thread1, linked_object1, AddressRange(0, 7)
        );
    address_spaces.load(
        thread1, linked_object2, AddressRange(13, 27)
        );
    address_spaces.load(
        thread1, linked_object3, AddressRange(13, 113), Time(13)
        );
    
    linked_objects.clear();
    address_spaces.visitLinkedObjects(
        boost::bind(accumulate<LinkedObject>, _1, boost::ref(linked_objects))
        );
    BOOST_CHECK_EQUAL(linked_objects.size(), 3);
    BOOST_CHECK(linked_objects.find(linked_object1) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object2) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object3) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object4) == linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object5) == linked_objects.end());
    
    mappings.clear();
    address_spaces.visitMappings(
        boost::bind(accumulateMappings, _1, _2, _3, _4, boost::ref(mappings))
        );
    BOOST_CHECK_EQUAL(mappings.size(), 3);
    BOOST_CHECK(mappings.find(linked_object1) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object2) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object3) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object4) == mappings.end());
    BOOST_CHECK(mappings.find(linked_object5) == mappings.end());

    std::vector<boost::uint64_t> where = mappings.find(linked_object3)->second;
    BOOST_CHECK_EQUAL(where[0], 13);
    BOOST_CHECK_EQUAL(where[1], 113);
    BOOST_CHECK_EQUAL(where[2], 13);
    BOOST_CHECK_EQUAL(where[3], Time::TheEnd());
    
    mappings.clear();
    address_spaces.visitMappings(
        thread1,
        AddressRange(14, 27),
        TimeInterval(Time::TheBeginning(), Time::TheEnd()),
        boost::bind(accumulateMappings, _1, _2, _3, _4, boost::ref(mappings))
        );
    BOOST_CHECK_EQUAL(mappings.size(), 2);
    BOOST_CHECK(mappings.find(linked_object1) == mappings.end());
    BOOST_CHECK(mappings.find(linked_object2) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object3) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object4) == mappings.end());
    BOOST_CHECK(mappings.find(linked_object5) == mappings.end());

    //
    // Test unloading linked objects and the AddressSpace::visitLinkedObjects
    // and visitMappings queries.
    //

    address_spaces.unload(thread1, linked_object1);
    address_spaces.unload(thread1, linked_object2, Time(7));
    address_spaces.unload(thread1, linked_object3, Time(27));
    
    linked_objects.clear();
    address_spaces.visitLinkedObjects(
        boost::bind(accumulate<LinkedObject>, _1, boost::ref(linked_objects))
        );
    BOOST_CHECK_EQUAL(linked_objects.size(), 3);
    BOOST_CHECK(linked_objects.find(linked_object1) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object2) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object3) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object4) == linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object5) == linked_objects.end());

    mappings.clear();
    address_spaces.visitMappings(
        boost::bind(accumulateMappings, _1, _2, _3, _4, boost::ref(mappings))
        );
    BOOST_CHECK_EQUAL(mappings.size(), 3);
    BOOST_CHECK(mappings.find(linked_object1) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object2) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object3) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object4) == mappings.end());
    BOOST_CHECK(mappings.find(linked_object5) == mappings.end());

    where = mappings.find(linked_object3)->second;
    BOOST_CHECK_EQUAL(where[0], 13);
    BOOST_CHECK_EQUAL(where[1], 113);
    BOOST_CHECK_EQUAL(where[2], 13);
    BOOST_CHECK_EQUAL(where[3], 27);

    mappings.clear();
    address_spaces.visitMappings(
        thread1,
        AddressRange(14, 27),
        TimeInterval(Time::TheBeginning(), 7),
        boost::bind(accumulateMappings, _1, _2, _3, _4, boost::ref(mappings))
        );
    BOOST_CHECK_EQUAL(mappings.size(), 1);
    BOOST_CHECK(mappings.find(linked_object1) == mappings.end());
    BOOST_CHECK(mappings.find(linked_object2) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object3) == mappings.end());
    BOOST_CHECK(mappings.find(linked_object4) == mappings.end());
    BOOST_CHECK(mappings.find(linked_object5) == mappings.end());

    //
    // Test the application of CBTF_Protocol_LinkedObjectGroup messages.
    //

    CBTF_Protocol_LinkedObjectGroup group_message;

    group_message.thread = thread2;

    group_message.linkedobjects.linkedobjects_len = 1;
    group_message.linkedobjects.linkedobjects_val = 
        reinterpret_cast<CBTF_Protocol_LinkedObject*>(
            malloc(sizeof(CBTF_Protocol_LinkedObject))
            );
    group_message.linkedobjects.linkedobjects_val[0].linked_object =
        linked_object5.file();
    group_message.linkedobjects.linkedobjects_val[0].range.begin = 0;
    group_message.linkedobjects.linkedobjects_val[0].range.end = 7 + 1;
    group_message.linkedobjects.linkedobjects_val[0].time_begin = 13;
    group_message.linkedobjects.linkedobjects_val[0].time_end = 27 + 1;

    address_spaces.apply(group_message);

    linked_objects.clear();
    address_spaces.visitLinkedObjects(
        boost::bind(accumulate<LinkedObject>, _1, boost::ref(linked_objects))
        );
    for (std::set<LinkedObject>::const_iterator
             i = linked_objects.begin(); i != linked_objects.end(); ++i)
    {
        if (i->file() == linked_object5.file())
        {
            linked_object5 = *i;
        }
    }
    BOOST_CHECK_EQUAL(linked_objects.size(), 4);
    BOOST_CHECK(linked_objects.find(linked_object1) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object2) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object3) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object4) == linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object5) != linked_objects.end());

    mappings.clear();
    address_spaces.visitMappings(
        thread2,
        boost::bind(accumulateMappings, _1, _2, _3, _4, boost::ref(mappings))
        );
    BOOST_CHECK_EQUAL(mappings.size(), 1);
    BOOST_CHECK(mappings.find(linked_object1) == mappings.end());
    BOOST_CHECK(mappings.find(linked_object2) == mappings.end());
    BOOST_CHECK(mappings.find(linked_object3) == mappings.end());
    BOOST_CHECK(mappings.find(linked_object4) == mappings.end());
    BOOST_CHECK(mappings.find(linked_object5) != mappings.end());

    where = mappings.find(linked_object5)->second;
    BOOST_CHECK_EQUAL(where[0], 0);
    BOOST_CHECK_EQUAL(where[1], 7);
    BOOST_CHECK_EQUAL(where[2], 13);
    BOOST_CHECK_EQUAL(where[3], 27);
    
    //
    // Test the application of CBTF_Protocol_[Unl|L]oadedLinkedObject messages.
    //

    CBTF_Protocol_LoadedLinkedObject loaded_message;
    
    loaded_message.threads.names.names_len = 1;
    loaded_message.threads.names.names_val = 
        reinterpret_cast<CBTF_Protocol_ThreadName*>(
            malloc(sizeof(CBTF_Protocol_ThreadName))
            );
    loaded_message.threads.names.names_val[0] = thread1;
    loaded_message.time = 13;
    loaded_message.range.begin = 213;
    loaded_message.range.end = 227 + 1;
    loaded_message.linked_object = linked_object4.file();
    
    CBTF_Protocol_UnloadedLinkedObject unloaded_message;

    unloaded_message.threads.names.names_len = 1;
    unloaded_message.threads.names.names_val = 
        reinterpret_cast<CBTF_Protocol_ThreadName*>(
            malloc(sizeof(CBTF_Protocol_ThreadName))
            );
    unloaded_message.threads.names.names_val[0] = thread1;
    unloaded_message.time = Time::TheEnd();
    unloaded_message.linked_object = linked_object4.file();
    
    address_spaces.apply(loaded_message);
    address_spaces.apply(unloaded_message);

    linked_objects.clear();
    address_spaces.visitLinkedObjects(
        boost::bind(accumulate<LinkedObject>, _1, boost::ref(linked_objects))
        );
    for (std::set<LinkedObject>::const_iterator
             i = linked_objects.begin(); i != linked_objects.end(); ++i)
    {
        if (i->file() == linked_object4.file())
        {
            linked_object4 = *i;
        }
    }
    BOOST_CHECK_EQUAL(linked_objects.size(), 5);
    BOOST_CHECK(linked_objects.find(linked_object1) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object2) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object3) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object4) != linked_objects.end());
    BOOST_CHECK(linked_objects.find(linked_object5) != linked_objects.end());

    mappings.clear();
    address_spaces.visitMappings(
        boost::bind(accumulateMappings, _1, _2, _3, _4, boost::ref(mappings))
        );
    BOOST_CHECK_EQUAL(mappings.size(), 5);
    BOOST_CHECK(mappings.find(linked_object1) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object2) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object3) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object4) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object5) != mappings.end());

    where = mappings.find(linked_object4)->second;
    BOOST_CHECK_EQUAL(where[0], 213);
    BOOST_CHECK_EQUAL(where[1], 227);
    BOOST_CHECK_EQUAL(where[2], 13);
    BOOST_CHECK_EQUAL(where[3], Time::TheEnd());

    mappings.clear();
    address_spaces.visitMappings(
        thread1,
        AddressRange(Address::TheLowest(), Address::TheHighest()),
        TimeInterval(13, 27),
        boost::bind(accumulateMappings, _1, _2, _3, _4, boost::ref(mappings))
        );
    BOOST_CHECK_EQUAL(mappings.size(), 3);
    BOOST_CHECK(mappings.find(linked_object1) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object2) == mappings.end());
    BOOST_CHECK(mappings.find(linked_object3) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object4) != mappings.end());
    BOOST_CHECK(mappings.find(linked_object5) == mappings.end());
}



/**
 * Unit test for the FileName class.
 */
BOOST_AUTO_TEST_CASE(TestFileName)
{    
    FileName name1("/path/to/nonexistent/file");
    
    BOOST_CHECK_EQUAL(name1.path(), "/path/to/nonexistent/file");
    BOOST_CHECK_EQUAL(name1.checksum(), 0);

    BOOST_CHECK_EQUAL(static_cast<std::string>(name1),
                      "0x0000000000000000: \"/path/to/nonexistent/file\"");
    
#if defined(BOOST_FILESYSTEM_VERSION) && (BOOST_FILESYSTEM_VERSION == 3)
    boost::filesystem::path tmp_path = boost::filesystem::unique_path();    
#else
    char model[] = "test.XXXXXX";
    BOOST_REQUIRE_EQUAL(mkstemp(model), 0);
    boost::filesystem::path tmp_path(model);
#endif

    boost::filesystem::ofstream stream(tmp_path);
    stream << "Four score and seven years ago our fathers brought forth "
           << "on this continent a new nation, conceived in liberty, and "
           << "dedicated to the proposition that all men are created equal."
           << std::endl << std::endl
           << "Now we are engaged in a great civil war, testing whether "
           << "that nation, or any nation so conceived and so dedicated, "
           << "can long endure. We are met on a great battlefield of that "
           << "war. We have come to dedicate a portion of that field, as "
           << "a final resting place for those who here gave their lives "
           << "that that nation might live. It is altogether fitting and "
           << "proper that we should do this."
           << std::endl << std::endl
           << "But, in a larger sense, we can not dedicate, we can not "
           << "consecrate, we can not hallow this ground. The brave men, "
           << "living and dead, who struggled here, have consecrated it, "
           << "far above our poor power to add or detract. The world will "
           << "little note, nor long remember what we say here, but it can "
           << "never forget what they did here. It is for us the living, "
           << "rather, to be dedicated here to the unfinished work which "
           << "they who fought here have thus far so nobly advanced. It is "
           << "rather for us to be here dedicated to the great task remaining "
           << "before us - that from these honored dead we take increased "
           << "devotion to that cause for which they gave the last full "
           << "measure of devotion - that we here highly resolve that these "
           << "dead shall not have died in vain - that this nation, under "
           << "God, shall have a new birth of freedom - and that government "
           << "of the people, by the people, for the people, shall not perish "
           << "from the earth."
           << std::endl;
    stream.close();
    FileName name2 = FileName(tmp_path);

    BOOST_CHECK_EQUAL(name2.path(), tmp_path);
    BOOST_CHECK_EQUAL(name2.checksum(), 17734875587178274082llu);

    stream.open(tmp_path, std::ios::app);
    stream << std::endl
           << "-- President Abraham Lincoln, November 19, 1863"
           << std::endl;
    stream.close();
    FileName name3 = FileName(tmp_path);

    BOOST_CHECK_EQUAL(name3.path(), tmp_path);
    BOOST_CHECK_EQUAL(name3.checksum(), 1506913182069408458llu);
    
    boost::filesystem::remove(tmp_path);

    BOOST_CHECK_NE(name1, name2);
    BOOST_CHECK_NE(name1, name3);
    BOOST_CHECK_NE(name2, name3);

    BOOST_CHECK_LT(name3, name2);
    BOOST_CHECK_GT(name2, name3);

    BOOST_CHECK_EQUAL(
        FileName(static_cast<CBTF_Protocol_FileName>(name1)), name1
        );
    BOOST_CHECK_EQUAL(
        FileName(static_cast<CBTF_Protocol_FileName>(name2)), name2
        );
}



/**
 * Unit test for the LinkedObject, Function, Loop, and Statement classes.
 */
BOOST_AUTO_TEST_CASE(TestSymbolTable)
{
    std::set<AddressRange> addresses;
    
    LinkedObject linked_object(FileName("/path/to/nonexistent/dso"));
    
    BOOST_CHECK_EQUAL(LinkedObject(linked_object), linked_object);
    BOOST_CHECK_EQUAL(linked_object.file(),
                      FileName("/path/to/nonexistent/dso"));

    //
    // The following functions, loops, and statements (along with their listed
    // address ranges) are added to this linked object during the test:
    //
    //      function1:  [  0,   7]  [ 13,  27]
    //      function2:  [113, 127]
    //      function3:  [  7,  13]  [213, 227]
    //      function4:  [ 57,  63]
    //
    //          loop1:  [ 13,  27]              {Head = 13}
    //          loop2:  [  0,   7]  [110, 130]  {Head =  0}
    //          loop3:  [ 13, 100]              {Head = 13}
    //
    //     statement1:  [  0,   7]  [113, 127]
    //     statement2:  [ 13,  27]
    //     statement3:  [ 75, 100]
    //     statement4:  [213, 227]
    //

    //
    // Test adding functions and the LinkedObject::visitFunctions query.
    //

    std::set<Function> functions;
    linked_object.visitFunctions(
        boost::bind(accumulate<Function>, _1, boost::ref(functions))
        );
    BOOST_CHECK(functions.empty());

    Function function1(linked_object, "_Z2f1RKf");

    BOOST_CHECK_EQUAL(Function(function1), function1);
    BOOST_CHECK_EQUAL(function1.getLinkedObject(), linked_object);
    BOOST_CHECK_EQUAL(function1.getMangledName(), "_Z2f1RKf");
    BOOST_CHECK_EQUAL(function1.getDemangledName(), "f1(float const&)");
    BOOST_CHECK(function1.ranges().empty());

    Function function2(linked_object, "_Z2f2RKf");
    Function function3(linked_object, "_Z2f3RKf");
    Function function4(function3.clone(linked_object));

    BOOST_CHECK_NE(function1, function2);
    BOOST_CHECK_LT(function1, function2);
    BOOST_CHECK_GT(function3, function2);
    BOOST_CHECK_NE(function3, function4);

    BOOST_CHECK(!ArgoNavis::Base::equivalent(function1, function2));
    BOOST_CHECK(ArgoNavis::Base::equivalent(function3, function4));

    functions.clear();
    linked_object.visitFunctions(
        boost::bind(accumulate<Function>, _1, boost::ref(functions))
        );
    BOOST_CHECK_EQUAL(functions.size(), 4);

    //
    // Test adding statements and the LinkedObject::visitStatements query.
    //
    
    std::set<Statement> statements;
    linked_object.visitStatements(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK(statements.empty());
    
    Statement statement1(linked_object,
                         FileName("/path/to/nonexistent/source/file"), 1, 1);

    BOOST_CHECK_EQUAL(Statement(statement1), statement1);
    BOOST_CHECK_EQUAL(statement1.getLinkedObject(), linked_object);
    BOOST_CHECK_EQUAL(statement1.getFile(),
                      FileName("/path/to/nonexistent/source/file"));
    BOOST_CHECK_EQUAL(statement1.getLine(), 1);
    BOOST_CHECK_EQUAL(statement1.getColumn(), 1);
    BOOST_CHECK(statement1.ranges().empty());

    Statement statement2(linked_object,
                         FileName("/path/to/nonexistent/source/file"), 20, 1);
    Statement statement3(linked_object,
                         FileName("/path/to/nonexistent/source/file"), 30, 1);
    Statement statement4(statement3.clone(linked_object));
    
    BOOST_CHECK_NE(statement1, statement2);
    BOOST_CHECK_LT(statement1, statement2);
    BOOST_CHECK_GT(statement3, statement2);
    BOOST_CHECK_NE(statement3, statement4);

    BOOST_CHECK(!ArgoNavis::Base::equivalent(statement1, statement2));
    BOOST_CHECK(ArgoNavis::Base::equivalent(statement3, statement4));

    statements.clear();
    linked_object.visitStatements(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 4);

    //
    // Add address ranges to the functions.
    //

    addresses = boost::assign::list_of
        (AddressRange(0, 7))
        (AddressRange(13, 27));
    function1.add(addresses);

    addresses = boost::assign::list_of
        (AddressRange(113, 127));
    function2.add(addresses);

    addresses = boost::assign::list_of
        (AddressRange(7, 13))
        (AddressRange(213, 227));
    function3.add(addresses);
    
    addresses = boost::assign::list_of
        (AddressRange(57, 63));
    function4.add(addresses);
    
    BOOST_CHECK(!function1.ranges().empty());
    BOOST_CHECK(!function2.ranges().empty());
    BOOST_CHECK(!function3.ranges().empty());
    BOOST_CHECK(!function4.ranges().empty());

    //
    // Test the LinkedObject::visitFunctions(<address_range>) query.
    //

    functions.clear();
    linked_object.visitFunctions(
        AddressRange(0, 10),
        boost::bind(accumulate<Function>, _1, boost::ref(functions))
        );
    BOOST_CHECK_EQUAL(functions.size(), 2);
    BOOST_CHECK(functions.find(function1) != functions.end());
    BOOST_CHECK(functions.find(function2) == functions.end());
    BOOST_CHECK(functions.find(function3) != functions.end());
    BOOST_CHECK(functions.find(function4) == functions.end());

    functions.clear();
    linked_object.visitFunctions(
        AddressRange(200, 400),
        boost::bind(accumulate<Function>, _1, boost::ref(functions))
        );
    BOOST_CHECK_EQUAL(functions.size(), 1);
    BOOST_CHECK(functions.find(function1) == functions.end());
    BOOST_CHECK(functions.find(function2) == functions.end());
    BOOST_CHECK(functions.find(function3) != functions.end());
    BOOST_CHECK(functions.find(function4) == functions.end());

    functions.clear();
    linked_object.visitFunctions(
        AddressRange(28, 30),
        boost::bind(accumulate<Function>, _1, boost::ref(functions))
        );
    BOOST_CHECK(functions.empty());

    //
    // Add address ranges to the statements.
    //

    addresses = boost::assign::list_of
        (AddressRange(0, 7))
        (AddressRange(113, 127));
    statement1.add(addresses);

    addresses = boost::assign::list_of
        (AddressRange(13, 27));
    statement2.add(addresses);

    addresses = boost::assign::list_of
        (AddressRange(75, 100));
    statement3.add(addresses);
    
    addresses = boost::assign::list_of
        (AddressRange(213, 227));
    statement4.add(addresses);

    BOOST_CHECK(!statement1.ranges().empty());
    BOOST_CHECK(!statement2.ranges().empty());
    BOOST_CHECK(!statement3.ranges().empty());
    BOOST_CHECK(!statement4.ranges().empty());

    //
    // Test the LinkedObject::visitStatements(<address-range>) query.
    //

    statements.clear();
    linked_object.visitStatements(
        AddressRange(0, 20),
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 2);
    BOOST_CHECK(statements.find(statement1) != statements.end());
    BOOST_CHECK(statements.find(statement2) != statements.end());
    BOOST_CHECK(statements.find(statement3) == statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());

    statements.clear();
    linked_object.visitStatements(
        AddressRange(90, 110),
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 1);
    BOOST_CHECK(statements.find(statement1) == statements.end());
    BOOST_CHECK(statements.find(statement2) == statements.end());
    BOOST_CHECK(statements.find(statement3) != statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());

    statements.clear();
    linked_object.visitStatements(
        AddressRange(30, 40),
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK(statements.empty());

    //
    // Test the Function::visitDefinitions() query.
    //

    statements.clear();
    function1.visitDefinitions(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 1);
    BOOST_CHECK(statements.find(statement1) != statements.end());
    BOOST_CHECK(statements.find(statement2) == statements.end());
    BOOST_CHECK(statements.find(statement3) == statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());

    statements.clear();
    function2.visitDefinitions(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 1);
    BOOST_CHECK(statements.find(statement1) != statements.end());
    BOOST_CHECK(statements.find(statement2) == statements.end());
    BOOST_CHECK(statements.find(statement3) == statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());

    statements.clear();
    function3.visitDefinitions(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 1);
    BOOST_CHECK(statements.find(statement1) != statements.end());
    BOOST_CHECK(statements.find(statement2) == statements.end());
    BOOST_CHECK(statements.find(statement3) == statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());

    statements.clear();
    function4.visitDefinitions(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK(statements.empty());

    //
    // Test the Function::visitStatements() query.
    //

    statements.clear();
    function1.visitStatements(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 2);
    BOOST_CHECK(statements.find(statement1) != statements.end());
    BOOST_CHECK(statements.find(statement2) != statements.end());
    BOOST_CHECK(statements.find(statement3) == statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());

    statements.clear();
    function2.visitStatements(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 1);
    BOOST_CHECK(statements.find(statement1) != statements.end());
    BOOST_CHECK(statements.find(statement2) == statements.end());
    BOOST_CHECK(statements.find(statement3) == statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());

    statements.clear();
    function3.visitStatements(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 3);
    BOOST_CHECK(statements.find(statement1) != statements.end());
    BOOST_CHECK(statements.find(statement2) != statements.end());
    BOOST_CHECK(statements.find(statement3) == statements.end());
    BOOST_CHECK(statements.find(statement4) != statements.end());
    
    statements.clear();
    function4.visitStatements(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK(statements.empty());

    //
    // Test the Statement::visitFunctions() query.
    //

    functions.clear();
    statement1.visitFunctions(
        boost::bind(accumulate<Function>, _1, boost::ref(functions))
        );
    BOOST_CHECK_EQUAL(functions.size(), 3);
    BOOST_CHECK(functions.find(function1) != functions.end());
    BOOST_CHECK(functions.find(function2) != functions.end());
    BOOST_CHECK(functions.find(function3) != functions.end());
    BOOST_CHECK(functions.find(function4) == functions.end());

    functions.clear();
    statement2.visitFunctions(
        boost::bind(accumulate<Function>, _1, boost::ref(functions))
        );
    BOOST_CHECK_EQUAL(functions.size(), 2);
    BOOST_CHECK(functions.find(function1) != functions.end());
    BOOST_CHECK(functions.find(function2) == functions.end());
    BOOST_CHECK(functions.find(function3) != functions.end());
    BOOST_CHECK(functions.find(function4) == functions.end());

    functions.clear();
    statement3.visitFunctions(
        boost::bind(accumulate<Function>, _1, boost::ref(functions))
        );
    BOOST_CHECK(functions.empty());

    functions.clear();
    statement4.visitFunctions(
        boost::bind(accumulate<Function>, _1, boost::ref(functions))
        );
    BOOST_CHECK_EQUAL(functions.size(), 1);
    BOOST_CHECK(functions.find(function1) == functions.end());
    BOOST_CHECK(functions.find(function2) == functions.end());
    BOOST_CHECK(functions.find(function3) != functions.end());
    BOOST_CHECK(functions.find(function4) == functions.end());

    //
    // Test the conversion of LinkedObject to/from CBTF_Protocol_SymbolTable.
    // This is done before loops are added because that message doesn't contain
    // loop information, and the equivalent() check below would fail.
    //
    
    BOOST_CHECK(ArgoNavis::Base::equivalent(
        LinkedObject(static_cast<CBTF_Protocol_SymbolTable>(linked_object)),
        linked_object
        ));

    //
    // Test adding loops and the LinkedObject::visitLoops query.
    //

    std::set<Loop> loops;
    linked_object.visitLoops(
        boost::bind(accumulate<Loop>, _1, boost::ref(loops))
        );
    BOOST_CHECK(loops.empty());

    Loop loop1(linked_object, Address(13));

    BOOST_CHECK_EQUAL(Loop(loop1), loop1);
    BOOST_CHECK_EQUAL(loop1.getLinkedObject(), linked_object);
    BOOST_CHECK_EQUAL(loop1.getHeadAddress(), Address(13));
    BOOST_CHECK(loop1.ranges().empty());

    Loop loop2(linked_object, Address(0));
    Loop loop3(loop1.clone(linked_object));

    BOOST_CHECK_NE(loop1, loop2);
    BOOST_CHECK_LT(loop1, loop2);
    BOOST_CHECK_GT(loop3, loop2);
    BOOST_CHECK_NE(loop1, loop3);

    BOOST_CHECK(!ArgoNavis::Base::equivalent(loop1, loop2));
    BOOST_CHECK(ArgoNavis::Base::equivalent(loop1, loop3));

    loops.clear();
    linked_object.visitLoops(
        boost::bind(accumulate<Loop>, _1, boost::ref(loops))
        );
    BOOST_CHECK_EQUAL(loops.size(), 3);

    //
    // Add address ranges to the loops.
    //

    addresses = boost::assign::list_of
        (AddressRange(13, 27));
    loop1.add(addresses);

    addresses = boost::assign::list_of
        (AddressRange(0, 7))
        (AddressRange(110, 130));
    loop2.add(addresses);

    addresses = boost::assign::list_of
        (AddressRange(13, 100));
    loop3.add(addresses);

    BOOST_CHECK(!loop1.ranges().empty());
    BOOST_CHECK(!loop2.ranges().empty());
    BOOST_CHECK(!loop3.ranges().empty());

    //
    // Test the LinkedObject::visitLoops(<address_range>) query.
    //

    loops.clear();
    linked_object.visitLoops(
        AddressRange(30, 120),
        boost::bind(accumulate<Loop>, _1, boost::ref(loops))
        );
    BOOST_CHECK_EQUAL(loops.size(), 2);
    BOOST_CHECK(loops.find(loop1) == loops.end());
    BOOST_CHECK(loops.find(loop2) != loops.end());
    BOOST_CHECK(loops.find(loop3) != loops.end());

    loops.clear();
    linked_object.visitLoops(
        AddressRange(101, 105),
        boost::bind(accumulate<Loop>, _1, boost::ref(loops))
        );
    BOOST_CHECK(loops.empty());

    //
    // Test the Function::visitLoops() query.
    //

    loops.clear();
    function1.visitLoops(
        boost::bind(accumulate<Loop>, _1, boost::ref(loops))
        );
    BOOST_CHECK_EQUAL(loops.size(), 3);
    BOOST_CHECK(loops.find(loop1) != loops.end());
    BOOST_CHECK(loops.find(loop2) != loops.end());
    BOOST_CHECK(loops.find(loop3) != loops.end());
    
    loops.clear();
    function2.visitLoops(
        boost::bind(accumulate<Loop>, _1, boost::ref(loops))
        );
    BOOST_CHECK_EQUAL(loops.size(), 1);
    BOOST_CHECK(loops.find(loop1) == loops.end());
    BOOST_CHECK(loops.find(loop2) != loops.end());
    BOOST_CHECK(loops.find(loop3) == loops.end());
    
    loops.clear();
    function3.visitLoops(
        boost::bind(accumulate<Loop>, _1, boost::ref(loops))
        );
    BOOST_CHECK_EQUAL(loops.size(), 3);
    BOOST_CHECK(loops.find(loop1) != loops.end());
    BOOST_CHECK(loops.find(loop2) != loops.end());
    BOOST_CHECK(loops.find(loop3) != loops.end());
    
    loops.clear();
    function4.visitLoops(
        boost::bind(accumulate<Loop>, _1, boost::ref(loops))
        );
    BOOST_CHECK_EQUAL(loops.size(), 1);
    BOOST_CHECK(loops.find(loop1) == loops.end());
    BOOST_CHECK(loops.find(loop2) == loops.end());
    BOOST_CHECK(loops.find(loop3) != loops.end());

    //
    // Test the Statement::visitLoops() query.
    //

    loops.clear();
    statement1.visitLoops(
        boost::bind(accumulate<Loop>, _1, boost::ref(loops))
        );
    BOOST_CHECK_EQUAL(loops.size(), 1);
    BOOST_CHECK(loops.find(loop1) == loops.end());
    BOOST_CHECK(loops.find(loop2) != loops.end());
    BOOST_CHECK(loops.find(loop3) == loops.end());
    
    loops.clear();
    statement2.visitLoops(
        boost::bind(accumulate<Loop>, _1, boost::ref(loops))
        );
    BOOST_CHECK_EQUAL(loops.size(), 2);
    BOOST_CHECK(loops.find(loop1) != loops.end());
    BOOST_CHECK(loops.find(loop2) == loops.end());
    BOOST_CHECK(loops.find(loop3) != loops.end());
    
    loops.clear();
    statement3.visitLoops(
        boost::bind(accumulate<Loop>, _1, boost::ref(loops))
        );
    BOOST_CHECK_EQUAL(loops.size(), 1);
    BOOST_CHECK(loops.find(loop1) == loops.end());
    BOOST_CHECK(loops.find(loop2) == loops.end());
    BOOST_CHECK(loops.find(loop3) != loops.end());
    
    loops.clear();
    statement4.visitLoops(
        boost::bind(accumulate<Loop>, _1, boost::ref(loops))
        );
    BOOST_CHECK(loops.empty());
    
    //
    // Test the Loop::visitDefinitions() query.
    //

    statements.clear();
    loop1.visitDefinitions(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 1);
    BOOST_CHECK(statements.find(statement1) == statements.end());
    BOOST_CHECK(statements.find(statement2) != statements.end());
    BOOST_CHECK(statements.find(statement3) == statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());

    statements.clear();
    loop2.visitDefinitions(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 1);
    BOOST_CHECK(statements.find(statement1) != statements.end());
    BOOST_CHECK(statements.find(statement2) == statements.end());
    BOOST_CHECK(statements.find(statement3) == statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());

    statements.clear();
    loop3.visitDefinitions(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 1);
    BOOST_CHECK(statements.find(statement1) == statements.end());
    BOOST_CHECK(statements.find(statement2) != statements.end());
    BOOST_CHECK(statements.find(statement3) == statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());

    //
    // Test the Loop::visitFunctions() query.
    //

    functions.clear();
    loop1.visitFunctions(
        boost::bind(accumulate<Function>, _1, boost::ref(functions))
        );
    BOOST_CHECK_EQUAL(functions.size(), 2);
    BOOST_CHECK(functions.find(function1) != functions.end());
    BOOST_CHECK(functions.find(function2) == functions.end());
    BOOST_CHECK(functions.find(function3) != functions.end());
    BOOST_CHECK(functions.find(function4) == functions.end());

    functions.clear();
    loop2.visitFunctions(
        boost::bind(accumulate<Function>, _1, boost::ref(functions))
        );
    BOOST_CHECK_EQUAL(functions.size(), 3);
    BOOST_CHECK(functions.find(function1) != functions.end());
    BOOST_CHECK(functions.find(function2) != functions.end());
    BOOST_CHECK(functions.find(function3) != functions.end());
    BOOST_CHECK(functions.find(function4) == functions.end());

    functions.clear();
    loop3.visitFunctions(
        boost::bind(accumulate<Function>, _1, boost::ref(functions))
        );
    BOOST_CHECK_EQUAL(functions.size(), 3);
    BOOST_CHECK(functions.find(function1) != functions.end());
    BOOST_CHECK(functions.find(function2) == functions.end());
    BOOST_CHECK(functions.find(function3) != functions.end());
    BOOST_CHECK(functions.find(function4) != functions.end());

    //
    // Test the Loop::visitStatements() query.
    //
    
    statements.clear();
    loop1.visitStatements(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 1);
    BOOST_CHECK(statements.find(statement1) == statements.end());
    BOOST_CHECK(statements.find(statement2) != statements.end());
    BOOST_CHECK(statements.find(statement3) == statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());

    statements.clear();
    loop2.visitStatements(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 1);
    BOOST_CHECK(statements.find(statement1) != statements.end());
    BOOST_CHECK(statements.find(statement2) == statements.end());
    BOOST_CHECK(statements.find(statement3) == statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());

    statements.clear();
    loop3.visitStatements(
        boost::bind(accumulate<Statement>, _1, boost::ref(statements))
        );
    BOOST_CHECK_EQUAL(statements.size(), 2);
    BOOST_CHECK(statements.find(statement1) == statements.end());
    BOOST_CHECK(statements.find(statement2) != statements.end());
    BOOST_CHECK(statements.find(statement3) != statements.end());
    BOOST_CHECK(statements.find(statement4) == statements.end());    

    //
    // Test LinkedObject::clone().
    //
    
    LinkedObject clone = linked_object.clone();
    
    BOOST_CHECK((clone < linked_object) || (linked_object < clone));
    BOOST_CHECK_NE(clone, linked_object);

    BOOST_CHECK(ArgoNavis::Base::equivalent(clone, linked_object));
    Function function5(clone, "_Z2f5RKf");
    BOOST_CHECK(!ArgoNavis::Base::equivalent(clone, linked_object));
}



/**
 * Unit test for the ThreadName class.
 */
BOOST_AUTO_TEST_CASE(TestThreadName)
{
    ThreadName name1("first.host", 13);
    ThreadName name2("first.host", 13, 27);
    ThreadName name3("first.host", 13, 2002);
    ThreadName name4("first.host", 13, 2002, 2004);
    ThreadName name5("first.host", 13, 2002, boost::none, 911);
    ThreadName name6("second.host", 13);

    BOOST_CHECK_EQUAL(name1.host(), name2.host());
    BOOST_CHECK_EQUAL(name2.host(), name3.host());
    BOOST_CHECK_EQUAL(name3.host(), name4.host());
    BOOST_CHECK_EQUAL(name4.host(), name5.host());
    BOOST_CHECK_NE(name5.host(), name6.host());

    BOOST_CHECK_EQUAL(name1.pid(), name2.pid());
    BOOST_CHECK_EQUAL(name2.pid(), name3.pid());
    BOOST_CHECK_EQUAL(name3.pid(), name4.pid());
    BOOST_CHECK_EQUAL(name4.pid(), name5.pid());
    BOOST_CHECK_EQUAL(name5.pid(), name6.pid());

    BOOST_CHECK_EQUAL(static_cast<bool>(name1.tid()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name2.tid()), true);
    BOOST_CHECK_EQUAL(static_cast<bool>(name3.tid()), true);
    BOOST_CHECK_EQUAL(static_cast<bool>(name4.tid()), true);
    BOOST_CHECK_EQUAL(static_cast<bool>(name5.tid()), true);
    BOOST_CHECK_EQUAL(static_cast<bool>(name6.tid()), false);
    BOOST_CHECK_NE(*name2.tid(), *name3.tid());
    BOOST_CHECK_EQUAL(*name3.tid(), *name4.tid());

    BOOST_CHECK_EQUAL(static_cast<bool>(name1.mpi_rank()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name2.mpi_rank()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name3.mpi_rank()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name4.mpi_rank()), true);
    BOOST_CHECK_EQUAL(static_cast<bool>(name5.mpi_rank()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name6.mpi_rank()), false);
    BOOST_CHECK_EQUAL(*name4.mpi_rank(), 2004);
    
    BOOST_CHECK_EQUAL(static_cast<bool>(name1.omp_rank()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name2.omp_rank()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name3.omp_rank()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name4.omp_rank()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name5.omp_rank()), true);
    BOOST_CHECK_EQUAL(static_cast<bool>(name6.omp_rank()), false);
    BOOST_CHECK_EQUAL(*name5.omp_rank(), 911);
    
    BOOST_CHECK_NE(name1, name2);
    BOOST_CHECK_NE(name2, name3);
    BOOST_CHECK_EQUAL(name3, name4);
    BOOST_CHECK_EQUAL(name4, name5);
    BOOST_CHECK_NE(name5, name6);
    BOOST_CHECK_NE(name6, name1);

    BOOST_CHECK_LT(name1, name2);
    BOOST_CHECK_LT(name2, name3);
    BOOST_CHECK_EQUAL(name3, name4);
    BOOST_CHECK_EQUAL(name4, name5);
    BOOST_CHECK_LT(name5, name6);
    BOOST_CHECK_GT(name6, name1);

    BOOST_CHECK_EQUAL(static_cast<std::string>(name1),
                      "Host \"first.host\", PID 13");
    BOOST_CHECK_EQUAL(static_cast<std::string>(name2),
                      "Host \"first.host\", PID 13, TID 0x000000000000001B");
    BOOST_CHECK_EQUAL(static_cast<std::string>(name3),
                      "Host \"first.host\", PID 13, TID 0x00000000000007D2");
    BOOST_CHECK_EQUAL(static_cast<std::string>(name4),
                      "MPI Rank 2004, TID 0x00000000000007D2");
    BOOST_CHECK_EQUAL(static_cast<std::string>(name5),
                      "Host \"first.host\", PID 13, OpenMP Rank 911");
    BOOST_CHECK_EQUAL(static_cast<std::string>(name6),
                      "Host \"second.host\", PID 13");

    BOOST_CHECK_EQUAL(
        ThreadName(static_cast<CBTF_DataHeader>(name1)), name1
        );
    BOOST_CHECK_EQUAL(
        ThreadName(static_cast<CBTF_DataHeader>(name2)), name2
        );
    BOOST_CHECK_EQUAL(
        ThreadName(static_cast<CBTF_DataHeader>(name3)), name3
        );
    BOOST_CHECK_EQUAL(
        ThreadName(static_cast<CBTF_DataHeader>(name4)), name4
        );
    BOOST_CHECK_EQUAL(
        ThreadName(static_cast<CBTF_DataHeader>(name5)), name5
        );
    BOOST_CHECK_EQUAL(
        ThreadName(static_cast<CBTF_DataHeader>(name6)), name6
        );

    BOOST_CHECK_EQUAL(
        ThreadName(static_cast<CBTF_Protocol_ThreadName>(name1)), name1
        );
    BOOST_CHECK_EQUAL(
        ThreadName(static_cast<CBTF_Protocol_ThreadName>(name2)), name2
        );
    BOOST_CHECK_EQUAL(
        ThreadName(static_cast<CBTF_Protocol_ThreadName>(name3)), name3
        );
    BOOST_CHECK_EQUAL(
        ThreadName(static_cast<CBTF_Protocol_ThreadName>(name4)), name4
        );
    BOOST_CHECK_EQUAL(
        ThreadName(static_cast<CBTF_Protocol_ThreadName>(name5)), name5
        );
    BOOST_CHECK_EQUAL(
        ThreadName(static_cast<CBTF_Protocol_ThreadName>(name6)), name6
        );
}



/**
 * Unit test for the Time class.
 */
BOOST_AUTO_TEST_CASE(TestTime)
{
    BOOST_CHECK_EQUAL(Time(), Time::TheBeginning());
    BOOST_CHECK_NE(Time::TheBeginning(), Time::TheEnd());
    BOOST_CHECK_LT(Time::TheBeginning(), Time::TheEnd());
    BOOST_CHECK_GT(Time::TheEnd(), Time::TheBeginning());

    Time t1 = Time::Now();
    BOOST_CHECK_GT(t1, Time::TheBeginning());
    BOOST_CHECK_LT(t1, Time::TheEnd());
    Time t2 = Time::Now();
    BOOST_CHECK_GT(t2, Time::TheBeginning());
    BOOST_CHECK_GT(t2, t1);
    BOOST_CHECK_LT(t2, Time::TheEnd());

    BOOST_CHECK_EQUAL(Time(static_cast<CBTF_Protocol_Time>(Time(27))),
                      Time(27));

    BOOST_CHECK_EQUAL(--Time::TheBeginning(), Time::TheEnd());
    BOOST_CHECK_EQUAL(Time::TheBeginning() - 1, Time::TheEnd());

    BOOST_CHECK_EQUAL(Time::TheBeginning(), ++Time::TheEnd());
    BOOST_CHECK_EQUAL(Time::TheBeginning(), Time::TheEnd() + 1);

    BOOST_CHECK_EQUAL(Time(27), Time(4) + Time(23));
    BOOST_CHECK_EQUAL(Time(27), Time(2) + 25);

    BOOST_CHECK_EQUAL(Time(27), Time(30) - Time(3));
    BOOST_CHECK_EQUAL(Time(27), Time(29) - 2);

    BOOST_CHECK_EQUAL(static_cast<std::string>(Time(13 * 27000000000)),
                      "1969/12/31 18:05:51");
}



/**
 * Unit test for the TimeInterval class.
 */
BOOST_AUTO_TEST_CASE(TestTimeInterval)
{
    BOOST_CHECK(TimeInterval().empty());
    BOOST_CHECK(!TimeInterval(0).empty());
    BOOST_CHECK_EQUAL(TimeInterval(27).begin(), TimeInterval(27).end());
    BOOST_CHECK(!TimeInterval(0, 1).empty());
    BOOST_CHECK(TimeInterval(1, 0).empty());

    BOOST_CHECK_EQUAL(
        TimeInterval(
            static_cast<CBTF_Protocol_TimeInterval>(TimeInterval(0, 27))
            ),
        TimeInterval(0, 27)
        );

    BOOST_CHECK_LT(TimeInterval(0, 13), TimeInterval(1, 13));
    BOOST_CHECK_LT(TimeInterval(0, 13), TimeInterval(0, 27));
    BOOST_CHECK_GT(TimeInterval(1, 13), TimeInterval(0, 13));
    BOOST_CHECK_GT(TimeInterval(0, 27), TimeInterval(0, 13));
    BOOST_CHECK_EQUAL(TimeInterval(0, 13), TimeInterval(0, 13));
    BOOST_CHECK_NE(TimeInterval(0, 13), TimeInterval(0, 27));

    BOOST_CHECK_EQUAL(TimeInterval(0, 13) | TimeInterval(7, 27),
                      TimeInterval(0, 27));
    BOOST_CHECK_EQUAL(TimeInterval(7, 27) | TimeInterval(0, 13),
                      TimeInterval(0, 27));
    BOOST_CHECK_EQUAL(TimeInterval(0, 7) | TimeInterval(13, 27),
                      TimeInterval(0, 27));
    BOOST_CHECK_EQUAL(TimeInterval(13, 27) | TimeInterval(0, 7),
                      TimeInterval(0, 27));
    BOOST_CHECK_EQUAL(TimeInterval(0, 7) | TimeInterval(),
                      TimeInterval(0, 7));
    BOOST_CHECK_EQUAL(TimeInterval() | TimeInterval(13, 27),
                      TimeInterval(13, 27));

    BOOST_CHECK_EQUAL(TimeInterval(0, 13) & TimeInterval(7, 27),
                      TimeInterval(7, 13));
    BOOST_CHECK_EQUAL(TimeInterval(7, 27) & TimeInterval(0, 13),
                      TimeInterval(7, 13));
    BOOST_CHECK((TimeInterval(0, 7) & TimeInterval(13, 27)).empty());
    BOOST_CHECK((TimeInterval(13, 27) & TimeInterval(0, 7)).empty());
    BOOST_CHECK((TimeInterval(0, 13) & TimeInterval()).empty());
    //
    // GCC 4.8.1 and 4.9.0 contain a bug that prevents them from compiling
    // the following, perfectly legal, code. The only workaround is to not
    // compile this particular check for those compilers.
    //
    // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=57532
    //   
#if !((__GNUC__ == 4 && __GNUC_MINOR__ == 8 && __GNUC_PATCHLEVEL__ == 1) || \
      (__GNUC__ == 4 && __GNUC_MINOR__ == 9 && __GNUC_PATCHLEVEL__ == 0))    
    BOOST_CHECK((TimeInterval() & TimeInterval(0, 13)).empty());
#endif
    
    BOOST_CHECK_EQUAL(TimeInterval(0, 13).width(), Time(14));
    BOOST_CHECK_EQUAL(TimeInterval(13, 0).width(), Time(0));

    BOOST_CHECK(TimeInterval(0, 13).contains(7));
    BOOST_CHECK(!TimeInterval(0, 13).contains(27));
    BOOST_CHECK(!TimeInterval(13, 0).contains(7));
    BOOST_CHECK(!TimeInterval(13, 0).contains(27));

    BOOST_CHECK(TimeInterval(0, 27).contains(TimeInterval(7, 13)));
    BOOST_CHECK(!TimeInterval(0, 13).contains(TimeInterval(7, 27)));
    BOOST_CHECK(!TimeInterval(27, 0).contains(TimeInterval(7, 13)));
    BOOST_CHECK(!TimeInterval(13, 0).contains(TimeInterval(7, 27)));
    BOOST_CHECK(!TimeInterval(0, 27).contains(TimeInterval(13, 7)));
    BOOST_CHECK(!TimeInterval(0, 13).contains(TimeInterval(27, 7)));    
    BOOST_CHECK(!TimeInterval(27, 0).contains(TimeInterval(13, 7)));
    BOOST_CHECK(!TimeInterval(13, 0).contains(TimeInterval(27, 7)));

    BOOST_CHECK(TimeInterval(0, 27).intersects(TimeInterval(7, 13)));
    BOOST_CHECK(TimeInterval(0, 13).intersects(TimeInterval(7, 27)));
    BOOST_CHECK(!TimeInterval(27, 0).intersects(TimeInterval(7, 13)));
    BOOST_CHECK(!TimeInterval(13, 0).intersects(TimeInterval(7, 27)));
    BOOST_CHECK(!TimeInterval(0, 27).intersects(TimeInterval(13, 7)));
    BOOST_CHECK(!TimeInterval(0, 13).intersects(TimeInterval(27, 7)));    
    BOOST_CHECK(!TimeInterval(27, 0).intersects(TimeInterval(13, 7)));
    BOOST_CHECK(!TimeInterval(13, 0).intersects(TimeInterval(27, 7)));
    
    BOOST_CHECK_EQUAL(static_cast<std::string>(TimeInterval(13, 27000000000)),
                      "[1969/12/31 18:00:00, 1969/12/31 18:00:27]");

    BOOST_CHECK(!std::less<TimeInterval>()(
                    TimeInterval(0, 13), TimeInterval(7, 27)
                    ));
    BOOST_CHECK(!std::less<TimeInterval>()(
                    TimeInterval(7, 27), TimeInterval(0, 13)
                    ));
    BOOST_CHECK(!std::less<TimeInterval>()(
                    TimeInterval(7, 13), TimeInterval(7, 27)
                    ));
    BOOST_CHECK(!std::less<TimeInterval>()(
                    TimeInterval(7, 27), TimeInterval(7, 13)
                    ));
    BOOST_CHECK(!std::less<TimeInterval>()(
                    TimeInterval(0, 7), TimeInterval(7, 27)
                    ));
    BOOST_CHECK(!std::less<TimeInterval>()(
                    TimeInterval(7, 27), TimeInterval(0, 7)
                    ));
    BOOST_CHECK(std::less<TimeInterval>()(
                    TimeInterval(0, 7), TimeInterval(13, 27)
                    ));
    BOOST_CHECK(!std::less<TimeInterval>()(
                    TimeInterval(13, 27), TimeInterval(0, 7)
                    ));
}
