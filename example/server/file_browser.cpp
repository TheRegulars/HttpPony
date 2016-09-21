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

#include <boost/filesystem.hpp>
#include <magic.h>

#include "httpony.hpp"

/**
 * \brief Example server that sends back files from a given directory
 */
class ServeFiles : public httpony::Server
{
public:
    explicit ServeFiles(const std::string& path, httpony::IPAddress listen)
        : Server(listen), root(path)
    {
        magic_cookie = magic_open(MAGIC_SYMLINK|MAGIC_MIME_TYPE);
        magic_load(magic_cookie, nullptr);
        set_timeout(melanolib::time::seconds(16));
    }

    ~ServeFiles()
    {
        if ( magic_cookie )
            magic_close(magic_cookie);
    }

    void respond(httpony::Request& request, const httpony::Status& status) override
    {
        httpony::Response response;
        if ( status.is_error() )
            response = simple_response(status, request.protocol);
        else
            response = build_response(request);
        log_response(log_format, request, response, std::cout);
        send_response(request, response);
    }

protected:
    httpony::Response build_response(httpony::Request& request) const
    {
        try
        {
            if ( request.method != "GET"  && request.method != "HEAD")
                return simple_response(httpony::StatusCode::MethodNotAllowed, request.protocol);


            auto file = root;
            for ( const auto & dir : request.uri.path )
                file /= dir;

            if ( boost::filesystem::is_directory(file) )
            {
                using namespace httpony::quick_xml::html;
                httpony::Response response(request.protocol);
                response.body.start_output("text/html");
                HtmlDocument html(file.string());
                auto& list = html.body().append(List{});

                if ( !request.uri.path.empty() )
                {
                    list.add_item(Link{"..", "Parent"});
                }

                for ( const auto& item : boost::filesystem::directory_iterator(file) )
                {
                    std::string basename = item.path().filename().string();
                    list.add_item(Link{
                        (request.uri.path / basename).url_encoded(),
                        basename
                    });
                }

                html.print(response.body, true);
                response.body << '\n';
                return response;
            }
            else if ( boost::filesystem::is_regular(file) )
            {
                httpony::Response response(request.protocol);
                response.body.start_output(mime_type(file.string()));
                std::ifstream input(file.string());
                while ( input )
                {
                    char chunk[1024];
                    input.read(chunk, 1024);
                    response.body.write(chunk, input.gcount());
                }
                return response;
            }

            return simple_response(httpony::StatusCode::NotFound, request.protocol);
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

    /**
     * \brief Returns the Mime type from a file name
     */
    httpony::MimeType mime_type(const std::string& filename) const
    {
        if ( magic_cookie )
        {
            const char* mime = magic_file(magic_cookie, filename.c_str());
            if ( mime )
                return httpony::MimeType(mime);
        }
        return httpony::MimeType("application", "octet-stream");
    }

private:
    boost::filesystem::path root;
    std::string log_format = "%h %l %u %t \"%r\" %s %b \"%{Referer}i\" \"%{User-Agent}i\"";
    magic_t magic_cookie;
};

/**
 * The executable accepts two optional command line arguments:
 * the path to the directory to expose and the [address][:port] to listen on
 */
int main(int argc, char** argv)
{
    std::string path = "/home";
    std::string listen = "[::]";

    if ( argc > 1 )
        path = argv[1];

    if ( argc > 2 )
        listen = argv[2];

    ServeFiles server(path, httpony::IPAddress{listen});

    server.start();

    std::cout << "Serving files in " << path << "\n";
    std::cout << "Server started on port "<< server.listen_address().port << ", hit enter to quit\n";
    std::cin.get();
    std::cout << "Server stopped\n";

    return 0;
}
