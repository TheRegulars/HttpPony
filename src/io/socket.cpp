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
#include "httpony/io/socket.hpp"

namespace httpony {
namespace io {


OperationStatus TimeoutSocket::connect(boost_tcp::resolver::iterator endpoint_iterator)
{
    boost::system::error_code error = boost::asio::error::would_block;

    boost::asio::async_connect(raw_socket(), endpoint_iterator,
        [this, &error](
            const boost::system::error_code& error_code,
            boost_tcp::resolver::iterator endpoint_iterator
        )
        {
            error = error_code;
        }
    );

    io_loop(&error);

    return error_to_status(error);
}


boost_tcp::resolver::iterator TimeoutSocket::resolve(
    const boost_tcp::resolver::query& query,
    OperationStatus& status)
{
    boost::system::error_code error = boost::asio::error::would_block;
    boost_tcp::resolver::iterator result;
    resolver.async_resolve(
        query,
        [&error, &result](
            const boost::system::error_code& error_code,
            const boost_tcp::resolver::iterator& iterator
        )
        {
            error = error_code;
            result = iterator;
        }
    );

    io_loop(&error);

    status = error_to_status(error);
    return result;
}

OperationStatus TimeoutSocket::process_async()
{
    boost::system::error_code error;
    _io_service.run_one(error);
    return error_to_status(error);
}

void TimeoutSocket::io_loop(boost::system::error_code* error)
{
    do
        _io_service.run_one();
    while ( !_io_service.stopped() && *error == boost::asio::error::would_block );
}

void TimeoutSocket::check_deadline()
{
    if ( timed_out() )
    {
        _deadline.expires_at(boost::posix_time::pos_infin);
        _io_service.stop();
    }

    _deadline.async_wait([this](const boost::system::error_code&){check_deadline();});
}

} // namespace io
} // namespace httpony
