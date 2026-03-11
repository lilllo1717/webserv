// HTTP request arrives
//         ↓
// router detects .php CGI
//         ↓
// build environment variables
//         ↓
// create pipes
//         ↓
// fork()
//         ↓
// child: execve CGI interpreter
//         ↓
// parent: send body through stdin pipe
//         ↓
// CGI program runs script
//         ↓
// CGI writes headers + body to stdout
//         ↓
// server reads stdout
//         ↓
// parse CGI output
//         ↓
// construct HTTP response
//         ↓
// send response to client

#include "Cgi.hpp"

CgiHandler::CgiHandler(const HttpRequest& request, const routeConfig& config, const RequestMatchResult& configResult)
    :   _childPid(-1),
        _stdInPipe{-1, -1},
        _stdOutPipe{-1, -1},
        _envStrings(),
        _envp(),
        _request(request),
        _config(config),
        _configResult(configResult)
{
    std::cout << "CgiHandler Constructor called.\n";
}

CgiHandler::~CgiHandler()
{
    if (_stdInPipe[0] != -1)
        close(_stdInPipe[0]);
    if (_stdInPipe[1] != -1)
        close(_stdInPipe[1]);
    if (_stdOutPipe[0] != -1)
        close(_stdOutPipe[0]);
    if (_stdOutPipe[1] != -1)
        close(_stdOutPipe[1]);
    std::cout << "CgiHandler Destructor called.\n";
}

// HttpResponse CgiHandler::executeCgi()
// {
//     buildEnvVars();

// }
