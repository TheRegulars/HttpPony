/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2015-2016 Mattia Basaglia
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
#ifndef HTTPONY_JSON_HPP
#define HTTPONY_JSON_HPP

/// \cond
#include <fstream>
#include <istream>
#include <stack>
#include <sstream>
#include <string>
#include <iomanip>
#include <list>

#include <boost/property_tree/ptree.hpp>

#include <melanolib/string/stringutils.hpp>
#include <melanolib/string/encoding.hpp>
/// \endcond

namespace httpony {
namespace json {

namespace detail {
    /**
     * \brief Whether \c c can be espaced on a string literal
     */
    constexpr inline bool escapeable(char c)
    {
        switch ( c )
        {
            case '\b': case '\f': case '\r': case '\t': case '\n':
            case '\\': case '\"': case '/':
                return true;
        }
        return false;
    }

    constexpr inline char escape(char c)
    {
        switch ( c )
        {
            case '\b': return 'b';
            case '\f': return 'f';
            case '\r': return 'r';
            case '\t': return 't';
            case '\n': return 'n';
            default:   return c;
        }
    }

    inline void print_uniescape(std::ostream& out, uint32_t point, bool unicode_surrogates)
    {
        out << "\\u" << std::hex << std::setw(4) << std::setfill('0');
        if ( !unicode_surrogates || !melanolib::string::Utf8Parser::can_split_surrogates(point) )
        {
            out << point;
        }
        else
        {
            auto p = melanolib::string::Utf8Parser::split_surrogates(point);
            out << p.first << "\\u" << std::setw(4) << p.second;
        }
        out << std::dec;
    }

    inline void quote(const std::string& str, std::ostream& out,
                      bool unicode_surrogates)
    {
        melanolib::string::Utf8Parser parser(str);
        out << '"';
        while ( !parser.finished() )
        {
            auto unicode = parser.next();
            if ( unicode.is_ascii() )
            {
                char c = unicode.point();
                if ( escapeable(c) )
                    out << '\\' << escape(c);
                else
                    out << c;
            }
            else
            {
                print_uniescape(out, unicode.point(), unicode_surrogates);
            }
        }
        out << '"';
    }

    inline void add_indent(std::ostream& out, int indent, int depth)
    {
        if ( !indent )
            return;
        out.put('\n');
        for ( int i = 0; i < indent * depth; i++ )
            out.put(' ');
    }

    template<class T>
    using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;



    template<class T> void tree_node_to_array(T& node){}

} // namespace detail

/**
 * \brief Error encountered when parsing JSON
 */
struct JsonError : public std::runtime_error
{
    std::string file;     ///< file name originating the error
    int         line;     ///< line number originating the error

    JsonError(std::string file, int line, std::string msg)
        : std::runtime_error(msg),
        file(std::move(file)),
        line(line)
    {}
};


class JsonNode
{
public:
    enum Type {
        Null,
        String,
        Number,
        Boolean,
        Object,
        Array,
    };
    using path_type = std::string;
    using key_type = std::string;
    using mapped_type = JsonNode;
    using value_type = std::pair<key_type, mapped_type>;
    using container = std::list<value_type>;
    using iterator = container::iterator;
    using const_iterator = container::const_iterator;
    using size_type = std::size_t;

    JsonNode() : type_(Object) {}
    explicit JsonNode(std::nullptr_t) : type_(Null) {}
    explicit JsonNode(long value)
      : type_(Number),
        value_(melanolib::string::to_string(value))
    {}
    explicit JsonNode(double value)
      : type_(Number),
        value_(melanolib::string::to_string(value))
    {}
    explicit JsonNode(int value) : JsonNode(long(value)){}
    explicit JsonNode(bool value)
      : type_(Boolean),
      value_(value ? "true" : "false")
    {}
    explicit JsonNode(std::string value)
      : type_(String),
      value_(std::move(value))
    {}
    explicit JsonNode(const char* value) : JsonNode(std::string(value)) {}

    explicit JsonNode(const boost::property_tree::ptree& ptree)
        : type_(Object)
    {
        if ( ptree.empty() )
        {
            type_ = String;
            value_ = ptree.data();
        }
        else
        {
            for ( const auto& pair : ptree )
            {
                children_.push_back({pair.first, JsonNode(pair.second)});
            }
        }
    }

    boost::property_tree::ptree to_ptree() const
    {
        switch ( type_ )
        {
            case Null:
                return boost::property_tree::ptree("null");
            case Object:
            case Array:
            {
                boost::property_tree::ptree out;
                for ( const auto& pair : children_ )
                {
                    out.push_back({pair.first, pair.second.to_ptree()});
                }
                return out;
            }
            case Number:
            case String:
            case Boolean:
            default:
                return boost::property_tree::ptree(value_);
        }
    }

    const std::string& raw_value() const
    {
        return value_;
    }

    const auto& children() const
    {
        return children_;
    }

    Type type() const
    {
        return type_;
    }

    void set_is_object()
    {
        type_ = Object;
        value_.clear();
    }

    long value_int() const
    {
        if ( type_ != Number )
            throw JsonError("", 0, "Not a number value");
        return melanolib::string::to_int(value_);
    }

    double value_float() const
    {
        if ( type_ != Number )
            throw JsonError("", 0, "Not a number value");
        return std::stod(value_);
    }

    void set_value_bool(long value)
    {
        type_ = Number;
        value_ = melanolib::string::to_string(value);
    }

    bool value_bool() const
    {
        if ( type_ != Boolean )
            throw JsonError("", 0, "Not a boolean value");
        return value_ == "true";
    }

    void set_value_bool(bool value)
    {
        type_ = Boolean;
        value_ = value ? "true" : "false";
    }

    const std::string& value_string() const
    {
        if ( type_ != String )
            throw JsonError("", 0, "Not a string value");
        return value_;
    }

    void set_value_string(const std::string& value)
    {
        type_ = String;
        value_ = value;
    }

    void to_array()
    {
        type_ = Array;
        value_.clear();
    }

    void format(std::ostream& out, int indent = 0, int indent_depth = 0,
                bool unicode_surrogates = false ) const
    {
        switch ( type_ )
        {
            case Null:
                out << "null";
                break;
            case String:
                detail::quote(value_, out, unicode_surrogates);
                break;
            case Number:
            case Boolean:
                out << value_;
                break;
            case Object:
            {
                out << '{';
                int n = 0;
                for ( const auto& pair : children_ )
                {
                    if ( n++ ) out << ',';
                    detail::add_indent(out, indent, (indent_depth + 1));
                    detail::quote(pair.first, out, unicode_surrogates);
                    out << ':';
                    if ( indent )
                        out << ' ';
                    pair.second.format(out, indent, indent_depth + 1, unicode_surrogates);
                }
                detail::add_indent(out, indent, indent_depth);
                out << '}';
                break;
            }
            case Array:
            {
                out << '[';
                int n = 0;
                for ( const auto& pair : children_ )
                {
                    detail::add_indent(out, indent, (indent_depth + 1));
                    if ( n++ ) out << ',';
                    pair.second.format(out, indent, indent_depth, unicode_surrogates);
                }
                detail::add_indent(out, indent, indent_depth);
                out << ']';
                break;
            }

        }
    }

    friend std::ostream& operator<<(std::ostream& os, const JsonNode& node)
    {
        node.format(os);
        return os;
    }

    const_iterator find(const key_type& key) const
    {
        auto iter = children_.begin();
        for ( auto end = children_.end(); iter != end; ++iter )
            if ( iter->first == key )
                break;
        return iter;
    }

    iterator find(const key_type& key)
    {
        auto iter = children_.begin();
        for ( auto end = children_.end(); iter != end; ++iter )
            if ( iter->first == key )
                break;
        return iter;
    }

    mapped_type& operator[](const key_type& key)
    {
        auto iter = find(key);
        if ( iter == children_.end() )
        {
            iter = children_.emplace(iter, key, mapped_type());
        }
        return iter->second;
    }

    std::string get_raw(const path_type& path, const std::string& default_value) const
    {
        try {
            return get_child(path).raw_value();
        } catch (const JsonError&) {
            return default_value;
        }
    }

// pt - sequence
    void clear()
    {
        children_.clear();
        value_.clear();
        type_ = Object;
    }

    size_type size() const
    {
        return children_.size();
    }

    void push_back(const value_type& pair)
    {
        if ( type_ != Object && type_ != Array )
            throw JsonError("", 0, "Not an object");
        children_.push_back(pair);
    }

    value_type& back()
    {
        return children_.back();
    }

    const value_type& back() const
    {
        return children_.back();
    }

    iterator begin() { return children_.begin(); }
    const_iterator cbegin() const { return children_.begin(); }
    const_iterator begin() const { return children_.begin(); }
    iterator end() { return children_.end(); }
    const_iterator cend() const { return children_.end(); }
    const_iterator end() const { return children_.end(); }

// pt
    size_type count(const key_type & key) const
    {
        size_type count = 0;
        for ( const auto& pair : children_ )
            if ( pair.first == key )
                count++;
        return count;
    }

    JsonNode& get_child(const path_type& path)
    {
        const JsonNode* child = find_child_ptr(path);
        if ( !child )
            throw JsonError("", 0, "Node not found");
        return const_cast<JsonNode&>(*child);
    }

    const JsonNode& get_child(const path_type& path) const
    {
        const JsonNode* child = find_child_ptr(path);
        if ( !child )
            throw JsonError("", 0, "Node not found");
        return *child;
    }

    JsonNode& get_child(const path_type& path, JsonNode& default_child)
    {
        const JsonNode* child = find_child_ptr(path);
        if ( !child )
            return default_child;
        return const_cast<JsonNode&>(*child);
    }

    const JsonNode& get_child(const path_type& path, const JsonNode& default_child) const
    {
        const JsonNode* child = find_child_ptr(path);
        if ( !child )
            return default_child;
        return *child;
    }

    melanolib::Optional<JsonNode&> get_child_optional(const path_type& path)
    {
        const JsonNode* child = find_child_ptr(path);
        if ( !child )
            return {};
        return const_cast<JsonNode&>(*child);
    }

    melanolib::Optional<const JsonNode&> get_child_optional(const path_type& path) const
    {
        const JsonNode* child = find_child_ptr(path);
        if ( !child )
            return {};
        return *child;
    }

    JsonNode& add_child(const path_type& path, JsonNode node = {})
    {
        JsonNode* parent;
        std::string last;
        std::tie(parent, last) = add_parent(path);
        parent->children_.push_back({last, std::move(node)});
        return parent->children_.back().second;
    }

    JsonNode& put_child(const path_type& path, JsonNode node = {})
    {
        JsonNode* parent;
        std::string last;
        std::tie(parent, last) = add_parent(path);
        auto iter = parent->find(last);
        if ( iter == parent->children_.end() )
        {
            parent->children_.push_back({last, std::move(node)});
            return parent->children_.back().second;
        }
        else
        {
            iter->second = std::move(node);
            return iter->second;
        }
    }

    template<class T>
    JsonNode& put(const path_type& path, T&& value)
    {
        return put_child(path, JsonNode(std::forward<T>(value)));
    }

    template<class Type>
    std::enable_if_t<
        std::is_integral<detail::remove_cvref_t<Type>>::value
        && !std::is_same<detail::remove_cvref_t<Type>, bool>::value,
        detail::remove_cvref_t<Type>
    > get_value() const
    {
        return value_int();
    }

    template<class Type>
    std::enable_if_t<
        std::is_floating_point<detail::remove_cvref_t<Type>>::value,
        detail::remove_cvref_t<Type>
    > get_value() const
    {
        return value_float();
    }

    template<class Type>
    std::enable_if_t<
        std::is_same<detail::remove_cvref_t<Type>, bool>::value,
        bool
    > get_value() const
    {
        return value_bool();
    }

    template<class Type>
    std::enable_if_t<
        std::is_pointer<std::decay_t<Type>>::value &&
        std::is_same<
            std::remove_cv_t<std::remove_pointer_t<std::decay_t<Type>>>,
            char
        >::value,
        std::string
    > get_value() const
    {
        return value_string();
    }

    template<class Type>
    std::enable_if_t<
        std::is_convertible<std::string, detail::remove_cvref_t<Type>>::value
        && !std::is_same<
             detail::remove_cvref_t<Type>,
             std::string
        >::value,
        detail::remove_cvref_t<Type>
    > get_value() const
    {
        return value_string();
    }

    template<class Type>
    std::enable_if_t<
        std::is_same<detail::remove_cvref_t<Type>, std::string>::value,
        const std::string&
    > get_value() const
    {
        return value_string();
    }

    template<class Type>
    auto get_value(Type&& default_value) const -> decltype(get_value<Type>())
    {
        try {
            return get_value<Type>();
        } catch (const JsonError&) {
            return std::forward<Type>(default_value);
        }
    }


    template<class Type>
    auto get(const path_type& path) const -> decltype(get_value<Type>())
    {
        return get_child(path).get_value<Type>();
    }

    template<class Type>
    auto get(const path_type& path, Type&& default_value) const -> decltype(get_value<Type>())
    {
        try {
            return get_child(path).get_value<Type>();
        } catch (const JsonError&) {
            return std::forward<Type>(default_value);
        }
    }

    template<class Type>
    auto get_optional(const path_type& path) const -> melanolib::Optional<decltype(get_value<Type>())>
    {
        try {
            return get_child(path).get_value<Type>();
        } catch (const JsonError&) {
            return {};
        }
    }

private:
    const JsonNode* find_child_ptr(const path_type& path) const
    {
        auto pieces = melanolib::string::char_split(path, '.');
        const JsonNode* parent = this;
        for ( const auto& piece : pieces )
        {
            auto iter = parent->find(piece);
            if ( iter == parent->children_.end() )
                return nullptr;
            parent = &iter->second;
        }
        return parent;
    }

    std::pair<JsonNode*, std::string> add_parent(const path_type& path)
    {
        auto pieces = melanolib::string::char_split(path, '.');
        if ( pieces.size() < 1 )
            throw JsonError("", 0, "Missing path");

        auto last = pieces.back();
        pieces.pop_back();
        JsonNode* parent = this;
        for ( const auto& piece : pieces )
        {
            parent = &(*parent)[piece];
            if ( parent->type_ != Object && parent->type_ != Array )
                throw JsonError("", 0, "Not an object");
        }

        return {parent, last};
    }

private:
    Type type_;
    std::string value_;
    container children_;
};

namespace detail{
    template<> inline void tree_node_to_array(JsonNode& node)
    {
        node.to_array();
    }
} // namespace detail

/**
 * \brief Class that populates a property tree from a JSON in a stream
 *
 * Unlike from boost::property_tree::json_parser::read_json
 * it reads arrays element with their numeric index, is a bit forgiving
 * about syntax errors and allows simple unquoted strings
 */
template<class TreeT>
class JsonParserGeneric
{
public:
    using Tree = TreeT;

    /**
     * \brief Parse the stream
     * \param stream      Input stream
     * \param stream_name Used for error reporting
     * \throws JsonError On bad syntax
     * \returns The property tree populated from the stream
     */
    const Tree& parse(std::istream& stream, const std::string& stream_name = "")
    {
        this->stream.rdbuf(stream.rdbuf());
        this->stream_name = stream_name;
        line = 1;
        parse_json_root();
        return ptree;
    }

    /**
     * \brief Parse the given file
     * \throws JsonError On bad syntax
     * \returns The property tree populated from the stream
     */
    const Tree& parse_file(const std::string& file_name)
    {
        std::filebuf buf;
        line = 1;
        stream_name = file_name;
        if ( !buf.open(file_name, std::ios::in) )
            error("Cannot open file");
        this->stream.rdbuf(&buf);
        parse_json_root();
        return ptree;
    }

    /**
     * \brief Parse the string
     * \throws JsonError On bad syntax
     * \returns The property tree populated from the stream
     */
    const Tree& parse_string(const std::string& json,
                             const std::string& stream_name = "")
    {
        std::istringstream ss(json);
        return parse(ss, stream_name);
    }

    /**
     * \brief This can be used to get partial trees on errors
     */
    const Tree& tree() const
    {
        return ptree;
    }

    /**
     * \brief Whether there has been a parsing error
     */
    bool error() const
    {
        return error_flag;
    }

    /**
     * \brief Whether the parser can throw
     */
    bool throws() const
    {
        return !nothrow;
    }

    /**
     * \brief Sets whether the parser can throw
     */
    void throws(bool throws)
    {
        nothrow = !throws;
    }

private:
    /**
     * \brief Throws an exception pointing to the current line
     */
    void error /*[[noreturn]]*/ (const std::string& message)
    {
        throw JsonError(stream_name, line, message);
    }

    /**
     * \brief Parses the entire file
     */
    void parse_json_root()
    {
        error_flag = false;
        ptree.clear();
        context = decltype(context)();

        if ( nothrow )
        {
            try {
                parse_json_root_throw();
            } catch ( const JsonError& err ) {
                error_flag = true;
            }
        }
        else
        {
            parse_json_root_throw();
        }
    }

    void parse_json_root_throw()
    {
        char c = get_skipws();
        if ( stream.eof() )
            return;
        stream.unget();
        if ( c == '[' )
            parse_json_array();
        else
            parse_json_object();

        if ( !context.empty() )
            error("Abrupt ending");
    }

    /**
     * \brief Parse a JSON object
     */
    void parse_json_object()
    {
        char c = get_skipws();
        if ( c != '{' )
            error("Expected object");

        if ( !context.empty() )
            ptree.put_child(context_pos(), {});

        parse_json_properties();
    }

    /**
     * \brief Parse object properties
     */
    void parse_json_properties()
    {
        char c = get_skipws();
        while ( true )
        {
            if ( !stream )
                error("Expected } or ,");

            if ( c == '}' )
                break;

            if ( c == '\"' )
            {
                stream.unget();
                context_push(parse_json_string());
            }
            else if ( std::isalpha(c) )
            {
                stream.unget();
                context_push(parse_json_identifier());
            }
            else
                error("Expected property name");


            c = get_skipws();
            if ( c != ':' )
                stream.unget();

            parse_json_value();
            context_pop();

            c = get_skipws();

            if ( c == ',' )
                c = get_skipws();
        }
    }

    /**
     * \brief Parse a JSON array
     */
    void parse_json_array()
    {
        char c = get_skipws();
        if ( c != '[' )
            error("Expected array");

        if ( !context.empty() )
            detail::tree_node_to_array(ptree.put_child(context_pos(), {}));
        else
            detail::tree_node_to_array(ptree);

        context_push_array();
        parse_json_array_elements();
        context_pop();
    }

    /**
     * \brief Parse array elements
     */
    void parse_json_array_elements()
    {
        char c = get_skipws();

        while ( true )
        {
            if ( !stream )
                error("Expected ]");

            if ( c == ']' )
                break;

            stream.unget();

            parse_json_value();

            c = get_skipws();

            if ( c == ',' )
                c = get_skipws();

            context.top().array_index++;
        }
    }

    /**
     * \brief Parse a value (number, string, array, object, or other literal)
     */
    void parse_json_value()
    {
        char c = get_skipws();
        stream.unget();
        if ( c == '{' )
            parse_json_object();
        else if ( c == '[' )
            parse_json_array();
        else
            parse_json_literal();

    }

    /**
     * \brief Parse numeric, string and boolean literals
     */
    void parse_json_literal()
    {
        char c = get_skipws();
        if ( std::isalpha(c) )
        {
            stream.unget();
            std::string val = parse_json_identifier();
            if ( val == "true" )
                put(true);
            else if ( val == "false" )
                put(false);
            else if ( val != "null" ) // Null => no value
                put(val);
        }
        else if ( c == '\"' )
        {
            stream.unget();
            put(parse_json_string());
        }
        else if ( std::isdigit(c) || c == '.' || c == '-' || c == '+' )
        {
            stream.unget();
            put(parse_json_number());
        }
        else
        {
            error("Expected value");
        }
    }

    /**
     * \brief Parse a string literal
     */
    std::string parse_json_string()
    {
        char c = get_skipws();
        std::string r;
        uint16_t surrogate = 0;

        if ( c == '\"' )
        {
            while ( true )
            {
                c = stream.get();
                if ( !stream || c == '\"' )
                    break;
                if ( c == '\\' )
                {
                    c = unescape(stream.get());
                    if ( !stream )
                        break;

                    if ( c == 'u' )
                    {
                        char hex[] = "0000";
                        stream.read(hex, 4);
                        for ( int i = 0; i < 4; i++ )
                        {
                            if ( !std::isxdigit(hex[i]) )
                                hex[i] = '0';
                        }

                        uint32_t unipoint = melanolib::string::to_uint(hex, 16);
                        if ( surrogate )
                        {
                            if ( melanolib::string::Utf8Parser::is_low_surrogate(unipoint) )
                                unipoint = melanolib::string::Utf8Parser::combine_surrogates(surrogate, unipoint);
                            surrogate = 0;
                        }

                        if ( melanolib::string::Utf8Parser::is_high_surrogate(unipoint) )
                            surrogate = unipoint;
                        else
                            r += melanolib::string::Utf8Parser::encode(unipoint);

                        continue;
                    }

                    if ( !detail::escapeable(c) )
                        r += '\\';
                }

                surrogate = 0;

                r += c;

                if ( c == '\n' )
                    line++;
            }
        }

        return r;
    }

    /**
     * \brief Parses an identifier
     */
    std::string parse_json_identifier()
    {
        char c = get_skipws();
        if ( !std::isalpha(c) )
            return {};

        std::string r;
        while ( true )
        {
            r += c;
            c = stream.get();
            if ( !stream || ( !std::isalnum(c) && c != '_' && c != '-' ) )
                break;
        }
        stream.unget();

        return r;
    }

    /**
     * \brief Parse a numeric literal
     */
    double parse_json_number()
    {
        double d = 0;

        if ( !(stream >> d) )
        {
            stream.get();
            error("Expected numeric literal");
        }

        return d;
    }

    /**
     * \brief Get the next non-space character in the stream
     */
    char get_skipws()
    {
        char c;
        do
        {
            c = stream.get();
            if ( c == '/' )
            {
                // single line comments
                if ( stream.peek() == '/' )
                {
                    while ( stream && c != '\n' )
                        c = stream.get();
                }
                // multi-line comments
                else if ( stream.peek() == '*' )
                {
                    stream.ignore();
                    skip_comment();
                    c = stream.get();
                }
            }
            if ( c == '\n' )
                line++;
        }
        while ( stream && std::isspace(c) );
        return c;
    }

    /**
     * \brief Skips all characters until * /
     */
    void skip_comment()
    {
        char c;
        do
        {
            c = stream.get();
            if ( c == '*' && stream.peek() == '/' )
            {
                stream.get();
                break;
            }
        }
        while ( stream );
    }

    /**
     * \brief Escape code for \\ c
     */
    char unescape(char c)
    {
        switch ( c )
        {
            case 'b': return '\b';
            case 'f': return '\f';
            case 'r': return '\r';
            case 't': return '\t';
            case 'n': return '\n';
            default:  return c;
        }
    }

    /**
     * \brief Put a value in the tree
     */
    template<class T>
        void put(const T& val)
    {
        ptree.put(context_pos(), val);
    }

    /**
     * \brief Position in the tree as defined by the current context
     */
    std::string context_pos() const
    {
        if ( context.empty() )
            return {};
        const Context& ctx = context.top();
        std::string name = ctx.name;
        if ( ctx.array_index >= 0 )
        {
            if ( !name.empty() )
                name += '.';
            name += std::to_string(ctx.array_index);
        }
        return name;
    }

    /**
     * \brief Push a named context
     */
    void context_push(const std::string&name)
    {
        std::string current = context_pos();
        if ( !current.empty() )
            current += '.';
        current += name;
        context.push({current});
    }

    /**
     * \brief Push an array context
     */
    void context_push_array()
    {
        std::string current = context_pos();
        Context ctx(current);
        ctx.array_index = 0;
        context.push(ctx);
    }

    /**
     * \brief Pop a context
     */
    void context_pop()
    {
        context.pop();
    }

    /**
     * \brief Context information
     */
    struct Context
    {
        std::string name;
        int array_index = -1;
        Context(std::string name) : name(std::move(name)) {}
    };

    std::istream stream{nullptr};///< Input stream
    std::string stream_name;    ///< Name of the file
    int line = 0;               ///< Line number
    Tree ptree; ///< Output tree
    std::stack<Context> context;///< Context stack
    bool nothrow = false;       ///< If true, don't throw
    bool error_flag = false;    ///< If true, there has been an error
};

using JsonParserPtree = JsonParserGeneric<boost::property_tree::ptree>;
using JsonParser = JsonParserGeneric<JsonNode>;

} // namespace json
} // namespace httpony
#endif // HTTPONY_JSON_HPP
