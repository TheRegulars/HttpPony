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
#ifndef HTTPONY_HTTP_AGENT_CLIENT_HPP
#define HTTPONY_HTTP_AGENT_CLIENT_HPP

/// \cond
#include <list>
#include <type_traits>

#include <melanolib/utils/movable.hpp>
#include <melanolib/utils/functional.hpp>
/// \endcond

#include "httpony/io/basic_client.hpp"
#include "httpony/http/response.hpp"

namespace httpony {

class Client
{
public:
    explicit Client(
        int max_redirects = 0,
        const UserAgent& user_agent = UserAgent::default_user_agent()
    )
        : _user_agent(user_agent),
          _max_redirects(melanolib::math::max(0, max_redirects))
    {}

    virtual ~Client()
    {
    }

    /**
     * \brief Creates a connection to the target of the geiven Uri
     * \param[in]  target URI of the server to connect to
     * \param[out] status Status of the operation
     */
    io::Connection connect(Uri target, OperationStatus& status)
    {
        if ( target.scheme.empty() )
            target.scheme = "http";

        auto connection = create_connection(target);
        status = _basic_client.connect(target, connection);

        if ( !status.error() )
            status = on_connect(target, connection);

        return std::move(connection);
    }

    OperationStatus query(Request& request, Response& response)
    {
        OperationStatus status;
        auto connection = connect(request.url, status);

        if ( status.error() )
            return status;

        return get_response(connection, request, response);
    }

    /**
     * \brief Writes the request and retrieves the response over a connection object
     */
    OperationStatus get_response(io::Connection& connection, Request& request, Response& response);

    /**
     * \brief The timeout for network I/O operations
     */
    melanolib::Optional<melanolib::time::seconds> timeout() const
    {
        return _basic_client.timeout();
    }

    void set_timeout(melanolib::time::seconds timeout)
    {
        _basic_client.set_timeout(timeout);
    }

    void clear_timeout()
    {
        _basic_client.clear_timeout();
    }

    const UserAgent& user_agent() const
    {
        return _user_agent;
    }

    void set_user_agent(const UserAgent& user_agent)
    {
        _user_agent = user_agent;
    }

    void set_max_redirects(int max_redirects)
    {
        _max_redirects = melanolib::math::max(0, max_redirects);
    }

    int max_redirects() const
    {
        return _max_redirects;
    }

    std::size_t max_response_size() const
    {
        return _max_response_size;
    }

    /**
     * \brief Maximum response size
     *
     * The client will stop parsing any response larger than this.
     * (Defaults to unlimited).
     *
     * \see set_ulimited_response_size(), max_response_size()
     */
    void set_max_response_size(std::size_t size)
    {
        _max_response_size = size;
    }

    void set_ulimited_response_size()
    {
        set_max_response_size(io::NetworkInputBuffer::unlimited_input());
    }

protected:
    /**
     * \brief Called right before a request is sent to the connection
     */
    virtual void process_request(Request& request)
    {
        request.user_agent = _user_agent;
        if ( !request.post.empty() && !request.body.has_data() )
            request.format_post();
    }

    /**
     * \brief Called right after a request is successfully received from the connection
     */
    virtual void process_response(Request& request, Response& response)
    {
    }

    virtual OperationStatus on_attempt(Request& request, Response& response, int attempt_number);

    /**
     * \brief Creates a new connection object
     */
    virtual io::Connection create_connection(const Uri& target)
    {
        return io::Connection(io::SocketTag<io::PlainSocket>{});
    }

    OperationStatus get_response_attempt(int attempt, Request& request, Response& response);
    
    virtual OperationStatus on_connect(const Uri& target, io::Connection& connection)
    {
        return {};
    }

private:
    template<class ClientT>
        friend class BasicAsyncClient;

    io::BasicClient _basic_client;
    UserAgent _user_agent;
    int _max_redirects = 0;
    std::size_t _max_response_size = io::NetworkInputBuffer::unlimited_input();
};

template<class ClientT>
class BasicAsyncClient : public ClientT
{
    static_assert(std::is_base_of<Client, ClientT>::value, "Client class expected");
    using item_iterator = typename std::list<Request>::iterator;

public:
    template<class... Args>
        BasicAsyncClient(Args&&... args)
            : ClientT(std::forward<Args>(args)...),
              should_run(true)
        {}

    ~BasicAsyncClient()
    {
        stop();
    }

    void start()
    {
        if ( !started() )
        {
            should_run = true;
            thread = std::move(std::thread([this]{ run(); }));
        }
    }

    bool started() const
    {
        return thread.joinable();
    }

    void stop()
    {
        if ( started() )
        {
            should_run = false;
            condition.notify_all();
            thread.join();
        }
    }

    void run()
    {
        while( should_run )
        {
            std::unique_lock<std::mutex> lock(mutex);
            while ( items.empty() && should_run )
                condition.wait(lock);

            if ( !should_run )
                return;

            std::vector<io::Connection> connections;
            connections.reserve(items.size());

            for ( auto& item : items )
                connections.push_back(item.connection);

            lock.unlock();
            for ( auto connection : connections )
                connection.socket().process_async();
            lock.lock();

            std::unique_lock<std::mutex> old_lock(old_requests_mutex);
            for ( auto it_conn = items.begin(); it_conn != items.end(); )
            {
                it_conn = remove_old(it_conn);
            }
        }
    }

    template<class OnResponse, class OnConnect, class OnError>
        void async_query(
            Request&& request,
            const OnResponse& on_response,
            const OnConnect& on_connect,
            const OnError& on_error)
    {
        request.connection = ClientT::create_connection(request.url);

        std::unique_lock<std::mutex> lock(mutex);
        items.emplace_back(std::move(request));
        auto& item = items.back();
        lock.unlock();

        basic_client().async_connect(item.url, item.connection,
            [this, on_response, on_connect, on_error, &item]()
            {
                this->on_connect(item.url, item.connection);
                melanolib::callback(on_connect, item);
                Response response;
                /// \todo get_response is not async
                auto status = ClientT::get_response(item.connection, item, response);
                if ( status.error() )
                {
                    this->on_error(item, status);
                    melanolib::callback(on_error, item, status);
                }
                else
                {
                    //this->process_response(item, response);
                    melanolib::callback(on_response, item, response);
                }
                clean_request(&item);
            },
            [this, on_error, &item](const OperationStatus& status)
            {
                this->on_error(item, status);
                melanolib::callback(on_error, item, status);
                clean_request(&item);
            }
        );

        condition.notify_one();
    }

    void async_query(Request&& request)
    {
        async_query(
            std::move(request),
            [this](Request& request, Response& response){
                return on_response(request, response);
            },
            melanolib::Noop(),
            melanolib::Noop()
        );
    }

private:

    io::BasicClient& basic_client()
    {
        return this->_basic_client;
    }

    virtual void on_error(Request& request, const OperationStatus& status)
    {
    }

    virtual void on_response(Request& request, Response& response)
    {
    }

    void clean_request(Request* request)
    {
        std::unique_lock<std::mutex> lock(old_requests_mutex);
        old_requests.push_back(request);
        lock.unlock();
        condition.notify_one();
    }

    item_iterator remove_old(item_iterator iter)
    {
        for ( auto it_old = old_requests.begin(); it_old != old_requests.end(); )
        {
            if ( *it_old == &*iter )
            {
                old_requests.erase(it_old);
                return items.erase(iter);
            }
        }
        return ++iter;
    }

    std::thread thread;
    std::condition_variable condition;

    std::mutex mutex;
    std::atomic<bool> should_run = true;
    std::list<Request> items;

    std::mutex old_requests_mutex;
    std::list<Request*> old_requests;
};

using AsyncClient = BasicAsyncClient<Client>;

} // namespace httpony
#endif // HTTPONY_HTTP_AGENT_CLIENT_HPP
