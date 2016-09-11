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
#ifndef HTTPONY_IO_BUFFER_HPP
#define HTTPONY_IO_BUFFER_HPP

/// \cond
#include <limits>
/// \endcond

#include "httpony/io/socket.hpp"

namespace httpony {
namespace io {


/**
 * \brief Stream buffer linked to a socket for reading
 */
class NetworkInputBuffer : public boost::asio::streambuf
{
public:
    explicit NetworkInputBuffer(TimeoutSocket& socket)
        : _socket(socket)
    {
    }

    /**
     * \brief Reads up to size from the socket
     */
    std::size_t read_some(std::size_t size, OperationStatus& status)
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

    /**
     * \brief Expect at least \p byte_count to be available in the socket.
     */
    void expect_input(std::size_t byte_count)
    {
        if ( byte_count == unlimited_input() )
            expect_unlimited_input();
        else if ( byte_count > size() )
            _expected_input = byte_count - size();
        else
            _expected_input = 0;
    }

    /**
     * \brief Expect an unspecified of bytes
     *
     * This will cause to perform chunked reads of unlimited_input() bytes
     * during underflows.
     *
     * Once the remote endpoint fails to deliver enough bytes,
     * it is handled as the end of the stream (ie: expect_input(0))
     */
    void expect_unlimited_input()
    {
        _expected_input = unlimited_input();
    }

    std::size_t expected_input() const
    {
        return _expected_input;
    }

    OperationStatus status() const
    {
        return _status;
    }

    bool error() const
    {
        return _status.error();
    }

    static constexpr std::size_t unlimited_input()
    {
        return std::numeric_limits<std::size_t>::max();
    }

    static constexpr std::size_t chunk_size()
    {
        return 1024;
    }

    /**
     * \brief Number of bytes read from the source from this buffer
     */
    std::size_t total_read_size() const
    {
        return _total_read_size;
    }

    /**
     * \brief Number of bytes expected to have been read once all of the
     * expected input has been read.
     */
    std::size_t total_expected_size() const
    {
        if ( _expected_input == unlimited_input() )
            return unlimited_input();
        return _total_read_size + _expected_input;
    }

protected:
    int_type underflow() override
    {
        int_type ret = boost::asio::streambuf::underflow();
        if ( ret == traits_type::eof() && _expected_input > 0 )
        {
            auto request_size = _expected_input == unlimited_input() ?
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

private:

    TimeoutSocket& _socket;
    std::size_t _expected_input = 0;
    OperationStatus _status;
    std::size_t _total_read_size = 0;
};

using NetworkOutputBuffer = boost::asio::streambuf;

} // namespace io
} // namespace httpony
#endif // HTTPONY_IO_BUFFER_HPP
