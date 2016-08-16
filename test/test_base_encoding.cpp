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


#define BOOST_TEST_MODULE HttPony_BaseEncoding
#include <boost/test/unit_test.hpp>

#include "httpony/base_encoding.hpp"

using namespace httpony;

BOOST_AUTO_TEST_CASE( test_base64_encode )
{
    std::string output;
    Base64().encode("EUP", output);
    BOOST_CHECK_EQUAL( output, "RVVQ" );

    Base64().encode("Hello!", output);
    BOOST_CHECK_EQUAL( output, "SGVsbG8h" );

    Base64().encode("1", output);
    BOOST_CHECK_EQUAL( output, "MQ==" );

    Base64().encode("x", output);
    BOOST_CHECK_EQUAL( output, "eA==" );

    Base64().encode("Hello world", output);
    BOOST_CHECK_EQUAL( output, "SGVsbG8gd29ybGQ=" );

    Base64().encode("HttPony", output);
    BOOST_CHECK_EQUAL( output, "SHR0UG9ueQ==" );

    BOOST_CHECK_EQUAL( Base64().encode("~~>~~?"), "fn4+fn4/" );
    BOOST_CHECK_EQUAL( Base64('-', '_').encode("~~>~~?"), "fn4-fn4_" );
    BOOST_CHECK_EQUAL( Base64(false).encode("x"), "eA" );
}


BOOST_AUTO_TEST_CASE( test_base64_decode )
{
    BOOST_CHECK_EQUAL( Base64().decode("RVVQ"), "EUP" );

    std::string output;
    BOOST_CHECK( Base64().decode("SGVsbG8h", output) );
    BOOST_CHECK_EQUAL( output, "Hello!" );

    BOOST_CHECK_EQUAL( Base64().decode("MQ=="), "1" );

    BOOST_CHECK_EQUAL( Base64().decode("eA=="), "x");

    BOOST_CHECK_EQUAL( Base64().decode("SGVsbG8gd29ybGQ="), "Hello world" );

    BOOST_CHECK_EQUAL( Base64().decode("SHR0UG9ueQ=="), "HttPony" );

    BOOST_CHECK_EQUAL( Base64().decode("fn4+fn4/"), "~~>~~?" );
    BOOST_CHECK_EQUAL( Base64('-', '_').decode("fn4-fn4_"), "~~>~~?" );

    BOOST_CHECK_EQUAL( Base64(false).decode("eA"), "x" );

}

BOOST_AUTO_TEST_CASE( test_base64_decode_error )
{
    std::string output = "Hello";

    BOOST_CHECK( !Base64().decode("....", output) );
    BOOST_CHECK_EQUAL( output, "" );

    BOOST_CHECK_THROW( Base64().decode("...."), EncodingError );

    BOOST_CHECK_THROW( Base64().decode("eA"), EncodingError );
    BOOST_CHECK_THROW( Base64().decode("eA======"), EncodingError );
}

BOOST_AUTO_TEST_CASE( test_base32_encode )
{
    BOOST_CHECK_EQUAL( Base32().encode("Pony!"), "KBXW46JB" );
    BOOST_CHECK_EQUAL( Base32().encode("Pony"), "KBXW46I=" );
    BOOST_CHECK_EQUAL( Base32().encode("Pon"), "KBXW4===" );
    BOOST_CHECK_EQUAL( Base32().encode("Po"), "KBXQ====" );
    BOOST_CHECK_EQUAL( Base32().encode("P"), "KA======" );
    BOOST_CHECK_EQUAL( Base32().encode("HttPony"), "JB2HIUDPNZ4Q====" );
    BOOST_CHECK_EQUAL( Base32(false).encode("HttPony"), "JB2HIUDPNZ4Q" );
}

BOOST_AUTO_TEST_CASE( test_base32_decode )
{
    BOOST_CHECK_EQUAL( Base32().decode("kbxw46jb"), "Pony!" );
    BOOST_CHECK_EQUAL( Base32().decode("KBXW46JB"), "Pony!" );
    BOOST_CHECK_EQUAL( Base32().decode("KBXW46I="), "Pony" );
    BOOST_CHECK_EQUAL( Base32().decode("KBXW4==="), "Pon" );
    BOOST_CHECK_EQUAL( Base32().decode("KBXQ===="), "Po" );
    BOOST_CHECK_EQUAL( Base32().decode("KA======"), "P" );
    BOOST_CHECK_EQUAL( Base32().decode("JB2HIUDPNZ4Q===="), "HttPony" );
    BOOST_CHECK_EQUAL( Base32(false).decode("JB2HIUDPNZ4Q"), "HttPony" );
}

BOOST_AUTO_TEST_CASE( test_base32_decode_error )
{
    std::string output = "Hello";

    BOOST_CHECK( !Base32().decode("..======", output) );
    BOOST_CHECK_EQUAL( output, "" );

    BOOST_CHECK_THROW( Base32().decode("..======"), EncodingError );

    BOOST_CHECK_THROW( Base32().decode("KA"), EncodingError );
    BOOST_CHECK_THROW( Base32().decode("KA=========="), EncodingError );

    BOOST_CHECK_THROW( Base32().decode("99======"), EncodingError );
    BOOST_CHECK_THROW( Base32().decode("00======"), EncodingError );
}

BOOST_AUTO_TEST_CASE( test_base32hex_encode )
{
    BOOST_CHECK_EQUAL( Base32Hex().encode("Pony!"), "A1NMSU91" );
    BOOST_CHECK_EQUAL( Base32Hex().encode("Pony"), "A1NMSU8=" );
    BOOST_CHECK_EQUAL( Base32Hex().encode("Pon"), "A1NMS===" );
    BOOST_CHECK_EQUAL( Base32Hex().encode("Po"), "A1NG====" );
    BOOST_CHECK_EQUAL( Base32Hex().encode("P"), "A0======" );
    BOOST_CHECK_EQUAL( Base32Hex().encode("HttPony"), "91Q78K3FDPSG====" );
    BOOST_CHECK_EQUAL( Base32Hex(false).encode("HttPony"), "91Q78K3FDPSG" );
}

BOOST_AUTO_TEST_CASE( test_base32hex_decode )
{
    BOOST_CHECK_EQUAL( Base32Hex().decode("a1nmsu91"), "Pony!" );
    BOOST_CHECK_EQUAL( Base32Hex().decode("A1NMSU91"), "Pony!" );
    BOOST_CHECK_EQUAL( Base32Hex().decode("A1NMSU8="), "Pony" );
    BOOST_CHECK_EQUAL( Base32Hex().decode("A1NMS==="), "Pon" );
    BOOST_CHECK_EQUAL( Base32Hex().decode("A1NG===="), "Po" );
    BOOST_CHECK_EQUAL( Base32Hex().decode("A0======"), "P" );
    BOOST_CHECK_EQUAL( Base32Hex().decode("91Q78K3FDPSG===="), "HttPony" );
    BOOST_CHECK_EQUAL( Base32Hex(false).decode("91Q78K3FDPSG"), "HttPony" );
}

BOOST_AUTO_TEST_CASE( test_base32hex_decode_error )
{
    std::string output = "Hello";

    BOOST_CHECK( !Base32Hex().decode("..======", output) );
    BOOST_CHECK_EQUAL( output, "" );

    BOOST_CHECK_THROW( Base32Hex().decode("..======"), EncodingError );

    BOOST_CHECK_THROW( Base32Hex().decode("A0"), EncodingError );
    BOOST_CHECK_THROW( Base32Hex().decode("A0=========="), EncodingError );

    BOOST_CHECK_THROW( Base32Hex().decode("ZZ======"), EncodingError );
    BOOST_CHECK_THROW( Base32Hex().decode("zz======"), EncodingError );
}

BOOST_AUTO_TEST_CASE( test_base16_encode )
{
    BOOST_CHECK_EQUAL( Base16().encode(""), "" );
    BOOST_CHECK_EQUAL( Base16().encode("f"), "66" );
    BOOST_CHECK_EQUAL( Base16().encode("fo"), "666F" );
    BOOST_CHECK_EQUAL( Base16().encode("foo"), "666F6F" );
    BOOST_CHECK_EQUAL( Base16().encode("foob"), "666F6F62" );
    BOOST_CHECK_EQUAL( Base16().encode("fooba"), "666F6F6261" );
    BOOST_CHECK_EQUAL( Base16().encode("foobar"), "666F6F626172" );
}

BOOST_AUTO_TEST_CASE( test_base16_decode )
{
    BOOST_CHECK_EQUAL( Base16().decode(""), "" );
    BOOST_CHECK_EQUAL( Base16().decode("66"), "f" );
    BOOST_CHECK_EQUAL( Base16().decode("666F"), "fo" );
    BOOST_CHECK_EQUAL( Base16().decode("666F6F"), "foo" );
    BOOST_CHECK_EQUAL( Base16().decode("666F6F62"), "foob" );
    BOOST_CHECK_EQUAL( Base16().decode("666F6F6261"), "fooba" );
    BOOST_CHECK_EQUAL( Base16().decode("666F6F626172"), "foobar" );
    BOOST_CHECK_EQUAL( Base16().decode("666f6f626172"), "foobar" );
}

BOOST_AUTO_TEST_CASE( test_base16_decode_error )
{
    std::string output = "Hello";

    BOOST_CHECK( !Base16().decode("....", output) );
    BOOST_CHECK_EQUAL( output, "" );

    BOOST_CHECK_THROW( Base16().decode("...."), EncodingError );

    BOOST_CHECK_THROW( Base16().decode("666"), EncodingError );
    BOOST_CHECK_THROW( Base16().decode("666="), EncodingError );
}
