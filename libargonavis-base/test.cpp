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
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <functional>
#include <set>
#include <stdexcept>
#include <string>

#include <ArgoNavis/Base/Address.hpp>
#include <ArgoNavis/Base/AddressBitmap.hpp>
#include <ArgoNavis/Base/AddressRange.hpp>
#include <ArgoNavis/Base/FileName.hpp>
#include <ArgoNavis/Base/ThreadName.hpp>
#include <ArgoNavis/Base/Time.hpp>
#include <ArgoNavis/Base/TimeInterval.hpp>

using namespace ArgoNavis::Base;



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
 * Unit test for the ThreadName class.
 */
BOOST_AUTO_TEST_CASE(TestThreadName)
{
    ThreadName name1("first.host", 13);
    ThreadName name2("first.host", 13, 27);
    ThreadName name3("first.host", 13, 2002);
    ThreadName name4("first.host", 13, 2002, 2004);
    ThreadName name5("second.host", 13);

    BOOST_CHECK_EQUAL(name1.host(), name2.host());
    BOOST_CHECK_EQUAL(name2.host(), name3.host());
    BOOST_CHECK_EQUAL(name3.host(), name4.host());
    BOOST_CHECK_NE(name4.host(), name5.host());

    BOOST_CHECK_EQUAL(name1.pid(), name2.pid());
    BOOST_CHECK_EQUAL(name2.pid(), name3.pid());
    BOOST_CHECK_EQUAL(name3.pid(), name4.pid());
    BOOST_CHECK_EQUAL(name4.pid(), name5.pid());

    BOOST_CHECK_EQUAL(static_cast<bool>(name1.tid()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name2.tid()), true);
    BOOST_CHECK_EQUAL(static_cast<bool>(name3.tid()), true);
    BOOST_CHECK_EQUAL(static_cast<bool>(name4.tid()), true);
    BOOST_CHECK_EQUAL(static_cast<bool>(name5.tid()), false);
    BOOST_CHECK_NE(*name2.tid(), *name3.tid());
    BOOST_CHECK_EQUAL(*name3.tid(), *name4.tid());

    BOOST_CHECK_EQUAL(static_cast<bool>(name1.rank()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name2.rank()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name3.rank()), false);
    BOOST_CHECK_EQUAL(static_cast<bool>(name4.rank()), true);
    BOOST_CHECK_EQUAL(static_cast<bool>(name5.rank()), false);
    BOOST_CHECK_EQUAL(*name4.rank(), 2004);
    
    BOOST_CHECK_NE(name1, name2);
    BOOST_CHECK_NE(name2, name3);
    BOOST_CHECK_EQUAL(name3, name4);
    BOOST_CHECK_NE(name4, name5);
    BOOST_CHECK_NE(name5, name1);

    BOOST_CHECK_LT(name1, name2);
    BOOST_CHECK_LT(name2, name3);
    BOOST_CHECK_EQUAL(name3, name4);
    BOOST_CHECK_LT(name4, name5);
    BOOST_CHECK_GT(name5, name1);

    BOOST_CHECK_EQUAL(static_cast<std::string>(name1),
                      "Host \"first.host\", PID 13");
    BOOST_CHECK_EQUAL(static_cast<std::string>(name2),
                      "Host \"first.host\", PID 13, TID 0x000000000000001B");
    BOOST_CHECK_EQUAL(static_cast<std::string>(name3),
                      "Host \"first.host\", PID 13, TID 0x00000000000007D2");
    BOOST_CHECK_EQUAL(static_cast<std::string>(name4),
                      "MPI Rank 2004, TID 0x00000000000007D2");
    BOOST_CHECK_EQUAL(static_cast<std::string>(name5),
                      "Host \"second.host\", PID 13");

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
