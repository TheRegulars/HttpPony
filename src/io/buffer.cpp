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
#include "httpony/io/buffer.hpp"

namespace httpony {
namespace io {

std::size_t NetworkInputBuffer::read_some(std::size_t size, OperationStatus& status)
{
    auto prev_size = this->size();
    if ( size <= prev_size )
        return size;
    size -= prev_size;

    auto in_buffer = prepare(size);

    auto read_size = _socket.read_some(in_buffer, status);

    _total_read_size += read_size;

    commit(read_size);

    return read_size + prev_size;
}

void NetworkInputBuffer::expect_input(std::size_t byte_count)
{
    if ( byte_count == unlimited_input() )
        expect_unlimited_input();
    else if ( byte_count > size() )
        _expected_input = byte_count - size();
    else
        _expected_input = 0;
}

NetworkInputBuffer::int_type NetworkInputBuffer::underflow()
{
    int_type ret = boost::asio::streambuf::underflow();
    if ( ret == traits_type::eof() && _expected_input > 0 )
    {
        auto request_size = _expected_input > chunk_size() ?
            chunk_size() : _expected_input;

        auto read_size = read_some(request_size, _status);

        if ( _expected_input != unlimited_input() )
        {
            if ( read_size <= _expected_input )
                _expected_input -= read_size;
            else
                /// \todo This should trigger a bad request
                _status = "unexpected data in the stream";
        }

        ret = boost::asio::streambuf::underflow();
    }
    return ret;
}

} // namespace io
} // namespace httpony
