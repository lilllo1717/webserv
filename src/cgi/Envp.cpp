#include "Cgi.hpp"
#include "../http/Http.hpp"


void CgiHandler::executeCgi()
{
    std::cout << "entrred executeCgi" << "\n";
    buildEnvVars();
    // return 
}

std::string getServerName(std::string host)
{
    auto it = host.find(":");
    return host.substr(0, it);
}

void CgiHandler::buildEnvVars()
{
    std::cout << "entrred buildEnvVars" << "\n";


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
    _envStrings.push_back("REMOTE_HOST=" + _configResult.remoteAddress);
    _envStrings.push_back("SERVER_NAME=" + getServerName(_request.host));
    _envStrings.push_back("SERVER_PORT=" + std::to_string(_configResult.selectedServerCon.endpoint.port));
    _envStrings.push_back("SERVER_PROTOCOL=HTTP/1.1");
    _envStrings.push_back("SERVER_SOFTWARE=webserv/1.0");
    _envStrings.push_back("SCRIPT_NAME=" + _request.uri_path);

    std::string scriptPath = _request.uri_path;
    if (scriptPath.find(_config.path) == 0)
        scriptPath = scriptPath.substr(_config.path.size());
    _envStrings.push_back("SCRIPT_FILENAME=" + _config.rootDir + scriptPath);

    for (const auto& header: _request.headers)
    {
        std::string envKey = "HTTP_";
        for (char c : header.first)
        {
            if (c == '-')
                envKey += '_';
            else
             envKey+= std::toupper(c);
        }
        _envStrings.push_back(envKey + "=" + header.second);
    }

    for (auto envvar: _envStrings)
    {
        std::cout << envvar << "\n";
    }
}
