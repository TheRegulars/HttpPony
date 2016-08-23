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
#ifndef HTTPONY_IO_LISTEN_SERVER_HPP
#define HTTPONY_IO_LISTEN_SERVER_HPP

/// \cond
#include <list>
#include <stdexcept>
/// \endcond

#include "httpony/io/connection.hpp"

namespace httpony {
namespace io {

/**
 * \brief Address a server shall listen on
 */
struct ListenAddress : public IPAddress
{
    ListenAddress(const IPAddress& ip)
        : IPAddress(ip)
    {}

    ListenAddress(Type type, std::string string, uint16_t port)
        : IPAddress(type, std::move(string), port)
    {}

    ListenAddress(uint16_t port = 0)
        : IPAddress(Type::IPv6, "", port)
    {}

    ListenAddress(Type type, uint16_t port)
        : IPAddress(type, "", port)
    {}
};

/**
 * \brief Low-level server object that listen to a port and
 * calls functor on incoming connections
 */
class BasicServer
{
public:
    /**
     * \brief Sets up the server to listen on the given address
     * \note You'll still need to call run() for the server to actually accept
     *       incoming connections
     */
    ListenAddress start(const ListenAddress& listen)
    {
        boost_tcp protocol = listen.type == IPAddress::Type::IPv4 ? boost_tcp::v4() : boost_tcp::v6();

        boost_tcp::endpoint endpoint;
        if ( !listen.string.empty() )
        {
            boost_tcp::resolver resolver(io_service);
            endpoint = *resolver.resolve({
                protocol,
                listen.string,
                std::to_string(listen.port)
            });
        }
        else
        {
            endpoint = boost_tcp::endpoint(protocol, listen.port);
        }

        acceptor.open(endpoint.protocol());
        acceptor.set_option(boost_tcp::acceptor::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.listen();

        if ( io_service.stopped() )
            io_service.reset();

        return ListenAddress(SocketWrapper::endpoint_to_ip(acceptor.local_endpoint()));
    }

    /**
     * \brief Stops the server
     *
     * If run() is running, it will return
     */
    void stop()
    {
        /// \todo Needs locking?
        acceptor.close();
        io_service.stop();
    }

    /**
     * \brief Runs the server synchronously
     * \tparam OnSuccess Functor accepting a io::Connectio&
     * \tparam OnFailure Functor accepting a io::Connectio& and a std::string
     * \tparam CreateConnection Function returning an io::Connection
     * \param on_success Functor called on a successful connection
     * \param on_failure Functor called on a connection that had some issues
     * \param create_connection Function called to create a new connection object
     */
    template<class OnSuccess, class OnFailure, class CreateConnection>
        void run(const OnSuccess& on_success, const OnFailure& on_failure,
                 const CreateConnection& create_connection)
    {
        accept(on_success, on_failure, create_connection);
        io_service.run();
    }

    bool running() const
    {
        return !io_service.stopped();
    }

    /**
     * \brief Remove timeouts, connections will block indefinitely
     * \see set_timeout(), timeout()
     */
    void clear_timeout()
    {
        _timeout = melanolib::Optional<melanolib::time::seconds>();
    }

    /**
     * \brief Sets the connection timeout
     * \see clear_timeout(), timeout()
     */
    void set_timeout(melanolib::time::seconds timeout)
    {
        _timeout = timeout;
    }

    /**
     * \brief The timeout for network I/O operations
     * \see set_timeout(), clear_timeout()
     */
    melanolib::Optional<melanolib::time::seconds> timeout() const
    {
        return _timeout;
    }

private:
    /**
     * \brief Schedules an async connection
     */
    template<class OnSuccess, class OnFailure, class CreateConnection>
        void accept(const OnSuccess& on_success, const OnFailure& on_failure,
                 const CreateConnection& create_connection)
        {
            connections.push_back(create_connection());
            auto connection = connections.back();
            acceptor.async_accept(
                connection.socket().raw_socket(),
                [this, connection, on_success, on_failure, create_connection]
                (boost::system::error_code error) mutable
                {
                    if ( !acceptor.is_open() )
                        return;

                    auto conn_iter = std::find(
                        connections.begin(), connections.end(),
                        connection
                    );

                    // Should never be == end
                    if ( conn_iter == connections.end() )
                        throw std::logic_error("Connection vanished");

                    if ( _timeout )
                        connection.socket().set_timeout(*_timeout);

                    if ( !error )
                        on_success(*conn_iter);
                    else
                        on_failure(*conn_iter, error_to_status(error));

                    connections.erase(conn_iter);
                    accept(on_success, on_failure, create_connection);
                }
            );
        }

    melanolib::Optional<melanolib::time::seconds> _timeout;
    boost::asio::io_service                       io_service;
    boost_tcp::acceptor                           acceptor{io_service};
    std::list<io::Connection>                     connections;
};

} // namespace io
} // namespace httpony
#endif // HTTPONY_IO_LISTEN_SERVER_HPP
