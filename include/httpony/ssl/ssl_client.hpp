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
#ifndef HTTPONY_SSL_CLIENT_HPP
#define HTTPONY_SSL_CLIENT_HPP

#include "httpony/http/agent/client.hpp"
#include "httpony/ssl/ssl_agent.hpp"

namespace httpony {
namespace ssl {

class SslClient : public Client, public SslAgent
{
public:
    template<class... Args>
        SslClient(Args&&... args)
            : Client(std::forward<Args>(args)...)
        {}

protected:
    io::Connection create_connection(const Uri& target) override
    {
        return SslAgent::create_connection(target.scheme == "https");
    }

    OperationStatus on_connect(const Uri& target, io::Connection& connection) override
    {
        if ( target.scheme == "https" )
            return handshake(connection.socket(), true);
        return {};
    }
};


} // namespace ssl
} // namespace httpony
#endif // HTTPONY_SSL_CLIENT_HPP
