#include "RequestHandler.hpp"

// TODO: director file index, error pages, methods verification

// Converts URI (filesystem path)
std::string	RequestHandler::resolvePath(const HttpRequest& request, const routeConfig& route)
{
	if (!route.rootDir.empty())
		return route.rootDir + request.uri_path;
	return request.uri_path;
}

// Check if path is a directory
bool RequestHandler::isDirectory(const std::string& path)
{
	struct stat s;

	if (stat(path.c_str(), &s) != 0)
		return false;
	return S_ISDIR(s.st_mode);
}

// Handles index directive: index.html
std::string	RequestHandler::resolveIndexFile(const std::string& dirPath, const routeConfig& route)
{
	if (route.index.empty())
		return "";

	std::string fullPath = dirPath + "/" + route.index;

	std::ifstream file(fullPath.c_str());
	if (file.good())
		return fullPath;

	return "";
}

HttpResponse RequestHandler::buildAutoindex(const std::string& path, const std::string& uriPath)
{
	HttpResponse response;

	DIR* dir = opendir(path.c_str());
	if (!dir)
	{
		response.statusCode = static_cast<HTTP_StatusCode>(500);
		return constructResponse(response);
	}

	std::ostringstream html;

	html << "<html><body>";
	html << "<h1>Index of " << path << "</h1><ul>";

	std::string	base = uriPath;
	if (base[base.size() - 1] != '/')
		base += '/';
	
	struct dirent* entry;
	while((entry = readdir(dir)) != NULL)
	{
		if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..")
			continue;
			
		html	<< "<li><a href=\""
				<< base + entry->d_name
				<< "\">"
				<< entry->d_name
				<< "</a></li>";	
	}
	html << "</ul></body></html>";
	closedir(dir);

	std::string	body = html.str();

	response.statusCode = HTTP_StatusCode::OK;
	response.body.assign(body.begin(), body.end());
	response.headers["Content-Type"] = "text/html";
	response.headers["Content-Length"] = std::to_string(response.body.size());
	
	return response;
}

HttpResponse	RequestHandler::serveStaticFile(const std::string& path)
{
	HttpResponse	response;
	std::ifstream file(path.c_str(), std::ios::binary);

	if (!file)
	{
		std::cout << "File not found!\n";
		response.statusCode = static_cast<HTTP_StatusCode>(404);
		return constructResponse(response);
	}
	
	std::vector<uint8_t> fileData(
		(std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>()
	);
	
	response.statusCode = HTTP_StatusCode::OK;
	response.body = std::move(fileData);

	response.headers["Content-Length"] = std::to_string(response.body.size());
	response.headers["Content-Type"] = "text/html";

	return response;
}

HttpResponse	RequestHandler::handleGET(const HttpRequest& request, const routeConfig& route)
{
	HttpResponse response;
	std::string path = resolvePath(request, route);

	if (isDirectory(path))
	{
		std::string indexPath = resolveIndexFile(path, route);

		if (!indexPath.empty())
			return serveStaticFile(indexPath);
		
		if (route.autoindex)
		{
			std::string uri = request.uri_path;
			if (uri.empty() || uri[uri.size() - 1] != '/')
				uri += '/';
			return buildAutoindex(path, uri);
		}
		
		response.statusCode = static_cast<HTTP_StatusCode>(403);
		return constructResponse(response);
	}
	return serveStaticFile(path);

}

HttpResponse	RequestHandler::handlePOST(const HttpRequest& request, const routeConfig& route)
{
	HttpResponse response;

	if (route.uploadPath.empty())
	{
		response.statusCode = static_cast<HTTP_StatusCode>(403);
		return constructResponse(response);
	}

	std::string filename = "upload.txt";
	std::string fullPath = route.uploadPath + "/" + filename;

	std::ofstream out(fullPath.c_str(), std::ios::binary);
	if (!out)
	{
		response.statusCode = static_cast<HTTP_StatusCode>(403);
		return constructResponse(response);
	}

	out.write(reinterpret_cast<const char*>(request.body.data()), request.body.size());
	out.close();

	response.statusCode = static_cast<HTTP_StatusCode>(201);
	response.headers["Content-Length"] = "0";

	return constructResponse(response);
}

HttpResponse	RequestHandler::handleDELETE(const HttpRequest& request, const routeConfig& route)
{
	HttpResponse	response;
	std::string		path = resolvePath(request, route);

	struct stat	s;
	if (stat(path.c_str(), &s) != 0)
	{
		response.statusCode = static_cast<HTTP_StatusCode>(404);
		return constructResponse(response);
	}
	if (S_ISDIR(s.st_mode))
	{
		response.statusCode = static_cast<HTTP_StatusCode>(403);
		return constructResponse(response);
	}
	if (remove(path.c_str()) != 0)
	{
		response.statusCode = static_cast<HTTP_StatusCode>(500);
		return constructResponse(response);
	}

	response.statusCode = static_cast<HTTP_StatusCode>(204);
	response.headers["Content-Length"] = "0";

	return constructResponse(response);
}

HttpResponse	RequestHandler::executeNormal(const HttpRequest& request, const routeConfig& route)
{
	if (request.method == HTTP_GET)
		return handleGET(request, route);
	
	if (request.method == HTTP_POST)
		return handlePOST(request, route);

	if (request.method == HTTP_DELETE)
		return handleDELETE(request, route);

	HttpResponse	response;
	response.statusCode = static_cast<HTTP_StatusCode>(405);
	return constructResponse(response);
}