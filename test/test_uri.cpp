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


#define BOOST_TEST_MODULE HttPony_Uri
#include <boost/test/unit_test.hpp>

#include "httpony/uri.hpp"

using namespace httpony;

BOOST_AUTO_TEST_CASE( test_authority_empty )
{
    Authority auth;
    BOOST_CHECK( !auth.user );
    BOOST_CHECK( !auth.password );
    BOOST_CHECK( auth.host.empty() );
    BOOST_CHECK( !auth.port );
    BOOST_CHECK( auth.empty() );
    BOOST_CHECK( auth.full() == "" );
}

BOOST_AUTO_TEST_CASE( test_authority_empty_string )
{
    Authority auth("");
    BOOST_CHECK( !auth.user );
    BOOST_CHECK( !auth.password );
    BOOST_CHECK( auth.host.empty() );
    BOOST_CHECK( !auth.port );
    BOOST_CHECK( auth.empty() );
    BOOST_CHECK( auth.full() == "" );
}

BOOST_AUTO_TEST_CASE( test_authority_host_only )
{
    Authority auth("hello");
    BOOST_CHECK( !auth.user );
    BOOST_CHECK( !auth.password );
    BOOST_CHECK( auth.host == "hello" );
    BOOST_CHECK( !auth.port );
    BOOST_CHECK( !auth.empty() );
    BOOST_CHECK( auth.full() == "hello" );
}

BOOST_AUTO_TEST_CASE( test_authority_host_port )
{
    Authority auth("hello:123");
    BOOST_CHECK( !auth.user );
    BOOST_CHECK( !auth.password );
    BOOST_CHECK( auth.host == "hello" );
    BOOST_CHECK( *auth.port == 123 );
    BOOST_CHECK( !auth.empty() );
    BOOST_CHECK( auth.full() == "hello:123" );
}

BOOST_AUTO_TEST_CASE( test_authority_host_colon_noport )
{
    Authority auth("[::123]");
    BOOST_CHECK( !auth.user );
    BOOST_CHECK( !auth.password );
    BOOST_CHECK( auth.host == "[::123]" );
    BOOST_CHECK( !auth.port );
    BOOST_CHECK( !auth.empty() );
    BOOST_CHECK( auth.full() == "[::123]" );
}

BOOST_AUTO_TEST_CASE( test_authority_user_host )
{
    Authority auth("hello@world");
    BOOST_CHECK( *auth.user == "hello" );
    BOOST_CHECK( !auth.password );
    BOOST_CHECK( auth.host == "world" );
    BOOST_CHECK( !auth.port );
    BOOST_CHECK( !auth.empty() );
    BOOST_CHECK( auth.full() == "hello@world" );
}

BOOST_AUTO_TEST_CASE( test_authority_user_empty_password )
{
    Authority auth("hello:@world");
    BOOST_CHECK( *auth.user == "hello" );
    BOOST_CHECK( *auth.password == "" );
    BOOST_CHECK( auth.host == "world" );
    BOOST_CHECK( !auth.port );
    BOOST_CHECK( !auth.empty() );
    BOOST_CHECK( auth.full() == "hello:@world" );
}

BOOST_AUTO_TEST_CASE( test_authority_user_password )
{
    Authority auth("hello:there@world");
    BOOST_CHECK( *auth.user == "hello" );
    BOOST_CHECK( *auth.password == "there" );
    BOOST_CHECK( auth.host == "world" );
    BOOST_CHECK( !auth.port );
    BOOST_CHECK( !auth.empty() );
    BOOST_CHECK( auth.full() == "hello:there@world" );
}

BOOST_AUTO_TEST_CASE( test_authority_user_password_port )
{
    Authority auth("hello:there@world:123");
    BOOST_CHECK( *auth.user == "hello" );
    BOOST_CHECK( *auth.password == "there" );
    BOOST_CHECK( auth.host == "world" );
    BOOST_CHECK( *auth.port == 123 );
    BOOST_CHECK( !auth.empty() );
    BOOST_CHECK( auth.full() == "hello:there@world:123" );
}

BOOST_AUTO_TEST_CASE( test_authority_cmp )
{
    BOOST_CHECK( Authority("hello:there@world:123") == Authority("hello:there@world:123") );
    BOOST_CHECK( Authority("hello:there@world:123") != Authority("hello:there@world") );
    BOOST_CHECK( Authority("hello:there@world:123") != Authority("hello:there@world:1234") );
    BOOST_CHECK( Authority("hello:there@world:123") != Authority("hello:there@worl:123") );
    BOOST_CHECK( Authority("hello:there@world:123") != Authority("hello:@world:123") );
    BOOST_CHECK( Authority("hello@world:123") != Authority("hello:@world:123") );
    BOOST_CHECK( Authority("hello:there@world:123") != Authority("world:123") );
}

BOOST_AUTO_TEST_CASE( test_urlencode )
{
    BOOST_CHECK( urlencode("fo0.-_~ ?&/#:+%") == "fo0.-_~%20%3F%26%2F%23%3A%2B%25" );
    BOOST_CHECK( urlencode("fo0.-_~ ?&/#:+%", true) == "fo0.-_~+%3F%26%2F%23%3A%2B%25" );
}

BOOST_AUTO_TEST_CASE( test_urldecode )
{
    BOOST_CHECK( urldecode("fo0.-_~ ?&/#:+") == "fo0.-_~ ?&/#:+" );
    BOOST_CHECK( urldecode("fo0.-_~%20%3f%26%2F%23%3A%2B%25") == "fo0.-_~ ?&/#:+%" );
    BOOST_CHECK( urldecode("fo0.-_~+%3F%26%2F%23%3A%2B%25", true) == "fo0.-_~ ?&/#:+%" );
}

BOOST_AUTO_TEST_CASE( test_parse_query_string )
{
    BOOST_CHECK( parse_query_string("foo=bar")          == DataMap({{"foo", "bar"}})                );
    BOOST_CHECK( parse_query_string("?foo=bar")         == DataMap({{"foo", "bar"}})                );
    BOOST_CHECK( parse_query_string("foo=foo&bar=bar")  == DataMap({{"foo", "foo"}, {"bar", "bar"}}));
    BOOST_CHECK( parse_query_string("hello")            == DataMap({{"hello", ""}})                 );
    BOOST_CHECK( parse_query_string("test=1%2b1=2")     == DataMap({{"test", "1+1=2"}})             );
    BOOST_CHECK( parse_query_string("2%2b2=4")          == DataMap({{"2+2", "4"}})                  );
    BOOST_CHECK( parse_query_string("q=hello+world")    == DataMap({{"q", "hello world"}})          );
}

BOOST_AUTO_TEST_CASE( test_build_query_string )
{
    BOOST_CHECK( build_query_string(DataMap({{"foo", "bar"}}))                  == "foo=bar"        );
    BOOST_CHECK( build_query_string(DataMap({{"foo", "bar"}}), true)            == "?foo=bar"       );
    BOOST_CHECK( build_query_string(DataMap({{"foo", "foo"}, {"bar", "bar"}}))  == "foo=foo&bar=bar");
    BOOST_CHECK( build_query_string(DataMap({{"hello", ""}}))                   == "hello"          );
    BOOST_CHECK( build_query_string(DataMap({{"test", "1+1=2"}}))               == "test=1%2B1%3D2" );
    BOOST_CHECK( build_query_string(DataMap({{"2+2", "4"}}))                    == "2%2B2=4"        );
    BOOST_CHECK( build_query_string(DataMap({{"q", "hello world"}}))            == "q=hello+world"  );
}

BOOST_AUTO_TEST_CASE( test_uri_cmp )
{
    BOOST_CHECK( Uri({"", "", {}, {}, ""}) == Uri() );
    BOOST_CHECK( Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) == Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) );
    BOOST_CHECK( Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) != Uri({"a", "b", {"c"}, {{"d", "e"}}, "X"}) );
    BOOST_CHECK( Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) != Uri({"a", "b", {"c"}, {{"d", "X"}}, "f"}) );
    BOOST_CHECK( Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) != Uri({"a", "b", {"X"}, {{"d", "e"}}, "f"}) );
    BOOST_CHECK( Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) != Uri({"a", "X", {"c"}, {{"d", "e"}}, "f"}) );
    BOOST_CHECK( Uri({"a", "b", {"c"}, {{"d", "e"}}, "f"}) != Uri({"X", "b", {"c"}, {{"d", "e"}}, "f"}) );
}

BOOST_AUTO_TEST_CASE( test_uri_ctor_scheme )
{
    BOOST_CHECK( Uri("foo:") == Uri({"foo", "", {}, {}, ""}) );

    BOOST_CHECK( Uri("foo://bar") == Uri({"foo", "bar", {}, {}, ""}) );
    BOOST_CHECK( Uri("foo:/bar") == Uri({"foo", "", {"bar"}, {}, ""}) );
    BOOST_CHECK( Uri("foo:?bar") == Uri({"foo", "", {}, {{"bar", ""}}, ""}) );
    BOOST_CHECK( Uri("foo:#bar") == Uri({"foo", "", {}, {}, "bar"}) );

    BOOST_CHECK( Uri("foo://a/b?c=d#e") == Uri({"foo", "a", {"b"}, {{"c", "d"}}, "e"}) );

    BOOST_CHECK( Uri("//a/b?c=d#e") == Uri({"", "a", {"b"}, {{"c", "d"}}, "e"}) );
}

BOOST_AUTO_TEST_CASE( test_uri_ctor_authority )
{
    BOOST_CHECK( Uri("//foo") == Uri({"", "foo", {}, {}, ""}) );

    BOOST_CHECK( Uri("//foo/bar") == Uri({"", "foo", {"bar"}, {}, ""}) );
    BOOST_CHECK( Uri("//foo?bar") == Uri({"", "foo", {}, {{"bar", ""}}, ""}) );
    BOOST_CHECK( Uri("//foo#bar") == Uri({"", "foo", {}, {}, "bar"}) );

    BOOST_CHECK( Uri("a:/b?c=d#e") == Uri({"a", "", {"b"}, {{"c", "d"}}, "e"}) );
}

BOOST_AUTO_TEST_CASE( test_uri_ctor_path )
{
    BOOST_CHECK( Uri("/foo") == Uri({"", "", {"foo"}, {}, ""}) );
    BOOST_CHECK( Uri("/foo/bar") == Uri({"", "", {"foo", "bar"}, {}, ""}) );
    BOOST_CHECK( Uri("/foo/bar/") == Uri({"", "", {"foo", "bar"}, {}, ""}) );
    BOOST_CHECK( Uri("/foo//bar") == Uri({"", "", {"foo", "bar"}, {}, ""}) );
    BOOST_CHECK( Uri("/foo/./bar") == Uri({"", "", {"foo", "bar"}, {}, ""}) );
    BOOST_CHECK( Uri("/foo/../bar") == Uri({"", "", {"bar"}, {}, ""}) );
    BOOST_CHECK( Uri("/foo/../../../bar") == Uri({"", "", {"bar"}, {}, ""}) );
    BOOST_CHECK( Uri("foo") == Uri({"", "", {"foo"}, {}, ""}) );

    BOOST_CHECK( Uri("/foo?bar") == Uri({"", "", {"foo"}, {{"bar", ""}}, ""}) );
    BOOST_CHECK( Uri("/foo#bar") == Uri({"", "", {"foo"}, {}, "bar"}) );

    BOOST_CHECK( Uri("a://b?c=d#e") == Uri({"a", "b", {}, {{"c", "d"}}, "e"}) );
}

BOOST_AUTO_TEST_CASE( test_uri_ctor_query )
{
    BOOST_CHECK( Uri("?foo") == Uri({"", "", {}, {{"foo", ""}}, ""}) );
    BOOST_CHECK( Uri("?foo=bar") == Uri({"", "", {}, {{"foo", "bar"}}, ""}) );
    BOOST_CHECK( Uri("?foo&bar") == Uri({"", "", {}, {{"foo", ""}, {"bar", ""}}, ""}) );

    BOOST_CHECK( Uri("?foo#bar") == Uri({"", "", {}, {{"foo", ""}}, "bar"}) );

    BOOST_CHECK( Uri("a://b/c/d#e") == Uri({"a", "b", {"c", "d"}, {}, "e"}) );
}

BOOST_AUTO_TEST_CASE( test_uri_ctor_fragment )
{
    BOOST_CHECK( Uri("#foo") == Uri({"", "", {}, {}, "foo"}) );

    BOOST_CHECK( Uri("a://b/c?d=e") == Uri({"a", "b", {"c"}, {{"d", "e"}}, ""}) );
}

BOOST_AUTO_TEST_CASE( test_uri_path_string )
{
    BOOST_CHECK( Uri({"", "", {}, {}, ""}).path.url_encoded() == "" );
    BOOST_CHECK( Uri({"", "", {"foo"}, {}, ""}).path.url_encoded() == "/foo" );
    BOOST_CHECK( Uri({"", "", {"foo", "bar"}, {}, ""}).path.url_encoded() == "/foo/bar" );
    BOOST_CHECK( Uri({"", "", {}, {}, ""}).path.url_encoded() == "" );
    BOOST_CHECK( Uri({"", "", {"f o"}, {}, ""}).path.url_encoded() == "/f%20o" );
}

BOOST_AUTO_TEST_CASE( test_uri_query_string )
{
    BOOST_CHECK( Uri({"", "", {}, {}, ""}).query_string() == "" );
    BOOST_CHECK( Uri({"", "", {}, {{"foo", "bar"}}, ""}).query_string() == "foo=bar" );
    BOOST_CHECK( Uri({"", "", {}, {{"foo", "bar"}}, ""}).query_string(true) == "?foo=bar" );
    BOOST_CHECK( Uri({"", "", {}, {{"foo", "b r"}}, ""}).query_string(true) == "?foo=b+r" );
}

BOOST_AUTO_TEST_CASE( test_uri_full )
{
    BOOST_CHECK( Uri({"", "", {}, {}, ""}).full() == "" );

    BOOST_CHECK( Uri({"foo", "", {}, {}, ""}).full() == "foo:" );
    BOOST_CHECK( Uri({"", "foo", {}, {}, ""}).full() == "//foo" );
    BOOST_CHECK( Uri({"", "", {"foo"}, {}, ""}).full() == "/foo" );
    BOOST_CHECK( Uri({"", "", {}, {{"foo", ""}}, ""}).full() == "?foo" );
    BOOST_CHECK( Uri({"", "", {}, {}, "foo"}).full() == "#foo" );

    BOOST_CHECK( Uri({"scheme", "authority", {"path"}, {{"query", ""}}, "fragment"}).full() == "scheme://authority/path?query#fragment" );

    BOOST_CHECK( Uri({"", "authority", {"path"}, {{"query", ""}}, "fragment"}).full() == "//authority/path?query#fragment" );
    BOOST_CHECK( Uri({"scheme", "", {"path"}, {{"query", ""}}, "fragment"}).full() == "scheme:/path?query#fragment" );
    BOOST_CHECK( Uri({"scheme", "authority", {}, {{"query", ""}}, "fragment"}).full() == "scheme://authority?query#fragment" );
    BOOST_CHECK( Uri({"scheme", "authority", {"path"}, {}, "fragment"}).full() == "scheme://authority/path#fragment" );
    BOOST_CHECK( Uri({"scheme", "authority", {"path"}, {{"query", ""}}, ""}).full() == "scheme://authority/path?query" );
}

BOOST_AUTO_TEST_CASE( test_uri_ctor_simple )
{
    BOOST_CHECK( Uri("http://example.com", {{"foo", "bar"}}).full() == "http://example.com?foo=bar" );
    BOOST_CHECK( Uri("http://example.com", {{"foo", "bar"}}, "frag").full() == "http://example.com?foo=bar#frag" );
}
