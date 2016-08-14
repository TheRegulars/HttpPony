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
#include "httpony/ssl/ssl_socket.hpp"

namespace httpony {
namespace ssl {

class SslClient : public Client
{
public:
    template<class... Args>
        SslClient(Args&&... args)
            : Client(std::forward<Args>(args)...),
              context(boost_ssl::context::sslv23)
        {}

protected:
    io::Connection create_connection(const Uri& target) final
    {
        if ( target.scheme == "https" )
            return io::Connection(io::SocketTag<SslSocket>{}, context);
        return io::Connection(io::SocketTag<io::PlainSocket>{});
    }

    OperationStatus on_connect(const Uri& target, io::Connection& connection) override
    {
        if ( target.scheme == "https" )
        {
            /// \todo dynamic_cast? create_connection() might be best to not be final here
            SslSocket& socket = static_cast<SslSocket&>(connection.socket().socket_wrapper());
            /// \todo Async handshake
            return socket.handshake(true);
        }

        return {};
    }

    boost_ssl::context context;
};


} // namespace ssl
} // namespace httpony
#endif // HTTPONY_SSL_CLIENT_HPP
