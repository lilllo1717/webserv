#include "Http.hpp"
#include "../server/Server.hpp"



constexpr std::string_view reasonPhrase(HTTP_StatusCode status)
{
    switch (status)
    {
        case HTTP_StatusCode::OK: return "OK";
        case HTTP_StatusCode::CREATED: return "Created";
        case HTTP_StatusCode::NO_CONTENT: return "No Content";
        case HTTP_StatusCode::MOVED_PERMANENTLY: return "Moved Permanently";
        case HTTP_StatusCode::FOUND: return "Found";
        case HTTP_StatusCode::BAD_REQUEST: return "Bad Request";
        case HTTP_StatusCode::FORBIDDEN: return "Forbidden";
        case HTTP_StatusCode::NOT_FOUND: return "Not Found";
        case HTTP_StatusCode::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HTTP_StatusCode::PAYLOAD_TOO_LARGE: return "Payload Too Large";
        case HTTP_StatusCode::URI_TOO_LONG: return "URI Too Long";
        case HTTP_StatusCode::UNPROCESSABLE_ENTITY: return "Unprocessable Entity";
        case HTTP_StatusCode::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HTTP_StatusCode::NOT_IMPLEMENTED: return "Not Implemented";
        case HTTP_StatusCode::BAD_GATEWAY: return "Bad Gateway";
        case HTTP_StatusCode::GATEWAY_TIMEOUT: return "Gateway Timeout";
        default: return "Unknown";
    }
}

std::string getHostName(std::string server_host)
{

    auto pos = server_host.find(':');
    if (pos == std::string::npos)
        return server_host;
    return server_host.substr(0, pos);
}

HttpResponse constructResponse(int statusCode)
{
    HttpResponse response;

    response.statusCode = static_cast<HTTP_StatusCode>(statusCode);

    return response;

}

HttpResponse Router::handleRequest(const HttpRequest& request, const Listener& listener)
{
    (void)listener;
    HttpResponse response;
    if (request.host.empty())
    {
        std::cout << "no host name"  << "\n";

        return constructResponse(400);
    }
    const Server* selectedServer = listener.defaultServer;
    std::string hostName = getHostName(request.host);
    std::cout << "hostName: [" << hostName << "]\n";
    for (const Server* server : listener.servers)
    {
        const std::vector<std::string>& names = server->getServerNames();
        for (const std::string& name : names)
        {
            if (name == hostName)
            {
                selectedServer = server;
                std::cout << "targeted server found: " << name << "\n";
                break;
            }
        }
    }
    if (selectedServer)
    {
        std::cout << "Routing to server with port "
                << selectedServer->getListenPort() << "\n";
    }
    return constructResponse(200);
    // 1. Find matching server block (Host header)
    // 2. Find matching location block (URI path)
    // 3. Check allowed methods
    // 4. Check redirect
    // 5. Check CGI
    // 6. Serve static file
    // 7. Return proper status
}
