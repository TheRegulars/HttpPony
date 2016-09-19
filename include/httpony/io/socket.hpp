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
#ifndef HTTPONY_IO_SOCKET_HPP
#define HTTPONY_IO_SOCKET_HPP

#include <boost/asio.hpp>

/// \cond
#include <melanolib/time/date_time.hpp>
/// \endcond

#include "httpony/ip_address.hpp"
#include "httpony/util/operation_status.hpp"

namespace httpony {
namespace io {

using boost_tcp = boost::asio::ip::tcp;

inline OperationStatus error_to_status(const boost::system::error_code& error)
{
    if ( error == boost::asio::error::would_block )
        return "timeout";
    if ( error )
        return error.message();
    return {};
}

class SocketWrapper
{
public:
    using raw_socket_type = boost_tcp::socket;

    /**
     * \brief Functor for ASIO calls
     */
    class AsyncCallback
    {
    public:
        AsyncCallback(boost::system::error_code& error, std::size_t& bytes_transferred)
            : error(&error), bytes_transferred(&bytes_transferred)
        {
            *this->error = boost::asio::error::would_block;
            *this->bytes_transferred = 0;
        }

        void operator()(const boost::system::error_code& ec, std::size_t bt) const
        {
            *error = ec;
            *bytes_transferred += bt;
        }

    private:
        boost::system::error_code* error;
        std::size_t* bytes_transferred;
    };

    virtual ~SocketWrapper() {}

    /**
     * \brief Closes the socket, further IO calls will fail
     *
     * It shouldn't throw an exception on failure
     */
    virtual OperationStatus close() = 0;

    /**
     * \brief Returns the low-level socket object
     */
    virtual raw_socket_type& raw_socket() = 0;
    /**
     * \brief Returns the low-level socket object
     */
    virtual const raw_socket_type& raw_socket() const = 0;

    /**
     * \brief Async IO call to read from the socket into a buffer
     */
    virtual void async_read_some(boost::asio::mutable_buffers_1& buffer, const AsyncCallback& callback) = 0;

    /**
     * \brief Async IO call to write to the socket from a buffer
     */
    virtual void async_write(boost::asio::const_buffers_1& buffer, const AsyncCallback& callback) = 0;


    virtual bool is_open() const
    {
        return raw_socket().is_open();
    }

    virtual IPAddress remote_address() const
    {
        boost::system::error_code error;
        auto endpoint = raw_socket().remote_endpoint(error);
        if ( error )
            return {};
        return endpoint_to_ip(endpoint);
    }

    virtual IPAddress local_address() const
    {
        boost::system::error_code error;
        auto endpoint = raw_socket().local_endpoint(error);
        if ( error )
            return {};
        return endpoint_to_ip(endpoint);
    }

    /**
     * \brief Converts a boost endpoint to an IPAddress object
     */
    static IPAddress endpoint_to_ip(const boost_tcp::endpoint& endpoint)
    {
        return IPAddress(
            endpoint.address().is_v6() ? IPAddress::Type::IPv6 : IPAddress::Type::IPv4,
            endpoint.address().to_string(),
            endpoint.port()
        );
    }
};

class PlainSocket : public SocketWrapper
{
public:
    explicit PlainSocket(boost::asio::io_service& io_service)
        : socket(io_service)
    {}

    OperationStatus close() override
    {
        boost::system::error_code error;
        socket.close(error);
        return error_to_status(error);
    }

    raw_socket_type& raw_socket() override
    {
        return socket;
    }

    const raw_socket_type& raw_socket() const override
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

private:
    raw_socket_type socket;
};

template<class SocketType>
    struct SocketTag
{
    using type = SocketType;
};

/**
 * \brief A network socket with a timeout
 */
class TimeoutSocket
{
public:
    /**
     * \brief Creates a socket without setting a timeout
     */
    template<class SocketType, class... ExtraArgs>
        explicit TimeoutSocket(SocketTag<SocketType>, ExtraArgs&&... args)
            : _socket(std::make_unique<SocketType>(_io_service, std::forward<ExtraArgs>(args)...))
    {
        clear_timeout();
        check_deadline();
    }

    /**
     * \brief Closes the socket before destucting the object
     */
    ~TimeoutSocket()
    {
        close();
    }

    /**
     * \brief Closes the socket, further IO calls will fail
     */
    void close()
    {
        _socket->close();
    }

    /**
     * \brief Whether the socket timed out
     */
    bool timed_out() const
    {
        return _deadline.expires_at() <= boost::asio::deadline_timer::traits_type::now();
    }

    /**
     * \brief Returns the low-level socket object
     */
    boost_tcp::socket& raw_socket()
    {
        return _socket->raw_socket();
    }

    /**
     * \brief Returns the low-level socket object
     */
    const boost_tcp::socket& raw_socket() const
    {
        return _socket->raw_socket();
    }

    SocketWrapper& socket_wrapper()
    {
        return *_socket;
    }

    /**
     * \brief Sets the timeout
     *
     * timed_out() will be true this many seconds from this call
     * \see clear_timeout()
     */
    void set_timeout(melanolib::time::seconds timeout)
    {
        _deadline.expires_from_now(boost::posix_time::seconds(timeout.count()));
    }

    /**
     * \brief Removes the timeout, IO calls will block indefinitely after this
     * \see set_timeout()
     */
    void clear_timeout()
    {
        _deadline.expires_at(boost::posix_time::pos_infin);
    }

    /**
     * \brief Reads some data to fill the input buffer
     * \returns The number of bytes written to the destination
     */
    template<class MutableBufferSequence>
        std::size_t read_some(MutableBufferSequence&& buffer, OperationStatus& status)
    {
        return io_operation(&SocketWrapper::async_read_some, boost::asio::buffer(buffer), status);
    }

    /**
     * \brief writes all data from the given buffer
     * \returns The number of bytes read from the source
     */
    template<class ConstBufferSequence>
        std::size_t write(ConstBufferSequence&& buffer, OperationStatus& status)
    {
        return io_operation(&SocketWrapper::async_write, boost::asio::buffer(buffer), status);
    }

    OperationStatus connect(boost_tcp::resolver::iterator endpoint_iterator);

    boost_tcp::resolver::iterator resolve(
        const boost_tcp::resolver::query& query,
        OperationStatus& status);

    OperationStatus process_async();

    bool is_open() const
    {
        return _socket->is_open();
    }

    IPAddress remote_address() const
    {
        return _socket->remote_address();
    }

    IPAddress local_address() const
    {
        return _socket->local_address();
    }

    /**
     * \brief Queues an async connection
     * \tparam Callback A functor accepting an OperationStatus
     *                  and a boost_tcp::resolver::iterator
     * \note You must run process_asyncfor this to get handled
     */
    template<class Callback>
        void async_connect(
            boost_tcp::resolver::iterator endpoint_iterator,
            const Callback& callback)
    {
        boost::asio::async_connect(
            raw_socket(),
            endpoint_iterator,
            [this, callback](
                const boost::system::error_code& error,
                const boost_tcp::resolver::iterator& endpoint_iterator
            )
            {
                callback(error_to_status(error), endpoint_iterator);
            }
        );
    }

    /**
     * \brief Queues an async name resolution
     * \tparam Callback A functor accepting an OperationStatus
     *                  and a boost_tcp::resolver::iterator
     * \note You must run process_asyncfor this to get handled
     */
    template<class Callback>
        void async_resolve(
            const boost_tcp::resolver::query& query,
            const Callback& callback)
    {
        resolver.async_resolve(
            query,
            [this, callback](
                const boost::system::error_code& error,
                const boost_tcp::resolver::iterator& endpoint_iterator
            )
            {
                callback(error_to_status(error), endpoint_iterator);
            }
        );
    }

    /**
     * \brief Queues an async name resolution and connection
     * \tparam Callback A functor accepting an OperationStatus
     *                  and a boost_tcp::resolver::iterator
     * \note You must run process_asyncfor this to get handled
     */
    template<class Callback>
        void async_connect(
            const boost_tcp::resolver::query& query,
            const Callback& callback)
    {
        async_resolve(
            query,
            [this, callback](
                const OperationStatus& status,
                const boost_tcp::resolver::iterator& endpoint_iterator)
            {
                if ( status.error() )
                    callback(status, endpoint_iterator);
                else
                    async_connect(endpoint_iterator, callback);
            }
        );
    }

private:

    /**
     * \brief Calls a function for async IO operations, then runs the io service until completion or timeout
     */
    template<class Buffer>
    std::size_t io_operation(
        void (SocketWrapper::*func)(Buffer&, const SocketWrapper::AsyncCallback&),
        Buffer&& buffer,
        OperationStatus& status
    )
    {
        boost::system::error_code error = boost::asio::error::would_block;;
        std::size_t read_size;

        ((*_socket).*func)(buffer, SocketWrapper::AsyncCallback(error, read_size));

        io_loop(&error);

        status = error_to_status(error);
        return read_size;
    }

    void io_loop(boost::system::error_code* error);

    /**
     * \brief Async wait for the timeout
     */
    void check_deadline();

    boost::asio::io_service _io_service;
    std::unique_ptr<SocketWrapper> _socket;
    boost::asio::deadline_timer _deadline{_io_service};
    boost_tcp::resolver resolver{_io_service};
};

} // namespace io
} // namespace httpony
#endif // HTTPONY_IO_SOCKET_HPP
