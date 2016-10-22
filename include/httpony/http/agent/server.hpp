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

#ifndef HTTPONY_SERVER_HPP
#define HTTPONY_SERVER_HPP

/// \cond
#include <queue>
/// \endcond

#include "httpony/io/basic_server.hpp"
#include "httpony/http/response.hpp"

namespace httpony {

/**
 * \brief Base class for a simple HTTP server
 * \note It reads POST data in a single buffer instead of streaming it
 */
class Server
{
public:

    explicit Server(IPAddress listen);

    virtual ~Server();

    /**
     * \brief Listening address
     */
    IPAddress listen_address() const;

    /**
     * \brief Changes the listening address
     * \note If the server is already running, it will need to be restarted
     *       for this to take in effect.
     */
    void set_listen_address(const IPAddress& listen);

    /**
     * \brief Starts the server in a background thread
     * \throws runtime_error if it cannot be started
     */
    void start();

    /**
     * \brief Runs the server in the current thread
     */
    bool run();

    /**
     * \brief Whether the server has been started
     * \returns \b false if it failed to run
     *          (ie: already running on a different thread)
     */
    bool running() const;

    /**
     * \brief Stops the background threads
     */
    void stop();

    /**
     * \brief The timeout for network I/O operations
     */
    melanolib::Optional<melanolib::time::seconds> timeout() const;

    void set_timeout(melanolib::time::seconds timeout);

    void clear_timeout();

    /**
     * \brief Maximum size of a request body to be accepted
     *
     * If the header area is too large, the suggested response will be 400
     * (Bad Request).
     * If the header sdection is fine but the content length of the payload is
     * too long, the suggested response will be 413 (Payload Too Large).
     *
     * If you want to restrict only the payload, leave this to unlimited and
     * check the content_length of the request body.
     *
     * \see set_unlimited_request_size(), max_request_size()
     */
    void set_max_request_size(std::size_t size);

    /**
     * \brief Removes max request limits (this is the default)
     * \see set_max_request_size(), max_request_size()
     */
    void set_unlimited_request_size();

    std::size_t max_request_size() const;


    /**
     * \brief Function handling requests
     */
    virtual void respond(Request& request, const Status& status) = 0;

    /**
     * \brief Writes a line of log into \p output based on format
     */
    void log_response(
        const std::string& format,
        const Request& request,
        const Response& response,
        std::ostream& output) const;

protected:
    /**
     * \brief Handles connection errors
     */
    virtual void error(io::Connection& connection, const OperationStatus& what) const
    {
        std::cerr << "Server error: " << connection.remote_address() << ": " << what << std::endl;
    }

    OperationStatus send(httpony::Response& response) const;

    OperationStatus send(io::Connection& connection, Response& response) const
    {
        response.connection = connection;
        return send(response);
    }
    
    OperationStatus send(io::Connection& connection, Response&& response) const
    {
        return send(connection, response);
    }

    virtual void on_connection(io::Connection& connection);

private:
    /**
     * \brief Creates a new connection object
     */
    virtual io::Connection create_connection()
    {
        return io::Connection(io::SocketTag<io::PlainSocket>{});
    }

    /**
     * \brief Whether to accept the incoming connection
     *
     * At this stage no data has been read from \p connection
     */
    virtual OperationStatus accept(io::Connection& connection)
    {
        return {};
    }

    /**
     * \brief Writes a single log item into \p output
     */
    virtual void process_log_format(
        char label,
        const std::string& argument,
        const Request& request,
        const Response& response,
        std::ostream& output
    ) const;

    void run_init();
    void run_body();


    IPAddress _connect_address;
    IPAddress _listen_address;
    io::BasicServer _listen_server;
    std::size_t _max_request_size = io::NetworkInputBuffer::unlimited_input();
    std::thread _thread;
};

/**
 * \brief Handles incoming requests in different threads
 * \todo Throttle when the queue starts growing too much
 */
template<class ServerT>
class BasicPooledServer : public ServerT
{
    static_assert(std::is_base_of<Server, ServerT>::value, "Server class expected");
public:

    template<class... Args>
        BasicPooledServer(std::size_t pool_size, Args&&... args)
            : ServerT(std::forward<Args>(args)...)
        {
            resize_pool(pool_size);
        }

    ~BasicPooledServer()
    {
        do_resize_pool(0);
    }

    /**
     * \brief Blocks until all pending connections have been processed
     *        (And prevents new connections from coming through)
     * \note Cannot be called from a thread spawned by the pool
     */
    void wait()
    {
        if ( in_pool() )
            throw std::logic_error("Cannot call BasicPooledServer::wait inside a pooled thread");

        auto lock = lock_queue();
        pause = true;
        lock.unlock();

        do_wait(true);

        lock.lock();
        pause = false;
    }

    /**
     * \brief Resizes the thread pool
     * \note Cannot be called from a thread spawned by the pool
     */
    void resize_pool(std::size_t n)
    {
        if ( n == 0 )
            throw std::logic_error("Thread pool must not be empty");
        if ( in_pool() )
            throw std::logic_error("Cannot call BasicPooledServer::resize_pool inside a pooled thread");
        do_resize_pool(n);
    }

    /**
     * \brief Number of threads in the pool
     */
    std::size_t pool_size()
    {
        auto lock = lock_queue();
        return threads.size();
    }

private:
    void on_connection(io::Connection& connection) override
    {
        auto lock = lock_queue();
        queue.push(connection);
        lock.unlock();
        handle_queue();
    }

    /**
     * \brief Goes through the available threads and starts processing queued
     *        connections
     */
    void handle_queue()
    {
        auto queue_lock = lock_queue();
        if ( queue.empty() )
            return;

        std::size_t index = 0;
        for ( auto& thread : threads )
        {
            auto thread_lock = lock_thread(index);

            if ( thread.joinable() && !running[index] )
                thread.join();

            if ( !running[index] )
            {
                running[index] = true;
                auto connection = queue.front();
                queue.pop();
                thread = std::thread([this, connection, index]() {
                    thread_run(index, connection);
                });

                if ( queue.empty() )
                    return;
            }

            index++;
        }
    }

    /**
     * \brief Whether the function is being called from within a thread of the pool
     */
    bool in_pool() const
    {
        for ( const std::thread& thread : threads )
        {
            if ( std::this_thread::get_id() == thread.get_id() )
                return true;
        }
        return false;
    }

    /**
     * \brief Acquire a lock on the thread mutex by index
     */
    std::unique_lock<std::mutex> lock_thread(std::size_t index)
    {
        if ( index >= threads.size() )
            throw std::runtime_error("Trying to lock non-existing thread");
        return std::unique_lock<std::mutex>(mutexes[index]);
    }

    /**
     * \brief Lock the queue
     */
    std::unique_lock<std::mutex> lock_queue()
    {
        return std::unique_lock<std::mutex>(mutex_queue);
    }

    /**
     * \brief Lock the queue and all thread mutexes
     */
    std::list<std::unique_lock<std::mutex>> lock_all()
    {
        std::list<std::unique_lock<std::mutex>> locks;
        locks.emplace_back(mutex_queue);
        for ( auto& mutex : mutexes )
        {
           locks.emplace_back(mutex);
        }
        return locks;
    }

    /**
     * \brief Function called by the threads
     */
    void thread_run(std::size_t thread_index, io::Connection connection)
    {
        thread_start(thread_index, connection);
        do
        {
            this->ServerT::on_connection(connection);

            if ( !pause )
            {
                std::unique_lock<std::mutex> lock(mutex_queue, std::try_to_lock);
                if ( lock.owns_lock() && !pause && !queue.empty() )
                {
                    connection = queue.front();
                    queue.pop();
                    thread_continue(thread_index, connection);
                    continue;
                }
            }
        }
        while ( false );

        running[thread_index] = false;

        thread_stop(thread_index);
    }

    /**
     * \brief Unchecked resize for internal use
     */
    void do_resize_pool(std::size_t n)
    {
        std::unique_lock<std::mutex> lock(mutex_queue);
        pause = true;
        do_wait(false);
        threads.resize(n);
        running = std::vector<std::atomic<bool>>(n);
        mutexes = std::vector<std::mutex>(n);
        pause = false;
    }

    /**
     * \brief Unchecked wait for internal use
     */
    void do_wait(bool lock)
    {
        std::size_t index = 0;
        for ( auto& thread : threads )
        {
            std::unique_lock<std::mutex> mlock;
            if ( lock )
                mlock = lock_thread(index);
            if ( thread.joinable() )
                thread.join();
            index++;
        }
    }

    /**
     * \brief Called when a thread is spawned
     */
    virtual void thread_start(std::size_t index, io::Connection& connection)
    {
    }

    /**
     * \brief Called when a thread picks up a new connection
     */
    virtual void thread_continue(std::size_t index, io::Connection& connection)
    {
    }

    /**
     * \brief Called right before a thread exits
     */
    virtual void thread_stop(std::size_t index)
    {
    }

    /**
     * \brief mutex protecting the queue
     */
    std::mutex mutex_queue;
    /**
     * \brief Queue of incoming connections to be processed
     * \note protected by mutex_queue
     */
    std::queue<io::Connection> queue;
    /**
     * \brief Whether the queue should be temporarily paused
     * \note protected by mutex_queue
     */
    bool pause = false;
    /**
     * \brief Thread pool
     */
    std::vector<std::thread> threads;
    /**
     * \brief Indicates whether each thread is running
     */
    std::vector<std::atomic<bool>> running;
    /**
     * \brief A mutex for each thread in the pool to synchronize creations and joins
     */
    std::vector<std::mutex> mutexes;
};

using PooledServer = BasicPooledServer<Server>;

/**
 * \brief Calls a functor on incoming requests
 */
template<
    class ServerT,
    class RequestFunctor = std::function<void (Request&, const Status&)>,
    class ErrorFunctor = std::function<void (io::Connection&, const OperationStatus&)>
>
class ClosureServer : public ServerT
{
public:
    template<class... Args>
        ClosureServer(
            RequestFunctor request_functor,
            ErrorFunctor error_functor,
            Args&&... args)
    : ServerT(std::forward<Args>(args)...),
      request_functor(std::move(request_functor)),
      error_functor(std::move(error_functor))
    {}

    template<class... Args>
        ClosureServer(
            RequestFunctor request_functor,
            Args&&... args)
    : ClosureServer(
        std::forward<RequestFunctor>(request_functor),
        {},
        std::forward<Args>(args)...
    )
    {}

    void respond(Request& request, const Status& status)
    {
        melanolib::callback(request_functor, request, status);
    }

protected:
    void error(io::Connection& connection, const OperationStatus& what) const
    {
        melanolib::callback(error_functor, connection, what);
    }

private:
    RequestFunctor request_functor;
    ErrorFunctor error_functor;
};

} // namespace httpony
#endif // HTTPONY_SERVER_HPP
