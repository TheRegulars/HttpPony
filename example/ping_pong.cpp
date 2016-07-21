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

class PingPongServer : public httpony::Server
{
public:
    explicit PingPongServer(httpony::io::ListenAddress listen)
        : Server(listen)
    {
        set_timeout(melanolib::time::seconds(16));
    }

    void respond(httpony::io::Connection& connection, httpony::Request&& request) override
    {
        httpony::Response response = build_response(connection, request);
        log_response(log_format, connection, request, response, std::cout);
        send_response(connection, request, response);
    }

protected:
    httpony::Response build_response(httpony::io::Connection& connection, httpony::Request& request) const
    {
        try
        {
            if ( request.method != "GET"  && request.method != "HEAD")
                request.suggested_status = httpony::StatusCode::MethodNotAllowed;
            else if ( request.url.path.string() != "ping" )
                request.suggested_status = httpony::StatusCode::NotFound;

            if ( request.suggested_status.is_error() )
                return simple_response(request);

            httpony::Response response(request);
            response.body.start_output("text/plain");
            response.body << "pong";
            return response;
        }
        catch ( const std::exception& )
        {
            // Create a server error response if an exception happened
            // while handling the request
            request.suggested_status = httpony::StatusCode::InternalServerError;
            return simple_response(request);
        }
    }

    /**
     * \brief Creates a simple text response containing just the status message
     */
    httpony::Response simple_response(const httpony::Request& request) const
    {
        httpony::Response response(request);
        response.body.start_output("text/plain");
        response.body << response.status.message << '\n';
        return response;
    }

    /**
     * \brief Sends the response back to the client
     */
    void send_response(httpony::io::Connection& connection,
                       httpony::Request& request,
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

        if ( !connection.send_response(response) )
            connection.close();
    }

private:
    std::string log_format = "SV: %h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-Agent}i\"";
};

void queue_request(httpony::Client& client, const httpony::Authority& server)
{
    client.queue_request(httpony::Request(
        "GET",
        httpony::Uri("http", server, httpony::Path("ping"), {}, {})
    ));
}

int main(int argc, char** argv)
{
    uint16_t port = 8084;

    if ( argc > 1 )
        port = std::stoul(argv[1]);

    // This creates a server that listens to both IPv4 and IPv6
    // on the given port
    httpony::Authority sv_auth;
    sv_auth.host = "localhost";
    sv_auth.port = port;
    PingPongServer server(port);

    // This starts the server on a separate thread
    server.start();
    std::cout << "Server started on port " << server.listen_address().port << "\n";

    // This starts the client on a separate thread
    httpony::Client client;
    queue_request(client, sv_auth);
    client.start();
    std::cout << "Client started\n";

    // Pause the main thread listening to standard input
    std::cout << "Hit enter to quit\n";
    std::cin.get();
    std::cout << "Client stopped\n";
    std::cout << "Server stopped\n";

    // The destructors will stop client and server and join the thread
    return 0;
}