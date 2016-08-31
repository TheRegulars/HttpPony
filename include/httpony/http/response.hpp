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

#ifndef HTTPONY_RESPONSE_HPP
#define HTTPONY_RESPONSE_HPP

#include "httpony/http/cookie.hpp"
#include "httpony/http/request.hpp"
#include "status.hpp"

namespace httpony {

struct AuthChallenge
{
    std::string auth_scheme;
    std::string realm;
    Headers     parameters;
};

/**
 * \brief Response data
 */
struct Response
{
    explicit Response(
        std::string content_type,
        Status status = Status(),
        const Protocol& protocol = Protocol::http_1_1
    )
        : body(io::ContentStream::OpenMode::Output),
          status(std::move(status)),
          protocol(protocol)
    {
        body.start_output(content_type);
    }

    explicit Response(const Protocol& protocol)
        : Response(Status(), protocol)
    {}

    Response(Status status = Status(), const Protocol& protocol = Protocol::http_1_1)
        : status(std::move(status)),
          protocol(protocol)
    {}

    static Response redirect(const Uri& location, Status status = StatusCode::Found)
    {
        Response response(status);
        response.headers["Location"] = location.full();
        return response;
    }

    static Response authorization_required(std::vector<AuthChallenge> challenges)
    {
        Response response(StatusCode::Unauthorized);
        response.www_authenticate = std::move(challenges);
        return response;
    }

    /**
     * \brief Removes the response body when required by HTTP
     */
    void clean_body()
    {
        if ( body.has_data() )
        {
            if ( status.type() == StatusType::Informational ||
                status == StatusCode::NoContent ||
                status == StatusCode::NotModified
            )
            {
                body.stop_output();
            }
        }
    }

    /**
     * \brief Removes the response body when required by HTTP
     */
    void clean_body(const Request& input)
    {
        clean_body();
        if ( body.has_data() )
        {
            if ( status == StatusCode::OK && input.method == "CONNECT" )
            {
                body.stop_output();
            }
            else if ( input.method == "HEAD" )
            {
                headers["Content-Type"] = body.content_type().string();
                if ( headers.contains("Transfer-Encoding") )
                    headers.erase("Transfer-Encoding");
                else
                    headers["Content-Length"] = std::to_string(body.content_length());
                body.stop_output();
            }
        }
    }

    void clear_data()
    {
        body = io::ContentStream();
        status = Status();
        headers.clear();
        protocol = Protocol::http_1_1;
        cookies.clear();
        date = {};
        www_authenticate.clear();
        proxy_authenticate.clear();
    }


    io::ContentStream body;
    Status      status;
    Headers     headers;
    Protocol    protocol;
    CookieJar   cookies;
    melanolib::time::DateTime date;
    std::vector<AuthChallenge> www_authenticate;
    std::vector<AuthChallenge> proxy_authenticate;

    io::Connection connection;
};

} // namespace httpony
#endif // HTTPONY_RESPONSE_HPP
