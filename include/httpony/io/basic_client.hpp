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

#include "connection.hpp"
#include "httpony/http/response.hpp"

namespace httpony {
namespace io {

class BasicClient
{
public:

    /**
     * \todo Async connect
     */
    ClientStatus connect(const Uri& target, io::Connection& connection) const
    {
        if ( _timeout )
            connection.socket().set_timeout(*_timeout);

        std::string service = target.scheme;
        if ( target.authority.port )
            service = std::to_string(*target.authority.port);
        boost_tcp::resolver::query query(target.authority.host, service);

        boost::system::error_code error_code;
        auto endpoint_iterator = connection.socket().resolve(query, error_code);

        if ( error_code )
            return error_code.message();

        connection.socket().connect(endpoint_iterator, error_code);
        if ( error_code )
            return error_code.message();

        return {};
    }

    /**
     * \brief Remove timeouts, connections will block indefinitely
     * \see set_timeout(), timeout()
     */
    void clear_timeout()
    {
        _timeout = {};
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
    melanolib::Optional<melanolib::time::seconds> _timeout;
};

} // namespace io
} // namespace httpony
#endif // HTTPONY_IO_BASIC_CLIENT_HPP
