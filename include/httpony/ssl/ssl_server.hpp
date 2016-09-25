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
#ifndef HTTPONY_SSL_SERVER_HPP
#define HTTPONY_SSL_SERVER_HPP

#include "httpony/ssl/ssl_agent.hpp"
#include "httpony/http/agent/server.hpp"

namespace httpony {
namespace ssl {

/**
 * \brief Base class for SSL-enabled servers
 */
class SslServer : public Server, public SslAgent
{
public:
    explicit SslServer(
        IPAddress listen,
        bool ssl_enabled = true
    )
        : Server(std::move(listen)),
          _ssl_enabled(ssl_enabled)
    {}

    bool ssl_enabled() const
    {
        return _ssl_enabled;
    }

    bool set_ssl_enabled(bool enabled)
    {
        if ( !running() )
            _ssl_enabled = enabled;
        return _ssl_enabled;
    }


private:
    io::Connection create_connection() override
    {
        return SslAgent::create_connection(_ssl_enabled);
    }

    /**
     * \brief Performs the SSL handshake
     */
    OperationStatus accept(io::Connection& connection) final
    {
        if ( !_ssl_enabled )
            return {};
        return handshake(connection.socket(), false);
    }

    bool _ssl_enabled = true;
};

} // namespace ssl
} // namespace httpony
#endif // HTTPONY_SSL_SERVER_HPP
