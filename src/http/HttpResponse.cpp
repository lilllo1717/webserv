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

Server* findServerForRequest(const Listener& listener, const HttpRequest& request)
{
    std::string hostName = getHostName(request.host);
    for (const Server* server : listener.servers)
    {
        const std::vector<std::string>& names = server->getServerNames();
        for (const std::string& name : names)
        {
            if (name == hostName)
            {
                return const_cast<Server*>(server);
            }
        }
    }
    return listener.defaultServer;
}

void printRouteInfo(const routeConfig& route)
{
    std::cout << "\nDEBUG: Route path: " << route.path << "\n";
    std::cout << "HTTP Methods: ";
    for (const std::string& method : route.httpMethods)
    {
        std::cout << method << " ";
    }
    std::cout << "\n";
    std::cout << "Root Directory: " << route.rootDir << "\n";
    std::cout << "Index File: " << route.index << "\n";
    std::cout << "Autoindex: " << (route.autoindex ? "Enabled" : "Disabled") << "\n";
    if (route.isRedirect)
    {
        std::cout << "Redirect Code: " << route.redirectCode << "\n";
        std::cout << "Redirect Target: " << route.redirectTarget << "\n";
    }
    std::cout << "Allow Upload: " << (route.allow_upload ? "Yes" : "No") << "\n";
    if (route.allow_upload)
    {
        std::cout << "Upload Path: " << route.uploadPath << "\n";
    }
    if (!route.cgi.empty())
    {
        std::cout << "CGI Configurations:\n";
        for (const auto& cgiPair : route.cgi)
        {
            std::cout << "  Extension: " << cgiPair.first
                      << ", Interpreter: " << cgiPair.second << "\n";
        }
    }
}

RouterResult Router::handleRequest(HttpRequest& request, const Listener& listener, const std::string& remoteAddr)
{
    // (void)listener;
    RouterResult routerResult;
    RequestMatchResult matchResult;
    HttpResponse response;
    if (request.host.empty())
    {
        // std::cout << "no host name"  << "\n";
        routerResult.decision = DES_ERROR;
        routerResult.response.statusCode = static_cast<HTTP_StatusCode>(400);
        return routerResult;
    }
    const Server* selectedServer = listener.defaultServer;
    selectedServer = findServerForRequest(listener, request);
    if (selectedServer)
    {
        std::cout << "Routing to server with port "
                << selectedServer->getListenPort() << "\n";
    }
    const routeConfig* bestMatchRouteConfig = NULL;
    serverConfig selectedServerConfig = selectedServer->getConfig();

    size_t max_uri_len = 0;
    for (const routeConfig& route : selectedServer->getConfig().routes)
    {
        if (matchRouteWithConfig(request.uri_path, route.path))
        {
            if (route.path.size() > max_uri_len)
            {
                max_uri_len = route.path.size();
                bestMatchRouteConfig = &route;
            }
        }   
    }
    if (bestMatchRouteConfig == NULL)
    {
        // std::cout << "error ar matching uri "  "\n";
        routerResult.decision = DES_ERROR;
        routerResult.response.statusCode = static_cast<HTTP_StatusCode>(404);
        return routerResult;
    }
    printRouteInfo(*bestMatchRouteConfig);
    std::string methodStringed = methodToString(request.method);
    std::cout << "methodStringed: " << methodStringed << "\n";
    if (bestMatchRouteConfig->isRedirect == true)
    {
        routerResult.decision = DES_REDIRECT;
        routerResult.response.statusCode = static_cast<HTTP_StatusCode>(bestMatchRouteConfig->redirectCode);
        routerResult.response.headers["Location"] = bestMatchRouteConfig->redirectTarget;
        return routerResult;
    }
    if (std::find(bestMatchRouteConfig->httpMethods.begin(), bestMatchRouteConfig->httpMethods.end(), methodStringed)
         == bestMatchRouteConfig->httpMethods.end())
    {
    // std::cout << "no method match." << "\n";
    routerResult.decision = DES_ERROR;
    routerResult.response.statusCode = static_cast<HTTP_StatusCode>(405);
    return routerResult;
    }
    matchResult.selectedServer = selectedServer;
    matchResult.bestMatchRouteConfig = bestMatchRouteConfig;
    matchResult.stringifiedMethod = methodStringed;
    matchResult.remoteAddress = remoteAddr;
    matchResult.selectedServerCon = selectedServerConfig;
    
    std::string extensionFromRequest = extractExtensionFromUri(request);
    const std::map<std::string, std::string>::const_iterator cgiIter = bestMatchRouteConfig->cgi.find(extensionFromRequest);

    if (cgiIter != bestMatchRouteConfig->cgi.end())
    {
        matchResult.interpreter = cgiIter->second;
        routerResult.decision = DES_CGI;
        routerResult.matchResult = matchResult;
        routerResult.routeConfigure = bestMatchRouteConfig;
        return routerResult;
    }
    if (extensionFromRequest.empty())
    {
        std::string filePath = bestMatchRouteConfig->rootDir
                            + request.uri_path.substr(bestMatchRouteConfig->path.size());
        std::cout << "DEBUG stat check: [" << filePath << "]\n";
        struct stat st;
        if (stat(filePath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode) && (st.st_mode & S_IXUSR))
        {
            std::cout << "DEBUG file is executable → DES_CGI\n";
            matchResult.interpreter = "";
            routerResult.decision = DES_CGI;
            routerResult.matchResult = matchResult;
            routerResult.routeConfigure = bestMatchRouteConfig;
            return routerResult;
        }
        std::cout << "DEBUG stat failed or not executable\n";
    }
    routerResult.decision = DES_NORMAL;
    routerResult.matchResult = matchResult;
    routerResult.routeConfigure = bestMatchRouteConfig;
    return routerResult;
}
