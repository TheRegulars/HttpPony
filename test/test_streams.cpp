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


#define BOOST_TEST_MODULE HttPony_MimeType
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

#include "httpony/io/network_stream.hpp"
#include "httpony/io/buffer.hpp"

using namespace httpony;
using namespace httpony::io;

BOOST_AUTO_TEST_CASE( test_output_write_to )
{
    OutputContentStream stream(MimeType{"text/plain"});
    stream << "hello\n";

    boost::test_tools::output_test_stream test;
    stream.write_to(test);
    BOOST_CHECK( test.is_equal( "hello\n" ) );
}

BOOST_AUTO_TEST_CASE( test_output_multiple_write_to )
{
    OutputContentStream stream(MimeType{"text/plain"});
    stream << "hello\n";

    boost::test_tools::output_test_stream test1;
    boost::test_tools::output_test_stream test2;
    stream.write_to(test1);
    stream.write_to(test2);
    BOOST_CHECK( test1.is_equal( "hello\n" ) );
    BOOST_CHECK( test2.is_equal( "hello\n" ) );
}

BOOST_AUTO_TEST_CASE( test_input_write_to )
{
    std::stringbuf source("hello\n");
    Headers headers{
        {"Content-Type", "text/plain"},
        {"Content-Length", std::to_string(source.str().size())},
    };
    InputContentStream stream(&source, headers);

    boost::test_tools::output_test_stream test;
    stream.write_to(test);
    BOOST_CHECK( test.is_equal( "hello\n" ) );
}

BOOST_AUTO_TEST_CASE( test_input_multiple_write_to )
{
    std::stringbuf source("hello\n");
    Headers headers{
        {"Content-Type", "text/plain"},
        {"Content-Length", std::to_string(source.str().size())},
    };
    InputContentStream stream(&source, headers);

    boost::test_tools::output_test_stream test1;
    boost::test_tools::output_test_stream test2;
    stream.write_to(test1);
    stream.write_to(test2);
    BOOST_CHECK( test1.is_equal( "hello\n" ) );
    BOOST_CHECK( test2.is_equal( "hello\n" ) );
}

BOOST_AUTO_TEST_CASE( test_input_multiple_write_to_boost_streambuf )
{
    boost::asio::streambuf source;
    std::ostream(&source) << "hello\n";

    Headers headers{
        {"Content-Type", "text/plain"},
        {"Content-Length", "6"},
    };
    InputContentStream stream(&source, headers);

    boost::test_tools::output_test_stream test1;
    boost::test_tools::output_test_stream test2;
    stream.write_to(test1);
    stream.write_to(test2);
    BOOST_CHECK( test1.is_equal( "hello\n" ) );
    BOOST_CHECK( test2.is_equal( "hello\n" ) );
}

