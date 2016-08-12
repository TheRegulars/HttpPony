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
#include "httpony/ssl/ssl.hpp"

class PingPongServer : public httpony::ssl::SslServer
{
public:
    explicit PingPongServer(
        httpony::io::ListenAddress listen,
        const std::string& cert_file,
        const std::string& key_file,
        const std::string& dh_file = {}
    ) : SslServer(listen, cert_file, key_file, dh_file)
    {
        set_timeout(melanolib::time::seconds(16));
    }

    void respond(httpony::Request& request, const httpony::Status& status) override
    {
        httpony::Response response = build_response(request, status);

        std::cout << "=============\nServer:\n";
        httpony::http::Http1Formatter("\n").request(std::cout, request);
        std::cout << "=============\n";

        send_response(request, response);
    }

    ~PingPongServer()
    {
        std::cout << "Server stopped\n";
    }

protected:
    httpony::Response build_response(httpony::Request& request, const httpony::Status& status) const
    {
        try
        {
            if ( status.is_error() )
                return simple_response(status, request.protocol);

            if ( request.method != "GET"  && request.method != "HEAD")
                return simple_response(httpony::StatusCode::MethodNotAllowed, request.protocol);

            if ( request.url.path.string() != "/ping" )
                return simple_response(httpony::StatusCode::NotFound, request.protocol);

            httpony::Response response(request.protocol);
            response.body.start_output("text/plain");
            response.body << "pong\n";
            return response;
        }
        catch ( const std::exception& )
        {
            // Create a server error response if an exception happened
            // while handling the request
            return simple_response(httpony::StatusCode::InternalServerError, request.protocol);
        }
    }

    /**
     * \brief Creates a simple text response containing just the status message
     */
    httpony::Response simple_response(
        const httpony::Status& status,
        const httpony::Protocol& protocol) const
    {
        httpony::Response response(status, protocol);
        response.body.start_output("text/plain");
        response.body << response.status.message << '\n';
        return response;
    }

    /**
     * \brief Sends the response back to the client
     */
    void send_response(httpony::Request& request,
                       httpony::Response& response) const
    {
        // We are not going to keep the connection open
        if ( response.protocol >= httpony::Protocol::http_1_1 )
        {
            response.headers["Connection"] = "close";
        }

        // Ensure the response isn't cached
        response.headers["Expires"] = "0";

        // This removes the response body when mandated by HTTP
        response.clean_body(request);

        if ( !send(request.connection, response) )
            request.connection.close();
    }
};

class PingPongClient : public httpony::BasicAsyncClient<httpony::ssl::SslClient>
{
public:
    PingPongClient(const httpony::Authority& server)
        : uri("https", server, httpony::Path("ping"), {}, {})
    {}

    ~PingPongClient()
    {
        std::cout << "Client stopped\n";
    }

    void create_request()
    {
        async_query(httpony::Request("GET", uri));
    }

protected:
    void process_response(httpony::Request& request, httpony::Response& response) override
    {
        std::cout << "=============\nClient:\n";
        httpony::http::Http1Formatter("\n").response(std::cout, response);
        std::cout << "=============\n";
    }

    void on_error(httpony::Request& request, const httpony::OperationStatus& status) override
    {
        std::cerr << "Client error: " << request.url.full() << ": " << status.message() << std::endl;
    }

    void on_response(httpony::Request& request, httpony::Response& response) override
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        create_request();
    }

    httpony::Uri uri;
};


int main(int argc, char** argv)
{
    uint16_t port = 0;
    std::string cert_file = "server.pem";
    std::string key_file = "server.key";
    std::string dh_file;

    if ( argc > 1 )
        port = std::stoul(argv[1]);

    if ( argc > 2 )
        cert_file = argv[2];

    if ( argc > 3 )
        key_file = argv[3];

    if ( argc > 4 )
        dh_file = argv[4];

    // This creates a server that listens to both IPv4 and IPv6
    // on the given port
    httpony::Authority sv_auth;
    PingPongServer server(port, cert_file, key_file, dh_file);

    // This starts the server on a separate thread
    server.start();
    std::cout << "Server started on port " << server.listen_address().port << "\n";

    // This starts the client on a separate thread
    PingPongClient client(server.listen_address());
    client.start();
    std::cout << "Client started\n";
    client.create_request();

    // Pause the main thread listening to standard input
    std::cout << "Hit enter to quit\n";
    std::cin.get();

    // The destructors will stop client and server and join the thread
    return 0;
}
