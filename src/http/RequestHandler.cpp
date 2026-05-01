#include "RequestHandler.hpp"

HttpResponse RequestHandler::buildErrorResponse(HTTP_StatusCode code, const serverConfig& config)
{
    HttpResponse response;
    response.statusCode = code;

    int codeInt = static_cast<int>(code);
    auto it = config.errorPages.find(codeInt);
    if (it != config.errorPages.end())
    {
        return RequestHandler::serveStaticFile(it->second);
    }

    std::string phrase(reasonPhrase(code));
    std::string body = "<html><body><h1>" + phrase + "</h1></body></html>";
    response.body.assign(body.begin(), body.end());
    response.headers["Content-Type"] = "text/html";
    response.headers["Content-Length"] = std::to_string(response.body.size());
    return response;
}

// Converts URI (filesystem path)
std::string	RequestHandler::resolvePath(const HttpRequest& request, const routeConfig& route)
{
	 std::string relative = request.uri_path;

    // Remove location prefix
    if (relative.find(route.path) == 0)
        relative = relative.substr(route.path.length());

    if (relative.empty() || relative== "/")
        return route.rootDir;

    return route.rootDir + "/" + relative;
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

	std::string fullPath = dirPath;

	if (!fullPath.empty() && fullPath.back() == '/')
		fullPath.pop_back();

	fullPath += "/" + route.index;

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

std::string	getMimeType(const std::string& path)
{
	if (path.find(".jpg") != std::string::npos)
		return "image/jpeg";
	if (path.find(".png") != std::string::npos)
		return "image/png";
	if (path.find(".gif") != std::string::npos)
		return "image/gif";
	if (path.find(".html") != std::string::npos)
		return "text/html";

	return "application/octet-stream";
}

HttpResponse	RequestHandler::serveStaticFile(const std::string& path)
{
	HttpResponse	response;
	serverConfig	config;
	std::ifstream file(path.c_str(), std::ios::binary);

	if (!file)
	{
		std::cout << "File not found!\n";
		// response.statusCode = static_cast<HTTP_StatusCode>(404);
		// return constructResponse(response);
		return buildErrorResponse(static_cast<HTTP_StatusCode>(404), config);

	}
	
	std::vector<uint8_t> fileData(
		(std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>()
	);
	
	response.statusCode = HTTP_StatusCode::OK;
	response.body = std::move(fileData);

	std::cout << "serveStaticFile path: [" << path << "]\n";
	std::cout << "fileData size: " << fileData.size() << "\n";

	response.headers["Content-Length"] = std::to_string(response.body.size());
	response.headers["Content-Type"] = getMimeType(path);

	return response;
}

HttpResponse	RequestHandler::handleGET(const HttpRequest& request, const routeConfig& route, const serverConfig& config)
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
		
		// response.statusCode = static_cast<HTTP_StatusCode>(403);
		// return constructResponse(response);
		return buildErrorResponse(static_cast<HTTP_StatusCode>(403), config);
	}
	return serveStaticFile(path);
}

// Add this helper above handlePOST
static std::string extensionFromContentType(const std::string& contentType)
{
    if (contentType.find("image/jpeg") != std::string::npos) return ".jpg";
    if (contentType.find("image/png")  != std::string::npos) return ".png";
    if (contentType.find("image/gif")  != std::string::npos) return ".gif";
    if (contentType.find("text/html")  != std::string::npos) return ".html";
    if (contentType.find("text/plain") != std::string::npos) return ".txt";
    return ".bin";
}

HttpResponse RequestHandler::handlePOST(const HttpRequest& request, const routeConfig& route, const serverConfig& config)
{
    HttpResponse response;

    if (route.uploadPath.empty())
    {
        response.statusCode = static_cast<HTTP_StatusCode>(403);
        return constructResponse(response);
    }

    // Try to get filename from URL first (e.g. POST /upload/cat.jpg)
    size_t pos = request.uri_path.find_last_of('/');
    std::string filename = request.uri_path.substr(pos + 1);

    // If URI gave us just the location name (e.g. "upload"), it has no extension.
    // Fall back to generating one from Content-Type.
    if (filename.empty() || filename.find('.') == std::string::npos)
    {
        auto ct = request.headers.find("content-type");
        std::string ext = (ct != request.headers.end())
            ? extensionFromContentType(ct->second)
            : ".bin";
        if (filename.empty())
            filename = "upload" + ext;
        else
            filename += ext;        // e.g. "upload" → "upload.jpg"
    }

    std::string fullPath = route.uploadPath + "/" + filename;
    std::ofstream out(fullPath.c_str(), std::ios::binary);
    if (!out)
        return buildErrorResponse(static_cast<HTTP_StatusCode>(403), config);

    out.write(reinterpret_cast<const char*>(request.body.data()), request.body.size());
    out.close();

    // Tell the client exactly where the file was stored
    response.statusCode = static_cast<HTTP_StatusCode>(201);
    response.headers["Content-Length"] = "0";
    response.headers["Location"] = "/uploads/" + filename;
    return constructResponse(response);
}

// HttpResponse	RequestHandler::handlePOST(const HttpRequest& request, const routeConfig& route, const serverConfig& config)
// {
// 	HttpResponse response;

// 	std::cout << "=== HANDLE POST ===\n";
//     std::cout << "route.uploadPath = [" << route.uploadPath << "]\n";
//     std::cout << "route.rootDir    = [" << route.rootDir << "]\n";
//     std::cout << "request.uri_path = [" << request.uri_path << "]\n";
//     std::cout << "body size        = [" << request.body.size() << "]\n";

// 	if (route.uploadPath.empty())
// 	{
// 		response.statusCode = static_cast<HTTP_StatusCode>(403);
// 		return constructResponse(response);
// 	}

// 	size_t	pos = request.uri_path.find_last_of('/');
// 	std::string	filename = request.uri_path.substr(pos + 1);

// 	if (filename.empty())
// 		filename = "upload.bin"; // fallback incase file is empty 

// 	std::string fullPath = route.uploadPath + "/" + filename;

// 	std::ofstream out(fullPath.c_str(), std::ios::binary);
// 	if (!out)
// 	{
// 		// response.statusCode = static_cast<HTTP_StatusCode>(403);
// 		// return constructResponse(response);
// 		return buildErrorResponse(static_cast<HTTP_StatusCode>(403), config);
// 	}

// 	out.write(reinterpret_cast<const char*>(request.body.data()), request.body.size());
// 	out.close();

// 	response.statusCode = static_cast<HTTP_StatusCode>(201);
// 	response.headers["Content-Length"] = "0";

// 	return constructResponse(response);
// }

HttpResponse	RequestHandler::handleDELETE(const HttpRequest& request, const routeConfig& route, const serverConfig& config)
{
	HttpResponse	response;
	std::string		path = resolvePath(request, route);

	// Allow DELETE in upload directory
	if (route.rootDir != "./uploads")
	{
    	response.statusCode = static_cast<HTTP_StatusCode>(403);
   		return constructResponse(response);
	}

	struct stat	s;
	if (stat(path.c_str(), &s) != 0)
	{
		// response.statusCode = static_cast<HTTP_StatusCode>(404);
		// return constructResponse(response);
		return buildErrorResponse(static_cast<HTTP_StatusCode>(404), config);
	}
	if (S_ISDIR(s.st_mode))
	{
		// response.statusCode = static_cast<HTTP_StatusCode>(403);
		// return constructResponse(response);
		return buildErrorResponse(static_cast<HTTP_StatusCode>(403), config);
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

HttpResponse	RequestHandler::executeNormal(const HttpRequest& request, const routeConfig& route, const serverConfig& config)
{
	if (request.method == HTTP_GET)
		return handleGET(request, route, config);
	
	if (request.method == HTTP_POST)
		return handlePOST(request, route, config);

	if (request.method == HTTP_DELETE)
		return handleDELETE(request, route, config);

	HttpResponse	response;
	response.statusCode = static_cast<HTTP_StatusCode>(405);
	return constructResponse(response);
}