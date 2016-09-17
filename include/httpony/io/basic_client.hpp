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
#ifndef HTTPONY_IO_BASIC_CLIENT_HPP
#define HTTPONY_IO_BASIC_CLIENT_HPP

#include "httpony/io/connection.hpp"
#include "httpony/uri.hpp"

namespace httpony {
namespace io {

class BasicClient
{
public:

    OperationStatus connect(const Uri& target, io::Connection& connection) const
    {
        boost_tcp::resolver::query query = make_query(target, connection);

        OperationStatus status;
        auto endpoint_iterator = connection.socket().resolve(query, status);

        if ( status.error() )
            return status;

        if ( endpoint_iterator ==  boost_tcp::resolver::iterator{} )
            return "Could not resolve " + target.authority.full();

        return connection.socket().connect(endpoint_iterator);
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

    template<class OnConnect, class OnError>
    void async_connect(const Uri& target, io::Connection& connection, const OnConnect& on_connect, const OnError& on_error)
    {
        boost_tcp::resolver::query query = make_query(target, connection);
        connection.socket().async_connect(
            query,
            [on_error, on_connect](
                const OperationStatus& status,
                const boost_tcp::resolver::iterator&)
            {
                if ( status )
                    on_connect();
                else
                    on_error(status);
            }
        );
    }

private:
    boost_tcp::resolver::query make_query(const Uri& target, io::Connection& connection) const
    {
        if ( _timeout )
            connection.socket().set_timeout(*_timeout);

        std::string service = target.scheme;
        if ( target.authority.port )
            service = std::to_string(*target.authority.port);

        return boost_tcp::resolver::query(target.authority.host, service);
    }


    melanolib::Optional<melanolib::time::seconds> _timeout;
};

} // namespace io
} // namespace httpony
#endif // HTTPONY_IO_BASIC_CLIENT_HPP
