#include "Http.hpp"
#include "../server/Server.hpp"
#include "../cgi/Cgi.hpp"
#include "RequestHandler.hpp"

// constexpr std::string_view reasonPhrase(HTTP_StatusCode status)
// {
//     switch (status)
//     {
//         case HTTP_StatusCode::OK: return "OK";
//         case HTTP_StatusCode::CREATED: return "Created";
//         case HTTP_StatusCode::NO_CONTENT: return "No Content";
//         case HTTP_StatusCode::MOVED_PERMANENTLY: return "Moved Permanently";
//         case HTTP_StatusCode::FOUND: return "Found";
//         case HTTP_StatusCode::BAD_REQUEST: return "Bad Request";
//         case HTTP_StatusCode::FORBIDDEN: return "Forbidden";
//         case HTTP_StatusCode::NOT_FOUND: return "Not Found";
//         case HTTP_StatusCode::METHOD_NOT_ALLOWED: return "Method Not Allowed";
//         case HTTP_StatusCode::PAYLOAD_TOO_LARGE: return "Payload Too Large";
//         case HTTP_StatusCode::URI_TOO_LONG: return "URI Too Long";
//         case HTTP_StatusCode::UNPROCESSABLE_ENTITY: return "Unprocessable Entity";
//         case HTTP_StatusCode::INTERNAL_SERVER_ERROR: return "Internal Server Error";
//         case HTTP_StatusCode::NOT_IMPLEMENTED: return "Not Implemented";
//         case HTTP_StatusCode::BAD_GATEWAY: return "Bad Gateway";
//         case HTTP_StatusCode::GATEWAY_TIMEOUT: return "Gateway Timeout";
//         default: return "Unknown";
//     }
// }

HttpResponse constructResponse(HttpResponse& response)
{
    std::cout << "HTTP/1.1" << "\n";
    std::cout << "Status: " << reasonPhrase(response.statusCode) << "\n";
    for (const auto& head: response.headers)
        std::cout << head.first << ": " << head.second << "\n";
    std::string bodyy;
    bodyy.append(reinterpret_cast<const char*>(response.body.data()), response.body.size());
    std::cout << "Body: " << bodyy << "\n";
    return response;

}

std::string getHostName(std::string server_host)
{

    auto pos = server_host.find(':');
    if (pos == std::string::npos)
        return server_host;
    return server_host.substr(0, pos);
}

bool matchRouteWithConfig(const std::string& requestUri, const std::string& configUri)
{
    std::cout << "requestUri: [" << requestUri << "]\n";
    std::cout << "configUri: [" << configUri << "]\n";

    if (requestUri == configUri)
    {
        std::cout << "path matches" << "\n";
        return true;
    }
    if (requestUri.find(configUri) != 0)
    {
        std::cout << "no match" << "\n";
        return false;
    }
    if (configUri[configUri.size() - 1] == '/')
    {
        std::cout << "just root uri" << "\n";
        return true;
    }
    std::cout << "configUri[requestUri.size()]  [" << requestUri[configUri.size()] << "]\n";
    if (requestUri.size() > configUri.size() && requestUri[configUri.size()] == '/')
    {
        std::cout << "match, longer uri" << "\n";
        return true;
    }
    std::cout << "matchRouteWithConfig returns false" << "\n";
    return false;
}

std::string methodToString(HTTP_Method method)
{
    switch (method)
    {
        case HTTP_GET:
            return "GET";
        case HTTP_DELETE:
            return "DELETE";
        case HTTP_POST:
            return "POST";
        default:
            return "";
    }
}

std::string extractExtensionFromUri(HttpRequest request)
{
    std::cout << "request.uri_path: [" << request.uri_path << "]\n";
    size_t lastSlash = request.uri_path.rfind("/");
    size_t lastDot = request.uri_path.rfind(".");

    if (lastDot == std::string::npos || lastDot < lastSlash || lastDot == lastSlash + 1)
        return "";
     std::cout << "request.uri_path.substr(lastDot) [" << request.uri_path.substr(lastDot) << "]\n";
    return request.uri_path.substr(lastDot);

} 

HttpResponse Router::handleRequest(HttpRequest& request, const Listener& listener, const std::string& remoteAddr)
{
    (void)listener;
    RequestMatchResult matchResult;
    HttpResponse response;
    if (request.host.empty())
    {
        std::cout << "no host name"  << "\n";

        response.statusCode = static_cast<HTTP_StatusCode>(400);
        return constructResponse(response);
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
    const routeConfig* bestMatchRouteConfig = NULL;
    serverConfig selectedServerConfig = selectedServer->getConfig();

    size_t max_uri_len = 0;
    // std::cout << "route.path" <<  selectedServer->getConfig().routes << "\n";

    for (const routeConfig& route : selectedServer->getConfig().routes)
    {
        std::cout << "route.path [" <<  route.path << "]\n";
        if (matchRouteWithConfig(request.uri_path, route.path))
        {
            std::cout << "entered true matchRouteWithConfig" << "\n";

            if (route.path.size() > max_uri_len)
            {
                max_uri_len = route.path.size();
                bestMatchRouteConfig = &route;
                std::cout << "interm uri: " << bestMatchRouteConfig->path << "\n";
            }
        }   
    }
    if (bestMatchRouteConfig == NULL)
    {
        std::cout << "error ar matching uri "  "\n";
        response.statusCode = static_cast<HTTP_StatusCode>(404);
        return constructResponse(response);
        
    }
    std::cout << "bestMatchRouteConfig uri: " << bestMatchRouteConfig->path << "\n";
    std::string methodStringed = methodToString(request.method);
    std::cout << "methodStringed: " << methodStringed << "\n";

    if (std::find(bestMatchRouteConfig->httpMethods.begin(), bestMatchRouteConfig->httpMethods.end(), methodStringed)
         == bestMatchRouteConfig->httpMethods.end())
         {
            // std::cout << "no method match." << "\n";
            response.statusCode = static_cast<HTTP_StatusCode>(405);
            return constructResponse(response);
         }
    if (bestMatchRouteConfig->isRedirect == true)
    {
        response.statusCode = static_cast<HTTP_StatusCode>(bestMatchRouteConfig->redirectCode);
        request.headers["location"] = bestMatchRouteConfig->uploadPath;
        response.statusCode = static_cast<HTTP_StatusCode>(bestMatchRouteConfig->redirectCode);
        return constructResponse(response);

    }
    std::string extensionFromRequest = extractExtensionFromUri(request);
    const std::map<std::string, std::string>& cgi = bestMatchRouteConfig->cgi;
    if (cgi.empty())
    {
        std::cout << "CGI is empty =(( " << "\n";

    }

    for (std::map<std::string, std::string>::const_iterator it = cgi.begin();
        it != cgi.end();
        ++it)
    {
        matchResult.interpreter = it->second;
        std::cout << "CGI extension: " << it->first
                << " interpreter: " << it->second << "\n";
    }
    matchResult.selectedServer = selectedServer;
    matchResult.bestMatchRouteConfig = bestMatchRouteConfig;
    matchResult.stringifiedMethod = methodStringed;
    matchResult.remoteAddress = remoteAddr;
    matchResult.selectedServerCon = selectedServerConfig;
    std::map<std::string, std::string>::const_iterator it = bestMatchRouteConfig->cgi.find(extensionFromRequest);
    if (it != bestMatchRouteConfig->cgi.end())
    {
        CgiHandler cgi(request, *bestMatchRouteConfig, matchResult);
        std::cout << "Pair Found, execute CGI." << "\n";
        response = cgi.executeCgi();
    }
    else
    {
        std::cout << "Pair not found. Using normal handler" << "\n";
		response = RequestHandler::executeNormal(request, *bestMatchRouteConfig);
    }
	return response;
}
