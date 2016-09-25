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
#include <fstream>
#include "httpony/ssl/ssl.hpp"

/**
 * \brief Simple example server
 *
 * This server logs the contents of incoming requests to stdout and
 * returns simple "Hello World" responses to the client
 */
class MaybeSslHelloServer : public httpony::ssl::SslServer
{
public:
    explicit MaybeSslHelloServer(httpony::IPAddress listen)
        : SslServer(listen, false)
    {
        set_timeout(melanolib::time::seconds(16));
    }

    void respond(httpony::Request& request, const httpony::Status& status) override
    {
        httpony::Response response = build_response(request, status);
        send_response(request, response);
        log_response(log_format, request, response, std::cout);
    }

private:
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

            httpony::Response response(request.protocol);
            response.body.start_output("text/plain");
            response.body << "Hello world!\r\n";
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

        // Drop the connection if there is some network error on the response
        if ( !send(request.connection, response) )
            request.connection.close();
    }

    std::string log_format = "%h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-Agent}i\"";
    std::size_t max_size = 8192;
};

/**
 * The executable accepts the following (optional) arguments:
 *  address
 *      [address][:port] number to listen to (default=[::])
 *  cert_file
 *      Certificate file in PEM format (default=server.pem)
 *  key_file
 *      Private key file in PEM format (default=server.key)
 *  dh_file
 *      Diffie-Hellman paramaters file in PEM format (default=empty)
 *
 * To generate the various PEM files you can use the following commands:
 *
 * Generate RSA key:
 *      openssl genrsa -out server.key 1024
 *
 * Generate self-signed certificate from the key file:
 *      openssl req -days 365 -out server.pem -new -x509 -key server.key
 *
 * (Optional) Generate DH parameters:
 *      openssl dhparam -out dh512.pem 512
 *
 * Since these files are in the PEM format, you can also concatenate them and
 * keep a single file that will work for all three
 */
int main(int argc, char** argv)
{
    std::string listen = "[::]";
    std::string cert_file = "server.pem";
    std::string key_file = "server.key";
    std::string dh_file;

    if ( argc > 1 )
        listen = argv[1];

    if ( argc > 2 )
        cert_file = argv[2];

    if ( argc > 3 )
        key_file = argv[3];

    if ( argc > 4 )
        dh_file = argv[4];

    // This creates a server that listens on the given address
    MaybeSslHelloServer server(httpony::IPAddress{listen});

    if ( cert_file.empty() || !std::ifstream(cert_file).is_open() ||
         key_file.empty() || !std::ifstream(key_file).is_open() )
    {
        server.set_ssl_enabled(true);
        server.set_certificate(cert_file, key_file, dh_file);
    }

    // This starts the server on a separate thread
    server.start();

    std::cout << "Server started on port "<< server.listen_address().port << ", hit enter to quit\n";
    std::cout << "SSL support has been " << (server.ssl_enabled() ? "enabled" : "disabled") << '\n';
    // Pause the main thread listening to standard input
    std::cin.get();
    std::cout << "Server stopped\n";

    // The MyServer destructor will stop the server and join the thread
    return 0;
}
