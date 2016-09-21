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
 * The executable accepts an optional command line argument to change the
 * listen [address][:port]
 */
int main(int argc, char** argv)
{
    std::string listen = "[::]";

    if ( argc > 1 )
        listen = argv[1];

    // This creates a server that listens on the given address
    httpony::ClosureServer<httpony::Server> server(
        [](httpony::Request& request, const httpony::Status& status) {
            httpony::Response response(request.protocol);
            response.body.start_output("text/plain");
            response.body << "Hello world!\n";

            /// \todo Make the following bit a bit easier
            ///       Should make use of Server::send()
            response.connection = request.connection;
            auto stream = response.connection.send_stream();
            httpony::Http1Formatter().response(stream, response);
            return stream.send();
        },
        httpony::IPAddress{listen}
    );

    // This starts the server on a separate thread
    server.start();
    std::cout << "Server started on port " << server.listen_address().port << ", hit enter to quit\n";

    // Pause the main thread listening to standard input
    std::cin.get();
    std::cout << "Server stopped\n";

    // The destructor will stop the server and join the thread
    return 0;
}
