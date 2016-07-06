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

#include "http_parser.hpp"

namespace httpony {


Status read_request(std::istream& stream, Request& request, HttpParserFlags flags)
{
    request = Request();

    if ( !read_request_line(stream, request) )
        return StatusCode::BadRequest;

    if ( !read_headers(stream, request.headers,
            flags & ParseFoldedHeaders,
            flags & PreserveCookieHeaders ? nullptr : &request.cookies
        ) )
        return StatusCode::BadRequest;


    if ( request.headers.contains("Content-Length") )
    {
        if ( !request.body.get_data(stream.rdbuf(), request.headers) )
            return StatusCode::BadRequest;

        if ( request.protocol == Protocol("HTTP", 1, 1) && request.headers.get("Expect") == "100-continue" )
            return StatusCode::Continue;
    }
    else if ( !stream.eof() )
    {
        return StatusCode::LengthRequired;
    }

    if ( request.protocol == Protocol("HTTP", 1, 1) && request.headers.contains("Expect") )
        return StatusCode::ExpectationFailed;

    return StatusCode::OK;
}



void skip_line(std::istream& stream)
{
    while ( stream && stream.get() != '\n' );
}

bool read_request_line(std::istream& stream, Request& request)
{
    std::string uri;
    stream >> request.method >> uri >> request.protocol;
    request.url = uri;
    request.get = request.url.query;
    skip_line(stream);

    return request.protocol.valid() && stream;
}

bool read_headers(std::istream& stream, Headers& headers, bool parse_folded, DataMap* cookies)
{
    if ( cookies )
        cookies->clear();

    std::string name, value;
    while ( stream && stream.peek() != '\r' )
    {
        // (obsolete) header folding
        // it's compliant to either return 400 or parse them
        if ( std::isspace(stream.peek()) )
        {
            if ( !parse_folded )
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

        if ( !read_delimited(stream, name, ':') )
            return false;

        if ( cookies && name == "Cookie" )
        {
            if ( !read_cookies(stream, *cookies) )
                return false;
            continue;
        }

        if ( stream.peek() == '"' )
        {
            read_quoted_header_value(stream, value);
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

bool read_delimited(std::istream& stream, std::string& output, char delim, bool at_end)
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

bool skip_spaces(std::istream& stream, bool at_end )
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

bool read_quoted_header_value(std::istream& stream, std::string& value)
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

bool read_cookies(std::istream& stream, DataMap& cookies)
{
    do
    {
        std::string name;
        if ( !read_delimited(stream, name, '=') )
            return false;

        std::string value;
        if ( !read_delimited(stream, value, ';', true) )
            return false;

        cookies.append(name, value);
    }
    while ( stream.peek() != '\r' );

    skip_line(stream);

    return true;
}


} // namespace httpony