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
#include "httpony.hpp"

/**
 * \brief Simple example server
 *
 * This server only supports GET and returns
 * simple "Hello World" responses to the client
 */
class SimpleServer : public httpony::Server
{
public:
    explicit SimpleServer(httpony::io::ListenAddress listen)
        : Server(listen)
    {
        set_timeout(melanolib::time::seconds(16));
    }

    void respond(httpony::Request& request, const httpony::Status& status) override
    {
        httpony::Response response = build_response(request, status);
        log_response(log_format, request, response, std::cout);
        send_response(request, response);
    }

protected:
    /**
     * \brief Returns a response for the given request
     */
    httpony::Response build_response(
        httpony::Request& request,
        const httpony::Status& status) const
    {
        try
        {
            if ( status.is_error() )
                return simple_response(status, request.protocol);

            if ( request.method != "GET" && request.method != "HEAD")
                return simple_response(httpony::StatusCode::MethodNotAllowed, request.protocol);

            if ( !request.uri.path.empty() )
                return simple_response(httpony::StatusCode::NotFound, request.protocol);

            httpony::Response response(request.protocol);
            response.body.start_output("text/plain");
            response.body << "Hello world!";
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

private:
    std::string log_format = "%h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-Agent}i\"";
};

/**
 * The executable accepts an optional command line argument to change the
 * listen port
 */
int main(int argc, char** argv)
{
    uint16_t port = 0;

    if ( argc > 1 )
        port = std::stoul(argv[1]);

    // This creates a server that listens to both IPv4 and IPv6
    // on the given port
    SimpleServer server(port);

    // This starts the server on a separate thread
    server.start();
    std::cout << "Server started on port " << server.listen_address().port << ", hit enter to quit\n";

    // Pause the main thread listening to standard input
    std::cin.get();
    std::cout << "Server stopped\n";

    // The destructor will stop the server and join the thread
    return 0;
}
