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

#include "httpony/http/parser.hpp"
#include "httpony/base_encoding.hpp"

namespace httpony {

Status Http1Parser::request(std::istream& stream, Request& request) const
{
    if ( stream.fail() || stream.bad() )
        return StatusCode::BadRequest;

    request.clear_data();

    /// \todo Some way to switch parser based on the protocol
    if ( !request_line(stream, request) )
    {
        if ( !request.protocol.valid() )
            request.protocol = Protocol::http_1_1;
        return StatusCode::BadRequest;
    }

    if ( !headers(stream, request.headers) )
        return StatusCode::BadRequest;

    if ( flags & ParseCookies )
    {
        for ( const auto& cookie_header : request.headers.key_range("Cookie") )
        {
            melanolib::string::QuickStream cookie_stream(cookie_header.second);
            if ( !header_parameters(cookie_stream, request.cookies) )
                return StatusCode::BadRequest;
        }
    }

    if ( request.headers.contains("Authorization") )
        auth(request.headers.get("Authorization"), request.auth);

    if ( request.headers.contains("Proxy-Authorization") )
        auth(request.headers.get("Proxy-Authorization"), request.proxy_auth);

    /// \todo Maybe move parsing/formatting out of UserAgent
    if ( request.headers.contains("User-Agent") )
        request.user_agent = request.headers.get("User-Agent");

    if ( request.headers.contains("Content-Length") ||
         request.headers.contains("Transfer-Encoding") )
    {
        if ( !request.body.start_input(stream.rdbuf(), request.headers) )
            return StatusCode::BadRequest;

        if ( request.protocol >= Protocol::http_1_1 && request.headers.get("Expect") == "100-continue" )
        {
            return StatusCode::Continue;
        }
    }

    if ( request.protocol < Protocol::http_1_1 && request.headers.contains("Expect") )
    {
        return StatusCode::ExpectationFailed;
    }

    return StatusCode::OK;
}

OperationStatus Http1Parser::response(std::istream& stream, Response& response) const
{
    if ( stream.fail() || stream.bad() )
        return "network error";

    response.clear_data();

    /// \todo Some way to switch parser based on the protocol
    if ( !response_line(stream, response) )
        return "malformed response";

    if ( !headers(stream, response.headers) )
        return "malformed headers";


    if ( flags & ParseCookies )
    {
        for ( const auto& cookie_header : response.headers.key_range("Set-Cookie") )
        {
            melanolib::string::QuickStream cookie_stream(cookie_header.second);
            DataMap cookie_params;
            if ( !header_parameters(cookie_stream, cookie_params) || cookie_params.empty() )
                return "malformed headers";

            Cookie cookie(cookie_params.front().second);
            response.cookies.append(cookie_params.front().first, cookie);
        }
    }

    /// \todo Parse www-authenticate

    if ( response.headers.contains("Content-Length") ||
         response.headers.contains("Transfer-Encoding") )
    {
        if ( !response.body.start_input(stream.rdbuf(), response.headers) )
            return "invalid payload";
    }

    return {};
}

void Http1Parser::skip_line(std::istream& stream) const
{
    while ( stream && stream.get() != '\n' );
}

bool Http1Parser::delimited(std::istream& stream, std::string& output, char delim, bool at_end) const
{
    output.clear();
    while ( true )
    {
        auto c = stream.get();
        if ( c == std::istream::traits_type::eof() || c == '\r' )
        {
            stream.unget();
            return at_end;
        }
        if ( c == delim )
            break;
        output += c;
    }

    return skip_spaces(stream, at_end);
}

bool Http1Parser::skip_spaces(std::istream& stream, bool at_end ) const
{
    while ( true )
    {
        auto c = stream.peek();
        if ( c == '\r' || c == std::istream::traits_type::eof() )
            return at_end;
        if ( !std::isspace(c) )
            break;
        stream.ignore(1);
    }
    return true;
}

bool Http1Parser::headers(std::istream& stream, Headers& headers) const
{
    std::string name, value;
    while ( stream && stream.peek() != '\r' )
    {
        // (obsolete) header folding
        // it's compliant to either return 400 or parse them
        if ( std::isspace(stream.peek()) )
        {
            if ( !(flags & ParseFoldedHeaders) )
                return false;

            if ( !skip_spaces(stream) || headers.empty() )
                return false;

            std::getline(stream, value, '\r');
            stream.ignore(1, '\n');
            if ( !stream )
            {
                return false;
            }
            headers.back().second += ' ' + value;
            continue;
        }

        if ( !delimited(stream, name, ':') )
            return false;

        if ( stream.peek() == '"' )
        {
            quoted_header_value(stream, value);
        }
        else
        {
            std::getline(stream, value, '\r');
            stream.ignore(1, '\n');
        }
        /// \todo Read header comments

        if ( !stream )
        {
            return false;
        }
        /// \todo validate header name
        headers.append(name, value);
    }

    skip_line(stream);

    return true;
}

bool Http1Parser::quoted_header_value(std::istream& stream, std::string& value) const
{
    stream.ignore(1, '"');
    value.clear();
    bool last_slash = false;
    while ( true )
    {
        auto c = stream.get();
        if ( c == std::char_traits<char>::eof() || c == '\r' || c == '\n' )
        {
            return false;
        }

        if ( !last_slash )
        {
            if ( c == '"'  )
                break;
            if ( c == '\\' )
            {
                last_slash = true;
                continue;
            }
        }
        else
        {
            last_slash = false;
        }

        value += c;
    }

    skip_line(stream);

    return true;
}

bool Http1Parser::request_line(std::istream& stream, Request& request) const
{
    std::string uri;
    stream >> request.method >> uri >> request.protocol;
    request.uri = uri;
    request.get = request.uri.query;
    skip_line(stream);

    return request.protocol.valid() && stream;
}

bool Http1Parser::response_line(std::istream& stream, Response& response) const
{
    stream >> response.protocol >> response.status;
    skip_line(stream);
    return response.protocol.valid() && stream;
}


bool Http1Parser::multipart_valid_boundary(const std::string& boundary) const
{
    if ( boundary.empty() )
        return false;

    /// \note This is a bit more permissive than it should
    for ( auto c : boundary )
        if ( !melanolib::string::ascii::is_print(c) )
            return false;

    if ( boundary.back() == ' ' )
        return false;

    return true;
}

enum class MultipartLineType
{
    Boundary,       ///< A --boundary line
    LastBoundary,   ///< A --boundary-- line (Marking the end of multipart data)
    Data,           ///< A non-boundary line
};

/**
 * \brief Scans the input line to determine its type (for multipart data)
 */
static MultipartLineType multipart_line_type(const std::string& line, const std::string& boundary)
{
    if ( line.size() < boundary.size() + 2 || line[0] != '-' || line[1] != '-' )
        return MultipartLineType::Data;

    if ( !std::equal(line.begin() + 2, line.begin() + 2 + boundary.size(),
                     boundary.begin(), boundary.end()) )
        return MultipartLineType::Data;

    std::size_t space_start = boundary.size() + 2;
    MultipartLineType expected_result = MultipartLineType::Boundary;

    if ( line.size() >= space_start + 2 && line[space_start] == '-' && line[space_start+1] == '-' )
    {
        expected_result = MultipartLineType::LastBoundary;
        space_start += 2;
    }

    if ( std::all_of(line.begin() + space_start, line.end(), melanolib::string::ascii::is_blank) )
        return expected_result;

    return MultipartLineType::Data;
}

/**
 * \brief Cleans up after a boundary line has been found
 *
 * The "\r\n" found before a boundary are considered part of the boundary line so
 * they need to be stripped from the data.
 */
static bool multipart_cleanup_boundary(std::istream& stream, Multipart& output)
{
    if ( stream.get() != '\n' )
        return false;

    if ( output.parts.empty() )
        return true;

    std::string& content = output.parts.back().content;

    if ( content.empty() )
        return true;

    if ( !melanolib::string::ends_with(content, "\r\n") )
        return false;

    content.resize(content.size() - 2);
    return true;
}

bool Http1Parser::multipart(std::istream& stream, Multipart& multipart) const
{
    if ( !multipart_valid_boundary(multipart.boundary) )
        return false;

    while ( true )
    {
        std::string line;
        std::getline(stream, line, '\r');
        if ( !stream )
            return false;

        auto type = multipart_line_type(line, multipart.boundary);

        if ( type == MultipartLineType::LastBoundary )
        {
            return multipart_cleanup_boundary(stream, multipart);
        }
        else if ( type == MultipartLineType::Boundary )
        {
            if ( !multipart_cleanup_boundary(stream, multipart) )
                return false;

            multipart.parts.emplace_back();
            if ( !headers(stream, multipart.parts.back().headers) )
                return false;
        }
        else if ( !multipart.parts.empty() )
        {
            multipart.parts.back().content += line + '\r';
            std::getline(stream, line, '\n');
            multipart.parts.back().content += line + '\n';
        }
        else
        {
            return false;
        }
    }
}

bool Http1Parser::auth(const std::string& header_contents, Auth& auth) const
{
    melanolib::string::QuickStream stream(header_contents);

    auth.auth_scheme = stream.get_until(melanolib::string::ascii::is_space);
    stream.ignore_if(melanolib::string::ascii::is_space);

    auth.auth_string = stream.get_until(melanolib::string::ascii::is_space);

    if ( !header_parameters(stream, auth.parameters) );
        return false;

    auth.realm = auth.parameters.get("realm");
    auth.parameters.erase("realm");

    if ( auth.auth_scheme == "Basic" )
    {
        stream.str(Base64().decode(auth.auth_string));
        auth.user = stream.get_line(':');
        auth.password = stream.get_remaining();
    }

    return true;
}

} // namespace httpony
