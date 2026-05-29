#include "Cgi.hpp"
#include "../http/Http.hpp"


// HttpResponse CgiHandler::executeCgi()
// {
//     try
//     {
//         std::cout << "entrred executeCgi" << "\n";
//         buildEnvVars();
//     }
//     catch(const std::exception& e)
//     {
//         std::cerr << e.what() << '\n';
//         return constructResponse(500);
//     }
    
//     // return 
// }

std::string getServerName(std::string host)
{
    auto it = host.find(":");
    return host.substr(0, it);
}

// void CgiHandler::buildEnvVars()
// {
//     _envStrings.push_back("REQUEST_METHOD=" + _configResult.stringifiedMethod);
//     _envStrings.push_back("CONTENT_LENGTH=" + std::to_string(_request.contentLength));
//     if (_request.contentType != "")
//         _envStrings.push_back("CONTENT_TYPE=" + _request.contentType);
//     else
//         _envStrings.push_back("CONTENT_TYPE=");
//     _envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
//     if (_request.uri_query != "")
//         _envStrings.push_back("QUERY_STRING=" + _request.uri_query);
//     else
//         _envStrings.push_back("QUERY_STRING=");

//     _envStrings.push_back("REDIRECT_STATUS=200");
//     _envStrings.push_back("REMOTE_ADDR=" + _configResult.remoteAddress);
//     _envStrings.push_back("REMOTE_HOST=" + _configResult.remoteAddress);
//     _envStrings.push_back("SERVER_NAME=" + getServerName(_request.host));
//     _envStrings.push_back("SERVER_PORT=" + std::to_string(_configResult.selectedServerCon.endpoint.port));
//     _envStrings.push_back("SERVER_PROTOCOL=HTTP/1.1");
//     _envStrings.push_back("SERVER_SOFTWARE=webserv/1.0");
//     _envStrings.push_back("SCRIPT_NAME=" + _request.uri_path);
//     _envStrings.push_back("REQUEST_URI=" + _request.unparsed_uri);
//     // std::cout << "SCRIPT_NAME=" << _request.uri_path << "\n";


//     _scriptPath = _request.uri_path;
//     if (_scriptPath.find(_config.path) == 0)
//         _scriptPath = _scriptPath.substr(_config.path.size());

//     _scriptPath = _config.rootDir + _scriptPath;
//     _envStrings.push_back("SCRIPT_FILENAME=" + _scriptPath);
//     std::cout << "!!!SCRIPT_FILENAME=" << _scriptPath << "\n";

//     std::string pathInfo = _request.uri_path.substr(_config.path.size());
//     // size_t scriptEnd = pathInfo.find('/', 1);
//     // if (scriptEnd != std::string::npos)
//     //     pathInfo = pathInfo.substr(scriptEnd);
//     // else
//     //     pathInfo = "";
//     _envStrings.push_back("PATH_INFO=" + _request.unparsed_uri);
//     _envStrings.push_back("PATH_TRANSLATED=" + _scriptPath);

//     for (const auto& header: _request.headers)
//     {
//         std::string envKey = "HTTP_";
//         for (char c : header.first)
//         {
//             if (c == '-')
//                 envKey += '_';
//             else
//              envKey+= std::toupper(c);
//         }
//         _envStrings.push_back(envKey + "=" + header.second);
//     }

//     for (auto envvar: _envStrings)
//     {
//         std::cout << envvar << "\n";
//     }

//     for (std::string& envvar : _envStrings)
//         _envp.push_back(envvar.data());
//     _envp.push_back(nullptr);
// }

void CgiHandler::buildEnvVars()
{
    std::cout << "entrred buildEnvVars" << "\n";

    // Strip location prefix from uri_path to get script + optional extra path
    // e.g. uri_path=/world/worldclock.py, config.path=/world → /worldclock.py
    std::string scriptAndExtra = _request.uri_path.substr(_config.path.size());

    // Find where script ends and PATH_INFO begins (first '/' after position 0)
    // e.g. /worldclock.py/extra/stuff → scriptEnd points to /extra/stuff
    size_t scriptEnd = scriptAndExtra.find('/', 1);
    std::string scriptOnly  = scriptAndExtra.substr(0, scriptEnd); // /worldclock.py
    std::string pathInfo    = (scriptEnd != std::string::npos) ? scriptAndExtra.substr(scriptEnd) : "";

    _scriptPath = _config.rootDir + scriptOnly;

    _envStrings.push_back("REQUEST_METHOD="    + _configResult.stringifiedMethod);
    _envStrings.push_back("CONTENT_LENGTH="    + std::to_string(_request.contentLength));
    if (_request.contentType != "")
        _envStrings.push_back("CONTENT_TYPE="  + _request.contentType);
    else
        _envStrings.push_back("CONTENT_TYPE=");
    _envStrings.push_back("GATEWAY_INTERFACE=CGI/1.1");
    if (_request.uri_query != "")
        _envStrings.push_back("QUERY_STRING="  + _request.uri_query);
    else
        _envStrings.push_back("QUERY_STRING=");
    _envStrings.push_back("REDIRECT_STATUS=200");
    _envStrings.push_back("REMOTE_ADDR="       + _configResult.remoteAddress);
    _envStrings.push_back("REMOTE_HOST="       + _configResult.remoteAddress);
    _envStrings.push_back("SERVER_NAME="       + getServerName(_request.host));
    _envStrings.push_back("SERVER_PORT="       + std::to_string(_configResult.selectedServerCon.endpoint.port));
    _envStrings.push_back("SERVER_PROTOCOL=HTTP/1.1");
    _envStrings.push_back("SERVER_SOFTWARE=webserv/1.0");
    _envStrings.push_back("SCRIPT_NAME="       + _config.path + scriptOnly);
    _envStrings.push_back("SCRIPT_FILENAME="   + _scriptPath);
    _envStrings.push_back("REQUEST_URI="       + _request.unparsed_uri);
    _envStrings.push_back("PATH_INFO="         + pathInfo);
    _envStrings.push_back("PATH_TRANSLATED="   + (pathInfo.empty() ? "" : _config.rootDir + pathInfo));

    for (const auto& header : _request.headers)
    {
        std::string envKey = "HTTP_";
        for (char c : header.first)
        {
            if (c == '-')
                envKey += '_';
            else
                envKey += std::toupper(c);
        }
        _envStrings.push_back(envKey + "=" + header.second);
    }

    for (auto envvar : _envStrings)
        std::cout << envvar << "\n";

    for (std::string& envvar : _envStrings)
        _envp.push_back(envvar.data());
    _envp.push_back(nullptr);
}