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

class Server : public httpony::Server
{
public:
    explicit Server(httpony::io::ListenAddress listen)
        : httpony::Server(listen)
    {}

    void respond(httpony::Request& request, const httpony::Status& status) override
    {
        httpony::Response response = build_response(request, status);

        std::cout << "=============\nServer:\n";
        httpony::http::Http1Formatter("\n").request(std::cout, request);
        std::cout << "\n=============\n";

        send_response(request, response);
    }

    ~Server()
    {
        std::cout << "Server stopped\n";
    }

protected:
    httpony::Response build_response(httpony::Request& request, const httpony::Status& status) const
    {
        using namespace httpony::quick_xml;
        using namespace httpony::quick_xml::html;
        try
        {
            if ( status.is_error() )
                return simple_response(status, request.protocol);

            if ( request.uri.path.string() == "/login" )
            {
                if ( request.method == "POST" )
                {
                    request.parse_post();

                    if ( request.post["username"] == "admin" &&
                         request.post["password"] == "secret" )
                    {
                        auto response = httpony::Response::redirect(request.get["next"]);
                        response.cookies.append("logged_in", request.post["username"]);
                        return response;
                    }
                }

                httpony::Response response(request.protocol);
                response.headers["X-Login-Page"] = "Yes";
                response.body.start_output("text/html");

                HtmlDocument doc("Login");
                doc.body().append(
                    Element{"form", Attribute("method", "post"),
                        Label("username", "Username"),
                        Input("username", "text", request.post["username"]),
                        Label("password", "Password"),
                        Input("password", "password", request.post["password"]),
                        Input("submit", "submit", "Submit"),
                    }
                );
                doc.print(response.body, true);

                return response;
            }

            if ( request.uri.path.empty() || request.uri.path.front() != "admin" )
                return httpony::Response::redirect("/admin"+request.uri.path.string());

            if ( !request.cookies.contains("logged_in") )
                return httpony::Response::redirect("/login?next="+request.uri.full());

            httpony::Response response(request.protocol);
            response.body.start_output("text/html");
            HtmlDocument doc("Hello");
            doc.body().append(
                Element("p", Text("Welcome " + request.cookies["logged_in"] + "!"))
            );
            doc.print(response.body, true);

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

class Client : public httpony::AsyncClient
{
public:
    Client()
        : httpony::AsyncClient(5)
    {}

    ~Client()
    {
        std::cout << "Client stopped\n";
    }

protected:
    void process_request(httpony::Request& request) override
    {
        httpony::AsyncClient::process_request(request);

        // Clear old cookies
        melanolib::time::DateTime now;
        cookies.erase(
            std::remove_if(cookies.begin(), cookies.end(),
                [&now](const auto& cookie) {
                    return cookie.second.expired(now);
                }
            ),
            cookies.end()
        );

        // Set request cookies
        for ( const auto& cookie : cookies )
        {
            if ( cookie.second.matches_uri(request.uri) )
                request.cookies[cookie.first] = cookie.second.value;
        }
    }

    void process_response(httpony::Request& request, httpony::Response& response) override
    {
        std::cout.clear();
        std::cout << "=============\nClient:\n";
        httpony::http::Http1Formatter("\n").response(std::cout, response);
        std::cout << "\n=============\n";

        for ( const auto& cookie : response.cookies )
        {
            if ( cookie.second.matches_domain(request.uri.authority.host) )
                cookies.append(cookie.first, cookie.second);
        }
    }

private:
    void on_error(httpony::Request& request, const httpony::OperationStatus& status) override
    {
        std::cerr << "Error accessing " << request.uri.full() << ": " << status.message() << std::endl;
    }

    void on_response(httpony::Request& request, httpony::Response& response) override
    {
        if ( response.headers.contains("X-Login-Page") )
        {
            httpony::Request login_request("POST", request.uri);
            login_request.post["username"] = "admin";
            login_request.post["password"] = "secret";
            async_query(std::move(login_request));
        }
        else
        {
            std::cout << "Client: Request finished\n";
        }
    }

    httpony::ClientCookieJar cookies;
};


int main(int argc, char** argv)
{
    uint16_t port = 0;

    if ( argc > 1 )
        port = std::stoul(argv[1]);

    // This creates a server that listens to both IPv4 and IPv6
    // on the given port
    Server server(port);

    // This starts the server on a separate thread
    server.start();
    port = server.listen_address().port;
    std::cout << "Server started on port " << port << "\n";

    // This starts the client on a separate thread
    Client client;
    client.start();
    std::cout << "Client started\n";
    client.async_query(httpony::Request("GET",
        "http://localhost:" + std::to_string(port) + "/home"
    ));

    // Pause the main thread listening to standard input
    std::cout << "Hit enter to quit\n";
    std::cin.get();

    // The destructors will stop client and server and join the thread
    return 0;
}
