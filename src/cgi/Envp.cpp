#include "Cgi.hpp"
#include "../http/Http.hpp"


void CgiHandler::executeCgi()
{
    buildEnvVars();
    // return 
}

void CgiHandler::buildEnvVars()
{
    _envStrings.push_back("REQUEST_METHOD=" + _configResult.stringifiedMethod);
    _envStrings.push_back("CONTENT_LENGTH=" + std::to_string(_request.contentLength));
    if (_request.contentType != "")
        _envStrings.push_back("CONTENT_TYPE=" + _request.contentType);
    else
        _envStrings.push_back("CONTENT_TYPE=");
    _envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
    if (_request.uri_query != "")
        _envStrings.push_back("QUERY_STRING=" + _request.uri_query);
    else
        _envStrings.push_back("QUERY_STRING=");
    _envStrings.push_back("PATH_INFO=");
    _envStrings.push_back("PATH_TRANSLATED=");
    _envStrings.push_back("REMOTE_ADDR=" + _configResult.remoteAddress);


    


}