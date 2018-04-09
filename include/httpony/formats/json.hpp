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

    inline void quote(const std::string& str, std::ostream& out)
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
            }
            out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << unicode.point() << std::dec;
        }
        out << '"';
    }

    inline void add_indent(std::ostream& out, int indent)
    {
        if ( !indent )
            return;
        out.put('\n');
        for ( int i = 0; i < indent; i++ )
            out.put(' ');
    }
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

    JsonNode() : type_(Object) {}
    explicit JsonNode(std::nullptr_t) : type_(Null) {}
    explicit JsonNode(long value)
      : type_(Number),
        value_(melanolib::string::to_string(value))
    {}
    explicit JsonNode(bool value)
      : type_(Boolean),
      value_(value ? "true" : "false")
    {}
    explicit JsonNode(std::string value)
      : type_(String),
      value_(std::move(value))
    {}

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
            case Number:
            case String:
            case Boolean:
                return boost::property_tree::ptree(value_);
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

    void format(std::ostream& out, int indent = 0, int indent_depth = 0 ) const
    {
        switch ( type_ )
        {
            case Null:
                out << "null";
                break;
            case String:
                detail::quote(value_, out);
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
                    detail::add_indent(out, indent * (indent_depth + 1));
                    detail::quote(pair.first, out);
                    out << ':';
                    pair.second.format(out, indent, indent_depth + 1);
                }
                detail::add_indent(out, indent * indent_depth);
                out << '}';
                break;
            }
            case Array:
            {
                out << '[';
                int n = 0;
                for ( const auto& pair : children_ )
                {
                    detail::add_indent(out, indent * (indent_depth + 1));
                    if ( n++ ) out << ',';
                    pair.second.format(out);
                }
                detail::add_indent(out, indent * indent_depth);
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

// pt
    void clear()
    {
        children_.clear();
        value_.clear();
        type_ = Object;
    }

    JsonNode& get_child(const path_type& path)
    {
        const JsonNode* child = find_child_ptr(path);
        return const_cast<JsonNode&>(*child);
    }

    const JsonNode& get_child(const path_type& path) const
    {
        return *find_child_ptr(path);
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

    JsonNode& put_child(const path_type& path, JsonNode node = {})
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
            if ( parent->type_ != Object )
                throw JsonError("", 0, "Not an object");
        }
        parent->children_.push_back({last, node});
    }

    JsonNode& put(const path_type& path, long value)
    {
        return put_child(path, JsonNode(value));
    }

    JsonNode& put(const path_type& path, std::string value)
    {
        return put_child(path, JsonNode(std::move(value)));
    }

    JsonNode& put(const path_type& path, std::nullptr_t value)
    {
        return put_child(path, JsonNode(value));
    }

    JsonNode& put(const path_type& path, bool value)
    {
        return put_child(path, JsonNode(value));
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

private:
    Type type_;
    std::string value_;
    container children_;
};

/**
 * \brief Class that populates a property tree from a JSON in a stream
 *
 * Unlike from boost::property_tree::json_parser::read_json
 * it reads arrays element with their numeric index, is a bit forgiving
 * about syntax errors and allows simple unquoted strings
 */
class JsonParser
{
public:
    using Tree = boost::property_tree::ptree;

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
            ptree.put_child(context_pos(), {});

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
                        r += melanolib::string::Utf8Parser::encode(melanolib::string::to_uint(hex, 16));

                        continue;
                    }

                    if ( !detail::escapeable(c) )
                        r += '\\';
                }

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

} // namespace json
} // namespace httpony
#endif // HTTPONY_JSON_HPP
