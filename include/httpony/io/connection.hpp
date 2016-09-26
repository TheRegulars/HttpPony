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
#ifndef HTTPONY_CONNECTION_HPP
#define HTTPONY_CONNECTION_HPP

/// \cond
#include <iostream>
/// \endcond

#include "httpony/io/buffer.hpp"

namespace httpony {
namespace io {

class Connection
{
public:
    class SendStream;
    class ReceiveStream;

    template<class Tag, class... SocketArgs>
        explicit Connection(SocketTag<Tag> st, SocketArgs&&... args)
            : data(std::make_shared<Data>(st, std::forward<SocketArgs>(args)...))
    {}

    Connection() = default;


    NetworkInputBuffer& input_buffer()
    {
        return data->input_buffer;
    }

    NetworkOutputBuffer& output_buffer()
    {
        return data->output_buffer;
    }

    OperationStatus commit_output()
    {
        if ( data->output_buffer.size() == 0 )
            return {};
        OperationStatus status;
        data->socket.write(data->output_buffer.data(), status);
        data->output_buffer.consume(data->output_buffer.size());
        return status;
    }

    void close()
    {
        data->socket.close();
    }

    IPAddress remote_address() const
    {
        return data->socket.remote_address();
    }

    IPAddress local_address() const
    {
        return data->socket.local_address();
    }

    bool connected() const
    {
        return data->socket.is_open();
    }

    SendStream send_stream();

    ReceiveStream receive_stream();

    TimeoutSocket& socket()
    {
        return data->socket;
    }

    const TimeoutSocket& socket() const
    {
        return data->socket;
    }

    explicit operator bool() const
    {
        return !!data;
    }

    bool operator==(const io::Connection& oth) const
    {
        return data == oth.data;
    }

    bool operator!=(const io::Connection& oth) const
    {
        return data != oth.data;
    }

private:
    struct Data
    {
        template<class... SocketArgs>
            explicit Data(SocketArgs&&... args)
                : socket(std::forward<SocketArgs>(args)...)
        {}

        TimeoutSocket       socket;
        NetworkInputBuffer  input_buffer{socket};
        NetworkOutputBuffer output_buffer;
    };

    std::shared_ptr<Data> data;
};

/**
 * \brief Stream used to send data through the connection
 * \note There should be only one send stream per connection at a given time
 * \todo Better async streaming
 */
class Connection::SendStream : public std::ostream
{
public:
    SendStream(SendStream&& oth)
        : std::ostream(oth.rdbuf()),
            connection(oth.connection)
    {
        oth.unset();
    }

    SendStream& operator=(SendStream&& oth)
    {
        rdbuf(oth.rdbuf());
        connection = std::move(oth.connection);
        oth.unset();
        return *this;
    }

    ~SendStream()
    {
        send();
    }

    /**
        * \brief Ensures all data is being sent,
        * \returns \b true on success
        * \note If not called, the data will still be sent but you might not be
        * able to detect whether that has been successful
        */
    OperationStatus send()
    {
        OperationStatus status;
        if ( !connection )
            status = "invalid connection";
        else
            status = connection.commit_output();
        if ( status.error() )
            setstate(badbit);
        return status;
    }

private:
    void unset()
    {
        rdbuf(nullptr);
    }

    SendStream(Connection connection)
        : std::ostream(&connection.output_buffer()),
            connection(std::move(connection))
    {}

    friend Connection;
    Connection connection;
};

class Connection::ReceiveStream : public std::istream
{
public:
    ReceiveStream(ReceiveStream&& oth)
        : std::istream(oth.rdbuf()),
            connection(std::move(oth.connection))
    {
        oth.unset();
    }

    ReceiveStream& operator=(ReceiveStream&& oth)
    {
        rdbuf(oth.rdbuf());
        connection = std::move(oth.connection);
        oth.unset();
        return *this;
    }

    bool timed_out() const
    {
        return connection.data->socket.timed_out();
    }

private:
    void unset()
    {
        rdbuf(nullptr);
    }

    ReceiveStream(Connection connection)
        : std::istream(&connection.input_buffer()),
            connection(std::move(connection))
    {}

    friend Connection;
    Connection connection;
};


inline Connection::SendStream Connection::send_stream()
{
    return SendStream(*this);
}

inline Connection::ReceiveStream Connection::receive_stream()
{
    return ReceiveStream(*this);
}

} // namespace io
} // namespace httpony
#endif // HTTPONY_CONNECTION_HPP
