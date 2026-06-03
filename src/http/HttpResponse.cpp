#include "Http.hpp"
#include "../server/Server.hpp"
#include "../cgi/Cgi.hpp"
#include "RequestHandler.hpp"

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
    
    if (requestUri.size() > configUri.size() && requestUri[configUri.size()] == '/')
    {
        std::cout << "match, longer uri" << "\n";
        return true;
    }
   
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
    size_t lastSlash = request.uri_path.rfind("/");
    size_t lastDot = request.uri_path.rfind(".");

    if (lastDot == std::string::npos || lastDot < lastSlash || lastDot == lastSlash + 1)
        return "";

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

RouterResult Router::handleRequest(HttpRequest& request, const Listener& listener, const std::string& remoteAddr)
{
    RouterResult routerResult;
    RequestMatchResult matchResult;
    HttpResponse response;
    if (request.host.empty())
    {
        routerResult.decision = DES_ERROR;
        routerResult.response.statusCode = static_cast<HTTP_StatusCode>(400);
        return routerResult;
    }
    const Server* selectedServer = listener.defaultServer;
    selectedServer = findServerForRequest(listener, request);
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

        routerResult.decision = DES_ERROR;
        routerResult.response.statusCode = static_cast<HTTP_StatusCode>(404);
        return routerResult;
    }
    std::string methodStringed = methodToString(request.method);
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
        struct stat st;
        if (stat(filePath.c_str(), &st) == 0 && !S_ISDIR(st.st_mode) && (st.st_mode & S_IXUSR))
        {
            matchResult.interpreter = "";
            routerResult.decision = DES_CGI;
            routerResult.matchResult = matchResult;
            routerResult.routeConfigure = bestMatchRouteConfig;
            return routerResult;
        }
    }
    routerResult.decision = DES_NORMAL;
    routerResult.matchResult = matchResult;
    routerResult.routeConfigure = bestMatchRouteConfig;
    return routerResult;
}
