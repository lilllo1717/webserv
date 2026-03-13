// HTTP request arrives
//         ↓
// router detects .php CGI
//         ↓
// build environment variables -> done
//         ↓
// create pipes -> done
//         ↓
// fork() -> done
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

void CgiHandler::closePipes()
{
    close(_stdInPipe[0]);
    close(_stdInPipe[1]);
    close(_stdOutPipe[0]);
    close(_stdOutPipe[1]);
}

CgiHandler::CgiHandler(const HttpRequest& request, const routeConfig& config, const RequestMatchResult& configResult)
    :   _childPid(-1),
        _stdInPipe{-1, -1},
        _stdOutPipe{-1, -1},
        _envStrings(),
        _envp(),
        _scriptPath(""),
        _interpreter(""),
        _request(request),
        _config(config),
        _configResult(configResult)
  
{
    _interpreter = configResult.interpreter;
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


HttpResponse CgiHandler::executeCgi()
{
    try
    {
        std::cout << "entrred executeCgi" << "\n";
        buildEnvVars();
        if (!createPipes())
            return constructResponse(500);
        if (!createChildProcess())
            return constructResponse(500);
    }
    catch(const std::exception& e)
    {
        std::cerr << "CGI error." << e.what() << '\n';
        return constructResponse(500);
    }
    
    return constructResponse(200);

}

bool CgiHandler::createPipes()
{
    std::cout << "piping" << "\n";

    if (pipe(_stdInPipe) < 0)
    {
        std::cerr << "Failed to create stdin pipe: " << strerror(errno) << "\n";
        return false;
    }

    if (pipe(_stdOutPipe) < 0)
    {
        close(_stdInPipe[0]);
        close(_stdInPipe[1]);
        _stdInPipe[0] = -1;
        _stdOutPipe[1] = -1;
        std::cerr << "Failed to create stdout pipe: " << strerror(errno) << "\n";
        return false;
    }
    return true;
}

bool CgiHandler::executeChild()
{
    std::cout << "executing child" << "\n";
    close(_stdInPipe[1]);
    close(_stdOutPipe[0]);
    if (dup2(_stdInPipe[0], STDIN_FILENO) == -1)
    {
        std::cerr << "du2 IN failed: " << strerror(errno) << "\n";
        closePipes();
        exit(1);
        
    }

    if (dup2(_stdOutPipe[1], STDOUT_FILENO) == -1)
    {
        std::cerr << "du2 OUT failed: " << strerror(errno) << "\n";
        closePipes();
        exit(1); 
    }
    close(_stdInPipe[0]);
    close(_stdOutPipe[1]);
    char* argv[] = {
    const_cast<char*>(_interpreter.c_str()),
    const_cast<char*>(_scriptPath.c_str()),
    nullptr
    };
    // char* interp = const_cast<char*>(_interpreter.c_str());
    execve(_interpreter.c_str(), argv, _envp.data());
    std::cerr << "execve failed: " << strerror(errno) << "\n";
    closePipes();
    exit(1);

}

bool CgiHandler::createChildProcess()
{
    std::cout << "fokring child" << "\n";

    pid_t pid = fork();

    if (pid < 0)
    {
        std::cerr << "Failed to fork: " << strerror(errno) << "\n";
        return false;
    }
    else if (pid == 0)
    {
        std::cout << "In child process.\n";
        executeChild();
    }
    else
    {
        _childPid = pid;
        // executeParent();
    }
    return true;
}