/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2016 Mattia Basaglia
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#define BOOST_TEST_MODULE HttPony_TestIpAddress
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

#include "httpony/ip_address.hpp"

using namespace httpony;

BOOST_AUTO_TEST_CASE( test_output )
{
    boost::test_tools::output_test_stream test;

    test << IPAddress();
    BOOST_CHECK( test.is_equal( "(invalid)" ) );

    test << IPAddress(IPAddress::Type::IPv4, "127.0.0.1", 0);
    BOOST_CHECK( test.is_equal( "127.0.0.1:0" ) );

    test << IPAddress(IPAddress::Type::IPv4, "127.0.0.1", 80);
    BOOST_CHECK( test.is_equal( "127.0.0.1:80" ) );

    test << IPAddress(IPAddress::Type::IPv6, "::127.0.0.1", 80);
    BOOST_CHECK( test.is_equal( "[::127.0.0.1]:80" ) );

    test << IPAddress(IPAddress::Type::IPv6, "127.0.0.1", 80);
    BOOST_CHECK( test.is_equal( "127.0.0.1:80" ) );
}

BOOST_AUTO_TEST_CASE( test_from_string )
{
    IPAddress address("");
    BOOST_CHECK( address.type == IPAddress::Type::IPv6 );
    BOOST_CHECK( address.string == "" );
    BOOST_CHECK( address.port == 0 );

    address = IPAddress(":80");
    BOOST_CHECK( address.type == IPAddress::Type::IPv6 );
    BOOST_CHECK( address.string == "" );
    BOOST_CHECK( address.port == 80 );

    address = IPAddress("127.0.0.1:80");
    BOOST_CHECK( address.type == IPAddress::Type::IPv4 );
    BOOST_CHECK( address.string == "127.0.0.1" );
    BOOST_CHECK( address.port == 80 );

    address = IPAddress("127.0.0.1:80");
    BOOST_CHECK( address.type == IPAddress::Type::IPv4 );
    BOOST_CHECK( address.string == "127.0.0.1" );
    BOOST_CHECK( address.port == 80 );

    address = IPAddress("127.0.0.1");
    BOOST_CHECK( address.type == IPAddress::Type::IPv4 );
    BOOST_CHECK( address.string == "127.0.0.1" );
    BOOST_CHECK( address.port == 0 );

    address = IPAddress("[::]:80");
    BOOST_CHECK( address.type == IPAddress::Type::IPv6 );
    BOOST_CHECK( address.string == "::" );
    BOOST_CHECK( address.port == 80 );

    address = IPAddress("[::]");
    BOOST_CHECK( address.type == IPAddress::Type::IPv6 );
    BOOST_CHECK( address.string == "::" );
    BOOST_CHECK( address.port == 0 );

    address = IPAddress("::1");
    BOOST_CHECK( address.type == IPAddress::Type::IPv6 );
    BOOST_CHECK( address.string == "::1" );
    BOOST_CHECK( address.port == 0 );
}
