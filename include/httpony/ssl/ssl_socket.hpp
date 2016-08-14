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
#ifndef HTTPONY_SSL_SOCKET_HPP
#define HTTPONY_SSL_SOCKET_HPP

#include <boost/asio/ssl.hpp>

#include "httpony/io/socket.hpp"

namespace httpony {
namespace ssl {

namespace boost_ssl = boost::asio::ssl;


class SslSocket : public io::SocketWrapper
{
public:
    using ssl_socket_type = boost_ssl::stream<raw_socket_type>;

    explicit SslSocket(
        boost::asio::io_service& io_service,
        boost_ssl::context& context
    )
        : socket(io_service, context)
    {
    }

    OperationStatus close() override
    {
        boost::system::error_code error;
        raw_socket().close(error);
        return io::error_to_status(error);
    }

    raw_socket_type& raw_socket() override
    {
        return socket.next_layer();
    }

    const raw_socket_type& raw_socket() const override
    {
        return socket.next_layer();
    }

    ssl_socket_type& ssl_socket()
    {
        return socket;
    }

    void async_read_some(boost::asio::mutable_buffers_1& buffer, const AsyncCallback& callback) override
    {
        socket.async_read_some(buffer, callback);
    }

    void async_write(boost::asio::const_buffers_1& buffer, const AsyncCallback& callback) override
    {
        boost::asio::async_write(socket, buffer, callback);
    }

    httpony::OperationStatus handshake(bool client)
    {
        boost::system::error_code error;
        socket.handshake(
            client ? boost_ssl::stream_base::client : boost_ssl::stream_base::server,
            error
        );
        return io::error_to_status(error);
    }

private:
    ssl_socket_type socket;

};

} // namespace ssl
} // namespace httpony
#endif // HTTPONY_SSL_SOCKET_HPP
