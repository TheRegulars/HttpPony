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


#define BOOST_TEST_MODULE HttPony_Test
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

BOOST_AUTO_TEST_CASE( test_io_output_write_to )
{
    ContentStream io_stream;

    io_stream.start_output(MimeType{"text/plain"});
    io_stream << "hello\n";

    boost::test_tools::output_test_stream test;
    io_stream.write_to(test);
    BOOST_CHECK( test.is_equal( "hello\n" ) );
}

BOOST_AUTO_TEST_CASE( test_io_output_multiple_write_to )
{
    ContentStream io_stream;

    io_stream.start_output(MimeType{"text/plain"});
    io_stream << "hello\n";

    boost::test_tools::output_test_stream test1;
    boost::test_tools::output_test_stream test2;
    io_stream.write_to(test1);
    io_stream.write_to(test2);
    BOOST_CHECK( test1.is_equal( "hello\n" ) );
    BOOST_CHECK( test2.is_equal( "hello\n" ) );
}

BOOST_AUTO_TEST_CASE( test_io_input_write_to )
{
    std::stringbuf source("hello\n");
    Headers headers{
        {"Content-Type", "text/plain"},
        {"Content-Length", std::to_string(source.str().size())},
    };
    ContentStream io_stream;

    io_stream.start_input(&source, headers);

    boost::test_tools::output_test_stream test;
    io_stream.write_to(test);
    BOOST_CHECK( test.is_equal( "hello\n" ) );
}

BOOST_AUTO_TEST_CASE( test_io_input_multiple_write_to )
{
    std::stringbuf source("hello\n");
    Headers headers{
        {"Content-Type", "text/plain"},
        {"Content-Length", std::to_string(source.str().size())},
    };
    InputContentStream io_stream;

    io_stream.start_input(&source, headers);

    boost::test_tools::output_test_stream test1;
    boost::test_tools::output_test_stream test2;
    io_stream.write_to(test1);
    io_stream.write_to(test2);
    BOOST_CHECK( test1.is_equal( "hello\n" ) );
    BOOST_CHECK( test2.is_equal( "hello\n" ) );
}

BOOST_AUTO_TEST_CASE( test_io_input_multiple_write_to_boost_streambuf )
{
    boost::asio::streambuf source;
    std::ostream(&source) << "hello\n";

    Headers headers{
        {"Content-Type", "text/plain"},
        {"Content-Length", "6"},
    };
    InputContentStream io_stream;

    io_stream.start_input(&source, headers);

    boost::test_tools::output_test_stream test1;
    boost::test_tools::output_test_stream test2;
    io_stream.write_to(test1);
    io_stream.write_to(test2);
    BOOST_CHECK( test1.is_equal( "hello\n" ) );
    BOOST_CHECK( test2.is_equal( "hello\n" ) );
}

BOOST_AUTO_TEST_CASE( test_input_read_all_consume )
{
    std::stringbuf source("hello\n");
    Headers headers{
        {"Content-Type", "text/plain"},
        {"Content-Length", std::to_string(source.str().size())},
    };
    InputContentStream stream(&source, headers);

    BOOST_CHECK( stream.read_all(false) == "hello\n" );
    BOOST_CHECK( !stream.has_error() );
    BOOST_CHECK( stream.read_all() == "" );
    BOOST_CHECK( stream.has_error() );
}

BOOST_AUTO_TEST_CASE( test_input_read_all_preserve )
{
    std::stringbuf source("hello\n");
    Headers headers{
        {"Content-Type", "text/plain"},
        {"Content-Length", std::to_string(source.str().size())},
    };
    InputContentStream stream(&source, headers);

    BOOST_CHECK( stream.read_all(true) == "hello\n" );
    BOOST_CHECK( !stream.has_error() );
    BOOST_CHECK( stream.read_all() == "hello\n" );
    BOOST_CHECK( !stream.has_error() );
}

BOOST_AUTO_TEST_CASE( test_io_input_read_all_consume )
{
    std::stringbuf source("hello\n");
    Headers headers{
        {"Content-Type", "text/plain"},
        {"Content-Length", std::to_string(source.str().size())},
    };
    ContentStream io_stream;

    io_stream.start_input(&source, headers);

    BOOST_CHECK( io_stream.read_all(false) == "hello\n" );
    BOOST_CHECK( !io_stream.has_error() );
    BOOST_CHECK( io_stream.read_all() == "" );
    BOOST_CHECK( io_stream.has_error() );
}

BOOST_AUTO_TEST_CASE( test_io_input_read_all_preserve )
{
    std::stringbuf source("hello\n");
    Headers headers{
        {"Content-Type", "text/plain"},
        {"Content-Length", std::to_string(source.str().size())},
    };
    ContentStream io_stream;

    io_stream.start_input(&source, headers);

    BOOST_CHECK( io_stream.read_all(true) == "hello\n" );
    BOOST_CHECK( !io_stream.has_error() );
    BOOST_CHECK( io_stream.read_all() == "hello\n" );
    BOOST_CHECK( !io_stream.has_error() );
}

BOOST_AUTO_TEST_CASE( test_io_output_read_all )
{
    ContentStream io_stream;

    io_stream.start_output(MimeType{"text/plain"});
    io_stream << "hello\n";

    BOOST_CHECK( io_stream.read_all() == "hello\n" );
    BOOST_CHECK( !io_stream.has_error() );
}

BOOST_AUTO_TEST_CASE( test_output_move_ctor )
{
    OutputContentStream ostream("text/plain");
    ostream << "Hello";
    OutputContentStream other_stream = std::move(ostream);
    other_stream << " world!\n";
    boost::test_tools::output_test_stream test;
    other_stream.write_to(test);
    BOOST_CHECK( test.is_equal("Hello world!\n") );
}

BOOST_AUTO_TEST_CASE( test_input_move_ctor )
{
    std::stringbuf source("Hello\n");
    Headers headers{
        {"Content-Type", "text/plain"},
        {"Content-Length", std::to_string(source.str().size())},
    };
    InputContentStream istream(&source, headers);
    BOOST_CHECK( istream.read_all(true) == "Hello\n" );
    BOOST_CHECK_EQUAL( istream.get(), 'H' );
    BOOST_CHECK_EQUAL( istream.tellg(), 1 );

    InputContentStream other_stream = std::move(istream);
    BOOST_CHECK( other_stream.tellg() == 1 );
    BOOST_CHECK_EQUAL( other_stream.get(), 'e' );
    BOOST_CHECK_EQUAL( other_stream.read_all(true), "Hello\n" );
}
