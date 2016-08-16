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


#define BOOST_TEST_MODULE HttPony_QuickXml
#include <boost/test/unit_test.hpp>
#include <boost/test/output_test_stream.hpp>

#include "httpony/quick_xml.hpp"

using namespace httpony::quick_xml;
using namespace httpony::quick_xml::html;

BOOST_AUTO_TEST_CASE( test_text )
{
    Text text("hello");
    BOOST_CHECK( !text.is_attribute() );
    BOOST_CHECK( !text.is_element() );
    BOOST_CHECK( text.contents() == "hello" );
    boost::test_tools::output_test_stream output;
    output << text;
    BOOST_CHECK( output.is_equal( "hello" ) );
}

BOOST_AUTO_TEST_CASE( test_attribute )
{
    Attribute attribute("hello", "world");
    BOOST_CHECK( attribute.is_attribute() );
    BOOST_CHECK( !attribute.is_element() );
    BOOST_CHECK( attribute.name() == "hello" );
    BOOST_CHECK( attribute.value() == "world" );
    boost::test_tools::output_test_stream output;
    output << attribute;
    BOOST_CHECK( output.is_equal( " hello='world'" ) );
}

BOOST_AUTO_TEST_CASE( test_attributes )
{
    Attributes attributes({{"hello", "world"}, {"foo", "bar"}});
    BOOST_CHECK( attributes.is_attribute() );
    BOOST_CHECK( !attributes.is_element() );
    boost::test_tools::output_test_stream output;
    output << attributes;
    BOOST_CHECK( output.is_equal( " hello='world' foo='bar'" ) );
}

BOOST_AUTO_TEST_CASE( test_attributes_empty )
{
    Attributes attributes{};
    boost::test_tools::output_test_stream output;
    output << attributes;
    BOOST_CHECK( output.is_equal( "" ) );
}

BOOST_AUTO_TEST_CASE( test_doctype )
{
    DocType doctype("html");
    BOOST_CHECK( !doctype.is_attribute() );
    BOOST_CHECK( !doctype.is_element() );
    BOOST_CHECK( doctype.string() == "html" );
    boost::test_tools::output_test_stream output;
    output << doctype;
    BOOST_CHECK( output.is_equal( "<!DOCTYPE html>" ) );
}

BOOST_AUTO_TEST_CASE( test_xml_declaration )
{
    XmlDeclaration xml_decl;
    BOOST_CHECK( !xml_decl.is_attribute() );
    BOOST_CHECK( !xml_decl.is_element() );
    BOOST_CHECK( xml_decl.version() == "1.0" );
    BOOST_CHECK( xml_decl.encoding() == "utf-8" );
    boost::test_tools::output_test_stream output;
    output << xml_decl;
    BOOST_CHECK( output.is_equal( "<?xml version='1.0' encoding='utf-8'?>" ) );
}

BOOST_AUTO_TEST_CASE( test_block_element_empty )
{
    BlockElement elem("foo");
    BOOST_CHECK( !elem.is_attribute() );
    BOOST_CHECK( elem.is_element() );
    boost::test_tools::output_test_stream output;
    output << elem;
    BOOST_CHECK( output.is_equal( "<foo></foo>" ) );
}

BOOST_AUTO_TEST_CASE( test_element_empty )
{
    Element elem("foo");
    BOOST_CHECK( !elem.is_attribute() );
    BOOST_CHECK( elem.is_element() );
    boost::test_tools::output_test_stream output;
    output << elem;
    BOOST_CHECK( output.is_equal( "<foo/>" ) );
}

BOOST_AUTO_TEST_CASE( test_element_attronly )
{
    Element elem("foo", Attribute{"hello", "world"});
    boost::test_tools::output_test_stream output;
    output << elem;
    BOOST_CHECK( output.is_equal( "<foo hello='world'/>" ) );
}

BOOST_AUTO_TEST_CASE( test_element_full )
{
    Element elem("foo", Attribute{"hello", "world"}, Text{"foo"}, Element{"bar"});
    boost::test_tools::output_test_stream output;
    output << elem;
    BOOST_CHECK( output.is_equal( "<foo hello='world'>foo<bar/></foo>" ) );
}

BOOST_AUTO_TEST_CASE( test_block_element_full )
{
    BlockElement elem("foo",  Text{"foo"}, Attribute{"hello", "world"}, Element{"bar"});
    boost::test_tools::output_test_stream output;
    output << elem;
    BOOST_CHECK( output.is_equal( "<foo hello='world'>foo<bar/></foo>" ) );
}

HtmlDocument html_document()
{
    return HtmlDocument{
        "Hello",
        Element{"body",
            Comment{"This is an example"},
            Element{"p",
                Attributes{
                    {"id", "content"},
                    {"class", "main"}
                },
                Text{"hello world"}
            },
        },
    };
}

BOOST_AUTO_TEST_CASE( test_html_document )
{
    boost::test_tools::output_test_stream output;
    output << html_document();
    BOOST_CHECK( output.is_equal(
        "<!DOCTYPE html><html>"
        "<head><title>Hello</title></head>"
        "<body>"
        "<!--This is an example-->"
        "<p id='content' class='main'>hello world</p>"
        "</body>"
        "</html>"
    ) );
}

BOOST_AUTO_TEST_CASE( test_indent_elements )
{
    boost::test_tools::output_test_stream output;
    html_document().print(output, Indentation{Indentation::Element|Indentation::Comment});
    BOOST_CHECK( output.is_equal(
R"(<!DOCTYPE html>
<html>
    <head>
        <title>Hello</title>
    </head>
    <body>
        <!--This is an example-->
        <p id='content' class='main'>hello world</p>
    </body>
</html>)"
    ) );
}

BOOST_AUTO_TEST_CASE( test_indent_attributes )
{
    boost::test_tools::output_test_stream output;
    html_document().print(output, Indentation{Indentation::Element|Indentation::Attribute|Indentation::Comment});
    BOOST_CHECK( output.is_equal(
R"(<!DOCTYPE html>
<html>
    <head>
        <title>Hello</title>
    </head>
    <body>
        <!--This is an example-->
        <p
            id='content'
            class='main'
        >hello world</p>
    </body>
</html>)"
    ) );
}


BOOST_AUTO_TEST_CASE( test_indent_comments )
{
    boost::test_tools::output_test_stream output;
    html_document().print(output, Indentation{Indentation::Element|Indentation::Comment|Indentation::CommentText});
    BOOST_CHECK( output.is_equal(
R"(<!DOCTYPE html>
<html>
    <head>
        <title>Hello</title>
    </head>
    <body>
        <!--
            This is an example
        -->
        <p id='content' class='main'>hello world</p>
    </body>
</html>)"
    ) );
}

BOOST_AUTO_TEST_CASE( test_comment )
{
    Comment text("hello");
    BOOST_CHECK( !text.is_attribute() );
    BOOST_CHECK( !text.is_element() );
    BOOST_CHECK( text.contents() == "hello" );
    boost::test_tools::output_test_stream output;
    output << text;
    BOOST_CHECK( output.is_equal( "<!--hello-->" ) );
}

BOOST_AUTO_TEST_CASE( test_ul )
{
    List list;
    BOOST_CHECK( !list.is_attribute() );
    BOOST_CHECK( list.is_element() );
    list.add_item(Text{"hello"});
    list.add_item(Text{"world"});
    boost::test_tools::output_test_stream output;
    output << list;
    BOOST_CHECK( output.is_equal( "<ul><li>hello</li><li>world</li></ul>" ) );
}

BOOST_AUTO_TEST_CASE( test_ol )
{
    List list(true);
    BOOST_CHECK( !list.is_attribute() );
    BOOST_CHECK( list.is_element() );
    list.add_item(Text{"hello"});
    list.add_item(Text{"world"});
    boost::test_tools::output_test_stream output;
    output << list;
    BOOST_CHECK( output.is_equal( "<ol><li>hello</li><li>world</li></ol>" ) );
}

BOOST_AUTO_TEST_CASE( test_link )
{
    Link link("/foo", "bar");
    BOOST_CHECK( !link.is_attribute() );
    BOOST_CHECK( link.is_element() );
    BOOST_CHECK( link.target() == "/foo" );
    link.set_target("/bar");
    BOOST_CHECK( link.target() == "/bar" );
    boost::test_tools::output_test_stream output;
    output << link;
    BOOST_CHECK( output.is_equal( "<a href='/bar'>bar</a>" ) );
}

BOOST_AUTO_TEST_CASE( test_input )
{
    Input input("name", "text", "hello");
    BOOST_CHECK( !input.is_attribute() );
    BOOST_CHECK( input.is_element() );
    BOOST_CHECK( input.value() == "hello" );
    input.set_value("world");
    BOOST_CHECK( input.value() == "world" );
    boost::test_tools::output_test_stream output;
    output << input;
    BOOST_CHECK( output.is_equal( "<input type='text' name='name' id='name' value='world'/>" ) );
}

BOOST_AUTO_TEST_CASE( test_label )
{
    Label label("name", "Name");
    BOOST_CHECK( !label.is_attribute() );
    BOOST_CHECK( label.is_element() );
    BOOST_CHECK( label.target() == "name" );
    label.set_target("username");
    BOOST_CHECK( label.target() == "username" );
    boost::test_tools::output_test_stream output;
    output << label;
    BOOST_CHECK( output.is_equal( "<label for='username'>Name</label>" ) );
}


