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

#ifndef HTTPONY_HTTP_WRITE_HPP
#define HTTPONY_HTTP_WRITE_HPP

#include "httpony/http/response.hpp"
#include "httpony/multipart.hpp"

namespace httpony {
namespace http {

class Formatter
{
public:
    virtual ~Formatter(){}

    virtual void response(std::ostream& stream, Response& response) const = 0;
    virtual void request(std::ostream& stream, Request& request) const = 0;
    virtual void headers(std::ostream& stream, const Headers& headers) const = 0;
    virtual void auth_challenge(std::ostream& stream, const AuthChallenge& challenge) const = 0;
    virtual void multipart(std::ostream& stream, const Multipart& multipart) const = 0;

    /**
     * \brief Converts a compound header into a string
     */
    virtual std::string compound_header(const CompoundHeader& header) const = 0;
};

/**
 * \brief Formats HTTP objects as of the HTTP/1.x standards
 */
class Http1Formatter : public Formatter
{
public:
    explicit Http1Formatter(std::string line_ending="\r\n")
        : endl(std::move(line_ending))
    {}

    /**
     * \brief Writes the entire response to the stream
     * \note \p response is passed by non-const reference because
     *       writing the body could modify the state underlying buffer
     */
    void response(std::ostream& stream, Response& response) const override
    {
        response_line(stream, response);
        response_headers(stream, response);
        response.body.write_to(stream);
    }

    void request(std::ostream& stream, Request& request) const override
    {
        request_line(stream, request);
        request_headers(stream, request);
        request.body.write_to(stream);
    }

    /**
     * \brief Writes out a block of headers
     * \note This does not output a blank line at the end
     */
    void headers(std::ostream& stream, const Headers& headers) const override
    {
        for ( const auto& header_obj : headers )
            header(stream, header_obj.first, header_obj.second);
    }

    void auth_challenge(std::ostream& stream, const AuthChallenge& challenge) const override
    {
        stream << challenge.auth_scheme;

        if ( !challenge.realm.empty() )
            stream << " realm=\""
                   << melanolib::string::add_slashes(challenge.realm, "\"\\")
                   << "\";";

        if ( !challenge.parameters.empty() )
        {
            stream << ' ';
            header_parameters(stream, challenge.parameters);
        }
    }

    std::string compound_header(const CompoundHeader& header) const override
    {
        return compound_header(header, "; ");
    }

    std::string compound_header(const CompoundHeader& header, const std::string& delimiter) const
    {
        std::ostringstream result;
        result << header.value;
        if ( !header.parameters.empty() )
        {
            result << delimiter;
            header_parameters(result, header.parameters, delimiter);
        }

        return result.str();
    }

    void multipart(std::ostream& stream, const Multipart& multipart) const override
    {
        for ( const auto& part : multipart.parts )
        {
            stream << endl << "--" << multipart.boundary << endl;
            headers(stream, part.headers);
            stream << endl;
            stream.write(part.content.data(), part.content.size());
        }

        stream << endl << "--" << multipart.boundary << "--" << endl;
    }

private:

    /**
     * \brief Writes a response line
     */
    void response_line(std::ostream& stream, const Response& response) const
    {
        stream << response.protocol << ' '
               << response.status.code << ' '
               << response.status.message << endl;
    }

    void request_line(std::ostream& stream, Request& request) const
    {
        stream << request.method << ' ' << request.uri.path.url_encoded(true)
               << request.uri.query_string(true) << ' ' << request.protocol << endl;
    }


    template<class OrderedContainer>
        void header_parameters(std::ostream& stream, const OrderedContainer& input, const std::string& delimiter = ", ") const
    {
        if ( input.empty() )
            return;
        
        header_parameter(stream, input.front().first, input.front().second);

        for ( auto it = input.begin() + 1; it != input.end(); ++it )
        {
            stream << delimiter;
            header_parameter(stream, it->first, it->second);
        }
    }

    /**
     * \brief Writes a header line
     */
    template<class String, class Value>
        void header(std::ostream& stream, String&& name, Value&& value) const
    {
        stream << std::forward<String>(name) << ": " << std::forward<Value>(value) << endl;
    }

    void authenticate_header(std::ostream& stream, const std::string& name, const std::vector<AuthChallenge>& challenges) const
    {
        if ( challenges.empty() )
            return;

        stream << name << ": ";

        /// \todo merge this loop with header_parameters
        auth_challenge(stream, challenges.front());

        for ( auto it = challenges.begin() + 1; it != challenges.end(); ++it )
        {
            stream << ", ";
            auth_challenge(stream, *it);
        }

        stream << endl;
    }

    void header_parameter(std::ostream& stream, const std::string& name, const std::string& value) const
    {
        using namespace melanolib::string;
        static const char* const slashable = "\" \t\\";
        stream << name <<  '=';
        if ( contains_any(value, slashable) )
        {
            stream << '"' << add_slashes(value, slashable) << '"';
        }
        else
        {
            stream << value;
        }
    }

    /**
     * \brief Writes all response headers, including the blank line at the end
     */
    void response_headers(std::ostream& stream, const Response& response) const
    {
        if ( !response.headers.contains("Date") )
            header(stream, "Date", melanolib::time::strftime(response.date, "%r GMT"));

        headers(stream, response.headers);

        if ( !response.cookies.empty() && !response.headers.contains("Set-Cookie") )
            for ( const auto& cookie : response.cookies )
                header(stream, "Set-Cookie", cookie);

        if ( !response.headers.contains("WWW-Authenticate") )
            authenticate_header(stream, "WWW-Authenticate", response.www_authenticate);

        if ( !response.headers.contains("Proxy-Authenticate") )
            authenticate_header(stream, "Proxy-Authenticate", response.proxy_authenticate);

        if ( response.body.has_data() )
        {
            if ( !response.headers.contains("Content-Type") )
                header(stream, "Content-Type", response.body.content_type());
            
            if ( !response.headers.contains("Content-Length") )
                header(stream, "Content-Length", response.body.content_length());
        }

        stream << endl;
    }

    /**
     * \brief Writes all request headers, including the blank line at the end
     */
    void request_headers(std::ostream& stream, const Request& request) const
    {
        headers(stream, request.headers);

        if ( !request.headers.contains("Host") )
            header(stream, "Host", request.uri.authority.host);

        if ( !request.user_agent.empty() && !request.headers.contains("User-Agent") )
            header(stream, "User-Agent", request.user_agent);

        if ( !request.cookies.empty() && !request.headers.contains("Cookie") )
        {
            stream << "Cookie" << ": ";
            header_parameters(stream, request.cookies, "; ");
            stream << endl;
        }

        /// \todo authentication

        if ( request.body.has_data() )
        {
            if ( !request.headers.contains("Content-Type") )
                header(stream, "Content-Type", request.body.content_type());

            if ( !request.headers.contains("Content-Length") )
                header(stream, "Content-Length", request.body.content_length());
        }

        stream << endl;
    }

    std::string endl;
};

} // namespace http
} // namespace httpony
#endif // HTTPONY_HTTP_WRITE_HPP
