/**
 * \file
 * \author Mattia Basaglia
 * \copyright Copyright 2015-2016 Mattia Basaglia
 * \section License
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
#define BOOST_TEST_MODULE Test_Json

#include <boost/test/unit_test.hpp>

#include "httpony/formats/json.hpp"

using namespace httpony::json;

BOOST_AUTO_TEST_CASE( test_array )
{
    JsonParser parser;
    parser.throws(false);
    JsonParser::Tree tree;

    tree = parser.parse_string("[1, 2, 3]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "1" );
    BOOST_CHECK( tree.get<std::string>("1") == "2" );
    BOOST_CHECK( tree.get<std::string>("2") == "3" );

    // not valid json, but it shouldn't have issues
    tree = parser.parse_string("[4, 5, 6,]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "4" );
    BOOST_CHECK( tree.get<std::string>("1") == "5" );
    BOOST_CHECK( tree.get<std::string>("2") == "6" );

    // errors leave the parser in a consistent state
    tree = parser.parse_string("[7, 8, 9");
    BOOST_CHECK( parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "7" );
    BOOST_CHECK( tree.get<std::string>("1") == "8" );
    BOOST_CHECK( tree.get<std::string>("2") == "9" );

    tree = parser.parse_string("[]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( !tree.get_optional<std::string>("0") );

    tree = parser.parse_string("[[0,1],[2]]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0.0") == "0" );
    BOOST_CHECK( tree.get<std::string>("0.1") == "1" );
    BOOST_CHECK( tree.get<std::string>("1.0") == "2" );

}

BOOST_AUTO_TEST_CASE( test_object )
{
    JsonParser parser;
    parser.throws(false);
    JsonParser::Tree tree;

    tree = parser.parse_string("{}");
    BOOST_CHECK( !parser.error() );

    tree = parser.parse_string(R"({foo: "bar"})");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("foo") == "bar" );

    tree = parser.parse_string(R"({"foo": "bar"})");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("foo") == "bar" );

    tree = parser.parse_string(R"({foo: "bar", hello: "world"})");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("foo") == "bar" );
    BOOST_CHECK( tree.get<std::string>("hello") == "world" );

    tree = parser.parse_string(R"({foo: "bar", hello: "world",})");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("foo") == "bar" );
    BOOST_CHECK( tree.get<std::string>("hello") == "world" );

    tree = parser.parse_string(R"({foo: {hello: "world"}})");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("foo.hello") == "world" );

    tree = parser.parse_string(R"({foo: {hello: "bar")");
    BOOST_CHECK( parser.error() );
    BOOST_CHECK( tree.get<std::string>("foo.hello") == "bar" );
}

BOOST_AUTO_TEST_CASE( test_values )
{
    JsonParser parser;
    parser.throws(false);
    JsonParser::Tree tree;

    tree = parser.parse_string("[123]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<int>("0") == 123 );

    tree = parser.parse_string("[12.3]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( int(tree.get<double>("0")*10) == 123 );

    tree = parser.parse_string("[true, false]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<bool>("0") == true );
    BOOST_CHECK( tree.get<bool>("1") == false );

    tree = parser.parse_string("[null]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( !tree.get_optional<std::string>("0") );

    tree = parser.parse_string("[foo]");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "foo" );
}

BOOST_AUTO_TEST_CASE( test_string )
{
    JsonParser parser;
    parser.throws(false);
    JsonParser::Tree tree;

    tree = parser.parse_string(R"(["123"])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );

    tree = parser.parse_string(R"(["12\"3"])"); // "
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "12\"3" );

    tree = parser.parse_string(R"(["\b\f\r\t\n\\\"\/"])"); // "
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "\b\f\r\t\n\\\"/" );

    tree = parser.parse_string(R"(["\u0020"])"); // "
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == " " );

    tree = parser.parse_string(R"(["\u00E6"])"); // "
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "æ" );
}

BOOST_AUTO_TEST_CASE( test_comments )
{
    JsonParser parser;
    parser.throws(false);
    JsonParser::Tree tree;

    tree = parser.parse_string(R"([   123])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );

    tree = parser.parse_string(R"( [
        "123"])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );

    tree = parser.parse_string(R"([// hello
        123])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );

    tree = parser.parse_string(R"([/*hello*/123])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );

    tree = parser.parse_string(R"([ /*hello
    world*/
    123])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );

    tree = parser.parse_string(R"([/**/123])");
    BOOST_CHECK( !parser.error() );
    BOOST_CHECK( tree.get<std::string>("0") == "123" );
}


BOOST_AUTO_TEST_CASE( test_node_value_ctor )
{
    JsonNode empty;
    BOOST_CHECK(empty.type() == JsonNode::Object);
    BOOST_CHECK(empty.raw_value() == "");
    BOOST_CHECK(empty.children().empty());
    BOOST_CHECK_THROW(empty.value_string(), JsonError);
    BOOST_CHECK_THROW(empty.value_bool(), JsonError);
    BOOST_CHECK_THROW(empty.value_int(), JsonError);

    JsonNode number(1);
    BOOST_CHECK(number.type() == JsonNode::Number);
    BOOST_CHECK(number.raw_value() == "1");
    BOOST_CHECK(number.children().empty());
    BOOST_CHECK_THROW(number.value_string(), JsonError);
    BOOST_CHECK_THROW(number.value_bool(), JsonError);
    BOOST_CHECK(number.value_int() == 1);

    JsonNode booleant(true);
    BOOST_CHECK(booleant.type() == JsonNode::Boolean);
    BOOST_CHECK(booleant.raw_value() == "true");
    BOOST_CHECK(booleant.children().empty());
    BOOST_CHECK_THROW(booleant.value_string(), JsonError);
    BOOST_CHECK(booleant.value_bool() == true);
    BOOST_CHECK_THROW(booleant.value_int(), JsonError);

    JsonNode booleanf(false);
    BOOST_CHECK(booleanf.type() == JsonNode::Boolean);
    BOOST_CHECK(booleanf.raw_value() == "false");
    BOOST_CHECK(booleanf.children().empty());
    BOOST_CHECK_THROW(booleanf.value_string(), JsonError);
    BOOST_CHECK(booleanf.value_bool() == false);
    BOOST_CHECK_THROW(booleanf.value_int(), JsonError);

    JsonNode null(nullptr);
    BOOST_CHECK(null.type() == JsonNode::Null);
    BOOST_CHECK(null.raw_value() == "");
    BOOST_CHECK(null.children().empty());
    BOOST_CHECK_THROW(null.value_string(), JsonError);
    BOOST_CHECK_THROW(null.value_bool(), JsonError);
    BOOST_CHECK_THROW(null.value_int(), JsonError);

    JsonNode string("foo");
    BOOST_CHECK(string.type() == JsonNode::String);
    BOOST_CHECK(string.raw_value() == "foo");
    BOOST_CHECK(string.children().empty());
    BOOST_CHECK(string.value_string() == "foo");
    BOOST_CHECK_THROW(string.value_bool(), JsonError);
    BOOST_CHECK_THROW(string.value_int(), JsonError);
}

BOOST_AUTO_TEST_CASE( test_node_ctor_ptree )
{
    boost::property_tree::ptree ptree;
    ptree.put("foo.bar", "foo");
    ptree.put("foo.baz", "bar");
    ptree.put("hello", "world");

    JsonNode node(ptree);
    BOOST_CHECK(node.type() == JsonNode::Object);
    BOOST_CHECK(node.children().size() == 2);

    auto foo = node["foo"];
    BOOST_CHECK(foo.type() == JsonNode::Object);
    BOOST_CHECK(foo.children().size() == 2);
    BOOST_CHECK(foo["bar"].type() == JsonNode::String);
    BOOST_CHECK(foo["bar"].value_string() == "foo");
    BOOST_CHECK(foo["baz"].type() == JsonNode::String);
    BOOST_CHECK(foo["baz"].value_string() == "bar");

    auto hello = node["hello"];
    BOOST_CHECK(hello.type() == JsonNode::String);
    BOOST_CHECK(hello.value_string() == "world");
}

BOOST_AUTO_TEST_CASE( test_node_to_ptree )
{
    boost::property_tree::ptree ptree;
    ptree = JsonNode(nullptr).to_ptree();
    BOOST_CHECK(ptree.data() == "null");
    BOOST_CHECK(ptree.size() == 0);

    ptree = JsonNode(123).to_ptree();
    BOOST_CHECK(ptree.data() == "123");
    BOOST_CHECK(ptree.size() == 0);

    ptree = JsonNode(true).to_ptree();
    BOOST_CHECK(ptree.data() == "true");
    BOOST_CHECK(ptree.size() == 0);

    ptree = JsonNode("foo").to_ptree();
    BOOST_CHECK(ptree.data() == "foo");
    BOOST_CHECK(ptree.size() == 0);

    ptree = JsonNode().to_ptree();
    BOOST_CHECK(ptree.data() == "");
    BOOST_CHECK(ptree.size() == 0);

    JsonNode node;
    node.put("foo.bar", "Bar");
    node.put("foo.baz", "Baz");
    ptree = node.to_ptree();
    BOOST_CHECK(ptree.data() == "");
    BOOST_CHECK(ptree.size() == 1);
    BOOST_CHECK(ptree.get_child("foo").size() == 2);
    BOOST_CHECK_EQUAL(ptree.get<std::string>("foo.bar"), "Bar");
    BOOST_CHECK_EQUAL(ptree.get<std::string>("foo.baz"), "Baz");
}

BOOST_AUTO_TEST_CASE( test_node_put_child )
{
    JsonNode node;
    BOOST_CHECK_THROW(node.put_child(""), JsonError);

    JsonNode* child = &node.put_child("foo.bar");
    BOOST_CHECK_EQUAL(node.size(), 1);
    BOOST_CHECK_EQUAL(&node["foo"]["bar"], child);
    node.put_child("foo.bar.hello");
    node.put_child("foo.bar.world");
    BOOST_CHECK_EQUAL(child->size(), 2);

    child = &node.put_child("bar.foo", JsonNode(123));
    BOOST_CHECK_EQUAL(node.size(), 2);
    BOOST_CHECK_EQUAL(&node["bar"]["foo"], child);
    BOOST_CHECK_EQUAL(child->type(), JsonNode::Number);
    BOOST_CHECK_EQUAL(child->value_int(), 123);
    BOOST_CHECK_EQUAL(child->size(), 0);
    BOOST_CHECK_THROW(node.put_child("bar.foo.hello"), JsonError);

    JsonNode* owchild = &node.put_child("bar.foo", JsonNode(true));
    BOOST_CHECK_EQUAL(owchild, child);
    BOOST_CHECK_EQUAL(child->type(), JsonNode::Boolean);
    BOOST_CHECK_EQUAL(child->value_bool(), true);
    JsonNode* addchild = &node.add_child("bar.foo", JsonNode(567));
    BOOST_CHECK_NE(addchild, child);
    BOOST_CHECK_EQUAL(addchild->type(), JsonNode::Number);
    BOOST_CHECK_EQUAL(addchild->value_int(), 567);
}

BOOST_AUTO_TEST_CASE( test_node_put )
{
    JsonNode node;
    BOOST_CHECK_THROW(node.put("", 123), JsonError);

    JsonNode* child = &node.put("foo.bar", 123);
    BOOST_CHECK_EQUAL(node.size(), 1);
    BOOST_CHECK_EQUAL(&node["foo"]["bar"], child);
    BOOST_CHECK_EQUAL(child->type(), JsonNode::Number);
    BOOST_CHECK_EQUAL(child->value_int(), 123);
}

BOOST_AUTO_TEST_CASE( test_node_get_child )
{
    JsonNode node;
    node.put("foo.bar", 123);
    node.put("foo.baz", true);
    JsonNode *child = &node.get_child("foo.bar");
    BOOST_CHECK_EQUAL(child->type(), JsonNode::Number);
    BOOST_CHECK_EQUAL(child->value_int(), 123);
    BOOST_CHECK_EQUAL(child, &node["foo"]["bar"]);

    child = &node.get_child("foo");
    BOOST_CHECK_EQUAL(child->type(), JsonNode::Object);
    BOOST_CHECK_EQUAL(child->size(), 2);
}

BOOST_AUTO_TEST_CASE( test_node_to_json )
{
    std::ostringstream ss;
    ss.str(""); ss << JsonNode();
    BOOST_CHECK_EQUAL(ss.str(), "{}");
    ss.str(""); ss << JsonNode(123);
    BOOST_CHECK_EQUAL(ss.str(), "123");
    ss.str(""); ss << JsonNode(true);
    BOOST_CHECK_EQUAL(ss.str(), "true");
    ss.str(""); ss << JsonNode(nullptr);
    BOOST_CHECK_EQUAL(ss.str(), "null");
    ss.str(""); ss << JsonNode("\xe2\x82\xac foo\n");
    BOOST_CHECK_EQUAL(ss.str(), R"("\u20ac foo\n")");

    JsonNode node;
    node.put("foo.bar", 123);
    node.put("foo.baz", true);
    ss.str(""); ss << node;
    BOOST_CHECK_EQUAL(ss.str(), R"({"foo":{"bar":123,"baz":true}})");
}

BOOST_AUTO_TEST_CASE( test_node_to_json_pretty )
{
    JsonNode node;
    node.put("foo.bar", 123);
    node.put("foo.baz", true);
    std::ostringstream ss;
    node.format(ss, 2);
    BOOST_CHECK_EQUAL(ss.str(),
R"({
  "foo": {
    "bar": 123,
    "baz": true
  }
})");

}
