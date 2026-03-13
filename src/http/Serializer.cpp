#include "Http.hpp"
#include "../server/Server.hpp"
#include "../cgi/Cgi.hpp"

std::string	serializeHttpResponse(const HttpResponse& response)
{
	std::ostringstream	stream;

	stream	<< "HTTP/1.1 "
			<< static_cast<int>(response.statusCode)
			<< " "
			<< reasonPhrase(response.statusCode)
			<< "\r\n";

	for (std::map<std::string, std::string>::const_iterator it = response.headers.begin();
			it != response.headers.end();
			++it)
	{
		stream << it->first << ": " << it->second << "\r\n";
	}

	if (response.closeConnection)
		stream << "Connection: close\r\n";
	else
		stream << "Connection: keep-alive\r\n";

	stream << "\r\n";

	std::string headerPart = stream.str();

	std::string result;
	result.reserve(headerPart.size() + response.body.size());

	result += headerPart;
	result.insert(result.end(), response.body.begin(), response.body.end());

	return (result);
}
