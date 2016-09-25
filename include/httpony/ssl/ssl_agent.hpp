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
#ifndef HTTPONY_SSL_AGENT_HPP
#define HTTPONY_SSL_AGENT_HPP

#include "httpony/ssl/ssl_socket.hpp"
#include "httpony/io/connection.hpp"

namespace httpony {
namespace ssl {

class SslAgent
{
public:
    SslAgent()
        : context(boost_ssl::context::sslv23)
    {}

    /**
     * \brief Toggles certificate verification (disabled by default)
     * \see load_cert_authority() and load_default_authorities()
     */
    void set_verify_mode(bool verify)
    {
        this->verify = verify;
    }

    /**
     * \brief Whether the agent shall perform certificate verification
     */
    bool verify_mode() const
    {
        return verify;
    }

    /**
     * \brief Load a certificate authority file (Which must be in PEM format).
     *
     * This by itself only loads the file, you also need to call
     * set_verify_mode() to enable verification.
     * \see load_default_authorities() and set_verify_mode()
     */
    OperationStatus load_cert_authority(const std::string& file_name)
    {
        boost::system::error_code error;
        context.load_verify_file(file_name, error);
        return io::error_to_status(error);
    }
    /**
     * \brief Loads the default authority file).
     *
     * This by itself only loads the files, you also need to call
     * set_verify_mode() to enable verification.
     * \see load_cert_authority() and set_verify_mode()
     */
    OperationStatus load_default_authorities()
    {
        boost::system::error_code error;
        context.set_default_verify_paths(error);
        return io::error_to_status(error);
    }

    OperationStatus set_certificate(
        const std::string& cert_file,
        const std::string& key_file,
        const std::string& dh_file = {},
        const std::string& password_reading = {},
        const std::string& password_writing = {}
    )
    {
        boost::system::error_code error;
        context.set_password_callback([password_reading, password_writing]
            (std::size_t max_length,
             boost_ssl::context::password_purpose purpose) {
                return purpose == boost::asio::ssl::context_base::for_reading ?
                    password_reading :
                    password_writing;
        }, error);
        if ( !error )
            context.use_certificate_chain_file(cert_file, error);
        if ( !error )
            context.use_private_key_file(key_file, boost::asio::ssl::context::pem, error);
        if ( !error && !dh_file.empty() )
            context.use_tmp_dh_file(dh_file, error);
        return io::error_to_status(error);
    }

    httpony::OperationStatus set_session_id_context(const std::string& id)
    {

        if ( !SSL_CTX_set_session_id_context(context.native_handle(),
            (const unsigned char*)id.data(), id.size()) )
        {
            return "Session ID too long";
        }
        return {};
    }

protected:
    /**
     * \brief Creates a connection linked to a SSL socket
     */
    io::Connection create_connection(bool ssl)
    {
        if ( ssl )
            return io::Connection(io::SocketTag<SslSocket>{}, context);
        return io::Connection(io::SocketTag<io::PlainSocket>{});
    }

    OperationStatus handshake(io::TimeoutSocket& in, bool client)
    {
        if ( SslSocket* socket = socket_cast(in) )
        {
            /// \todo Async handshake
            if ( auto status = socket->set_verify_mode(verify) )
                return socket->handshake(client);
            else
                return status;
        }
        return "Not an SSL connection";
    }

    static SslSocket* socket_cast(io::TimeoutSocket& in)
    {
        return dynamic_cast<SslSocket*>(&in.socket_wrapper());
    }

private:
    boost_ssl::context context;
    bool verify = false;
};

} // namespace ssl
} // namespace httpony
#endif // HTTPONY_SSL_AGENT_HPP
