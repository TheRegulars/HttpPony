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

#ifndef HTTPONY_REQUEST_HPP
#define HTTPONY_REQUEST_HPP

#include "httpony/http/protocol.hpp"
#include "httpony/io/network_stream.hpp"
#include "httpony/io/connection.hpp"
#include "httpony/uri.hpp"
#include "httpony/http/user_agent.hpp"

namespace httpony {

/**
 * \brief HTTP Authentication credentials
 */
struct Auth
{
    Auth(){}

    std::string user;
    std::string password;
    std::string auth_scheme;
    std::string auth_string;
    std::string realm;
    Headers     parameters;
};


/**
 * \brief HTTP request data
 */
struct Request
{
    Request() = default;

    Request(std::string method, const Uri& uri, Protocol protocol = Protocol::http_1_1)
        : method(std::move(method)),
          uri(std::move(uri)),
          protocol(std::move(protocol))
    {}

    bool can_parse_post() const;
    bool parse_post();

    bool format_post();

    void clear_data()
    {
        method = {};
        uri = {};
        protocol = Protocol::http_1_1;
        headers.clear();
        cookies.clear();
        get.clear();
        post.clear();
        auth = {};
        body = {};
    }

    std::string method;
    Uri         uri;
    Protocol    protocol = Protocol::http_1_1;
    Headers     headers;
    DataMap     cookies;
    DataMap     get;
    DataMap     post;
    Auth        auth;
    UserAgent   user_agent;

    io::ContentStream body;

    melanolib::time::DateTime received_date;

    io::Connection connection;
};

} // namespace httpony
#endif // HTTPONY_REQUEST_HPP
