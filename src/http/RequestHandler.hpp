#ifndef REQUEST_HANDLER_HPP
# define REQUEST_HANDLER_HPP

# include "Http.hpp"

class RequestHandler
{
	public:
		static HttpResponse	executeNormal(const HttpRequest& request, const routeConfig& route);
	
	private:
		static std::string	resolvePath(const HttpRequest& request, const routeConfig& route);
		static bool			isDirectory(const std::string& path);
		static std::string	resolveIndexFile(const std::string& dirPath, const routeConfig& route);
		static HttpResponse buildAutoindex(const std::string& path, const std::string& uriPath);
		static HttpResponse	serveStaticFile(const std::string& path);

		static HttpResponse	handleGET(const HttpRequest& request, const routeConfig& route);
		static HttpResponse handlePOST(const HttpRequest& request, const routeConfig& route);
		static HttpResponse	handleDELETE(const HttpRequest& request, const routeConfig& route);
		static HttpResponse buildErrorResponse(HTTP_StatusCode code, const serverConfig& config);

};

#endif