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
 * This server logs the contents of incoming requests to stdout and
 * returns simple "Hello World" responses to the client
 */
class ServerUpload : public httpony::Server
{
public:
    explicit ServerUpload(httpony::IPAddress listen)
        : Server(listen)
    {
        set_timeout(melanolib::time::seconds(16));
    }

    ~ServerUpload()
    {
        std::cout << "Server stopped\n";
    }

    void respond(httpony::Request& request, const httpony::Status& status) override
    {
        httpony::Response response = build_response(request, status);

        send_response(request, response);

        print_info(request, response);
    }

private:
    /**
     * \brief Parses the request payload
     */
    httpony::Status parse_body(
        httpony::Request& request,
        const httpony::Status& status
    ) const
    {
        // Handle HTTP/1.1 requests with an Expect: 100-continue header
        if ( status == httpony::StatusCode::Continue )
        {
            auto response_100 = simple_response(status, request.protocol);
            send_response(request, response_100, false);
        }

        // Parse form data
        if ( request.can_parse_post() )
        {
            if ( !request.parse_post() )
                return httpony::StatusCode::BadRequest;
        }
        else
        {
            return httpony::StatusCode::BadRequest;
        }

        return httpony::StatusCode::OK;
    }

    /**
     * \brief Returns a response for the given request
     */
    httpony::Response build_response(
        httpony::Request& request,
        httpony::Status status
    ) const
    {
        try
        {
            if ( status.is_error() )
                return simple_response(status, request.protocol);

            if ( !request.uri.path.empty() )
                return simple_response(httpony::StatusCode::NotFound, request.protocol);

            httpony::Response response(request.protocol);
            response.body.start_output("text/html");

            if ( request.method == "POST" || request.method == "PUT" )
            {
                status = parse_body(request, status);
                if ( status.is_error() )
                    return simple_response(status, request.protocol);
            }

            using namespace httpony::quick_xml::html;
            using namespace httpony::quick_xml;
            HtmlDocument html("Upload");
            html.body().append(
                Element{"form",
                    Attributes{
                        {"method", "post"},
                        {"enctype", "multipart/form-data"},
                    },
                    Element{"p",
                        Label("filename", "File Name"),
                        Input("filename", "text")},
                    Element{"p",
                        Label("contents", "Contents"),
                        Input("contents", "file")},
                    Element{"p",
                        Input("submit", "submit", "Submit")},
                }
            );

            response.body << html;

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
                       httpony::Response& response,
                       bool final_response = true
                      ) const
    {
        // We are not going to keep the connection open
        if ( final_response && response.protocol >= httpony::Protocol::http_1_1 )
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

    /**
     * \brief Prints out dictionary data
     */
    template<class Map>
        void show_headers(const std::string& title, const Map& data) const
    {
        std::cout << title << ":\n";
        for ( const auto& item : data )
            std::cout << '\t' << item.first << " : " << item.second << '\n';
    }

    /**
     * \brief Logs info on the current request/response
     */
    void print_info(httpony::Request& request,
                    const httpony::Response& response) const
    {
        std::cout << '\n';
        log_response(log_format, request, response, std::cout);

        show_headers("Headers", request.headers);
        show_headers("Post", request.post);

        std::cout << "Files:\n";
        for ( const auto& item : request.files )
        {
            show_headers("  " + item.first, item.second.headers);
            std::string body = item.second.contents;
            std::replace_if(body.begin(), body.end(), [](char c){return c < ' ' && c != '\n';}, ' ');
            std::cout << body << "\n\n";
        }
    }


    std::string log_format = "%h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-Agent}i\"";
};

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
    ServerUpload server(httpony::IPAddress{listen});

    // This starts the server on a separate thread
    server.start();

    std::cout << "Server started on port "<< server.listen_address().port << ", hit enter to quit\n";
    // Pause the main thread by listening to standard input
    std::cin.get();

    // The server destructor will stop the server and join the thread
    return 0;
}
