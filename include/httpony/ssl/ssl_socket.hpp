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

enum class VerifyMode
{
    Disabled = boost_ssl::verify_none,
    Loose = boost_ssl::verify_peer,
    Strict = boost_ssl::verify_peer|boost_ssl::verify_fail_if_no_peer_cert,
};

class SslSocket : public io::SocketWrapper
{
public:

    using ssl_socket_type = boost_ssl::stream<raw_socket_type>;

    explicit SslSocket(
        boost::asio::io_service& io_service,
        boost_ssl::context& context
    )
        : socket(io_service, context)
    {}

    OperationStatus close(bool graceful = true) override
    {
        boost::system::error_code error;
        if ( !graceful )
        {
            raw_socket().cancel(error);
            socket.async_shutdown([](const boost::system::error_code&){});
            socket.get_io_service().run_one(error);
        }
        else
        {
            socket.shutdown(error);
        }
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

    httpony::OperationStatus set_verify_mode(VerifyMode verify)
    {
        boost::system::error_code error;
        socket.set_verify_mode(boost_ssl::verify_mode(verify), error);
        return io::error_to_status(error);
    }

    /**
     * \brief Retrieves the common name of the verified certificate (if any)
     */
    httpony::OperationStatus get_cert_common_name(std::string& output) const
    {
        // not going to modify anything so it should be const
        SSL* native = const_cast<ssl_socket_type&>(socket).native_handle();
        if ( X509* cert = SSL_get_peer_certificate(native) )
        {
            if ( auto name = X509_get_subject_name(cert) )
            {
                int index = X509_NAME_get_index_by_NID(name, NID_commonName, -1);
                if ( index >= 0 )
                {
                    if ( auto entry = X509_NAME_get_entry(name, index) )
                    {
                        if ( auto data = X509_NAME_ENTRY_get_data(entry) )
                        {
                            unsigned char* out_buffer = nullptr;
                            auto size = ASN1_STRING_to_UTF8(&out_buffer, data);

                            if ( size >= 0 )
                                output = std::string((char*)out_buffer, size);

                            if ( out_buffer )
                                free(out_buffer);

                            if ( size >= 0 )
                                return {};
                        }
                    }
                }
            }
            return "Error retrieving certificate name";
        }
        return "No SSL certificate";
    }

    std::string get_cert_common_name() const
    {
        std::string output;
        get_cert_common_name(output);
        return output;
    }

private:
    ssl_socket_type socket;
};

} // namespace ssl
} // namespace httpony
#endif // HTTPONY_SSL_SOCKET_HPP
