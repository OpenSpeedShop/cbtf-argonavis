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

/** @file Unit tests for the ArgoNavis symbol table library. */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE ArgoNavis-SymbolTable

#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <boost/cstdint.hpp>
#include <boost/ref.hpp>
#include <boost/test/unit_test.hpp>
#include <map>
#include <set>
#include <utility>
#include <vector>

#include <ArgoNavis/SymbolTable/AddressSpaces.hpp>
#include <ArgoNavis/SymbolTable/Function.hpp>
#include <ArgoNavis/SymbolTable/LinkedObject.hpp>
#include <ArgoNavis/SymbolTable/Loop.hpp>
#include <ArgoNavis/SymbolTable/Statement.hpp>

using namespace ArgoNavis::Base;
using namespace ArgoNavis::SymbolTable;
using namespace ArgoNavis::SymbolTable::Impl;



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
    
    address_spaces.loadLinkedObject(
        thread1, linked_object1, AddressRange(0, 7)
        );
    address_spaces.loadLinkedObject(
        thread1, linked_object2, AddressRange(13, 27)
        );
    address_spaces.loadLinkedObject(
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

    address_spaces.unloadLinkedObject(thread1, linked_object1);
    address_spaces.unloadLinkedObject(thread1, linked_object2, Time(7));
    address_spaces.unloadLinkedObject(thread1, linked_object3, Time(27));
    
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
        linked_object5.getFile();
    group_message.linkedobjects.linkedobjects_val[0].range.begin = 0;
    group_message.linkedobjects.linkedobjects_val[0].range.end = 7 + 1;
    group_message.linkedobjects.linkedobjects_val[0].time_begin = 13;
    group_message.linkedobjects.linkedobjects_val[0].time_end = 27 + 1;

    address_spaces.applyMessage(group_message);

    linked_objects.clear();
    address_spaces.visitLinkedObjects(
        boost::bind(accumulate<LinkedObject>, _1, boost::ref(linked_objects))
        );
    for (std::set<LinkedObject>::const_iterator
             i = linked_objects.begin(); i != linked_objects.end(); ++i)
    {
        if (i->getFile() == linked_object5.getFile())
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
    loaded_message.linked_object = linked_object4.getFile();
    
    CBTF_Protocol_UnloadedLinkedObject unloaded_message;

    unloaded_message.threads.names.names_len = 1;
    unloaded_message.threads.names.names_val = 
        reinterpret_cast<CBTF_Protocol_ThreadName*>(
            malloc(sizeof(CBTF_Protocol_ThreadName))
            );
    unloaded_message.threads.names.names_val[0] = thread1;
    unloaded_message.time = Time::TheEnd();
    unloaded_message.linked_object = linked_object4.getFile();
    
    address_spaces.applyMessage(loaded_message);
    address_spaces.applyMessage(unloaded_message);

    linked_objects.clear();
    address_spaces.visitLinkedObjects(
        boost::bind(accumulate<LinkedObject>, _1, boost::ref(linked_objects))
        );
    for (std::set<LinkedObject>::const_iterator
             i = linked_objects.begin(); i != linked_objects.end(); ++i)
    {
        if (i->getFile() == linked_object4.getFile())
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
 * Unit test for the LinkedObject, Function, Loop, and Statement classes.
 */
BOOST_AUTO_TEST_CASE(TestSymbolTable)
{
    std::set<AddressRange> addresses;
    
    LinkedObject linked_object(FileName("/path/to/nonexistent/dso"));
    
    BOOST_CHECK_EQUAL(LinkedObject(linked_object), linked_object);
    BOOST_CHECK_EQUAL(linked_object.getFile(),
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
    BOOST_CHECK(function1.getAddressRanges().empty());

    Function function2(linked_object, "_Z2f2RKf");
    Function function3(linked_object, "_Z2f3RKf");
    Function function4(function3.clone(linked_object));

    BOOST_CHECK_NE(function1, function2);
    BOOST_CHECK_LT(function1, function2);
    BOOST_CHECK_GT(function3, function2);
    BOOST_CHECK_NE(function3, function4);

    BOOST_CHECK(!equivalent(function1, function2));
    BOOST_CHECK(equivalent(function3, function4));

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
    BOOST_CHECK(statement1.getAddressRanges().empty());

    Statement statement2(linked_object,
                         FileName("/path/to/nonexistent/source/file"), 20, 1);
    Statement statement3(linked_object,
                         FileName("/path/to/nonexistent/source/file"), 30, 1);
    Statement statement4(statement3.clone(linked_object));
    
    BOOST_CHECK_NE(statement1, statement2);
    BOOST_CHECK_LT(statement1, statement2);
    BOOST_CHECK_GT(statement3, statement2);
    BOOST_CHECK_NE(statement3, statement4);

    BOOST_CHECK(!equivalent(statement1, statement2));
    BOOST_CHECK(equivalent(statement3, statement4));

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
    function1.addAddressRanges(addresses);

    addresses = boost::assign::list_of
        (AddressRange(113, 127));
    function2.addAddressRanges(addresses);

    addresses = boost::assign::list_of
        (AddressRange(7, 13))
        (AddressRange(213, 227));
    function3.addAddressRanges(addresses);
    
    addresses = boost::assign::list_of
        (AddressRange(57, 63));
    function4.addAddressRanges(addresses);
    
    BOOST_CHECK(!function1.getAddressRanges().empty());
    BOOST_CHECK(!function2.getAddressRanges().empty());
    BOOST_CHECK(!function3.getAddressRanges().empty());
    BOOST_CHECK(!function4.getAddressRanges().empty());

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
    statement1.addAddressRanges(addresses);

    addresses = boost::assign::list_of
        (AddressRange(13, 27));
    statement2.addAddressRanges(addresses);

    addresses = boost::assign::list_of
        (AddressRange(75, 100));
    statement3.addAddressRanges(addresses);
    
    addresses = boost::assign::list_of
        (AddressRange(213, 227));
    statement4.addAddressRanges(addresses);

    BOOST_CHECK(!statement1.getAddressRanges().empty());
    BOOST_CHECK(!statement2.getAddressRanges().empty());
    BOOST_CHECK(!statement3.getAddressRanges().empty());
    BOOST_CHECK(!statement4.getAddressRanges().empty());

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
    
    BOOST_CHECK(equivalent(
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
    BOOST_CHECK(loop1.getAddressRanges().empty());

    Loop loop2(linked_object, Address(0));
    Loop loop3(loop1.clone(linked_object));

    BOOST_CHECK_NE(loop1, loop2);
    BOOST_CHECK_LT(loop1, loop2);
    BOOST_CHECK_GT(loop3, loop2);
    BOOST_CHECK_NE(loop1, loop3);

    BOOST_CHECK(!equivalent(loop1, loop2));
    BOOST_CHECK(equivalent(loop1, loop3));

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
    loop1.addAddressRanges(addresses);

    addresses = boost::assign::list_of
        (AddressRange(0, 7))
        (AddressRange(110, 130));
    loop2.addAddressRanges(addresses);

    addresses = boost::assign::list_of
        (AddressRange(13, 100));
    loop3.addAddressRanges(addresses);

    BOOST_CHECK(!loop1.getAddressRanges().empty());
    BOOST_CHECK(!loop2.getAddressRanges().empty());
    BOOST_CHECK(!loop3.getAddressRanges().empty());

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

    BOOST_CHECK(equivalent(clone, linked_object));
    Function function5(clone, "_Z2f5RKf");
    BOOST_CHECK(!equivalent(clone, linked_object));
}
