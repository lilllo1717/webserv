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
#include <sys/wait.h>

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
        _response(),
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

bool CgiHandler::parseOneCGIHeader(std::string& header_line)
{
    std::cout << "CGiI header_line : [" << header_line << "\n";
    auto col_pos = header_line.find(':');
    if (col_pos == std::string::npos)
        return false;
    std::string headers_key = header_line.substr(0, col_pos);
    std::string headers_val = header_line.substr(col_pos + 1);
    trim(headers_key);
    trim(headers_val);
    _response.headers[headers_key] = headers_val;
    std::cout << "CGI[" << headers_key << "]" << ":" << headers_val << "\n";
    return true;
}    // Convert entire body string → vector<uint8_t>

bool CgiHandler::parseCGIHeader(std::string& buffer)
{

    std::cout << "parsing CGI headers" << "\n";
    std::cout << "buffer:  [" <<  buffer << "]\n";
    while (!buffer.empty())
    {
        auto pos = buffer.find("\r\n");
        size_t to_add = 2;
        // std::cout << "pos " <<  pos << "\n";
        if (pos == std::string::npos)
        {
            pos = buffer.find("\n");
            to_add = 1;
            // std::cout << "pos " <<  pos << "\n";
            if (pos == std::string::npos)
                return parseOneCGIHeader(buffer);
                
        }

        std::string header_line = buffer.substr(0, pos);
        buffer.erase(0, pos + to_add);
        if (header_line.empty())
            return true;
        if (parseOneCGIHeader(header_line) == false)
            return false;
    }
    return true;
}

bool CgiHandler::parseCgiOutputIntoHttpResponse()
{
    std::string temp_sub_headers;
    std::string temp_body;
    size_t sep_len = 4;

    std::cout << "parsing body into response" << "\n";
    auto pos = _cgiOutput.find("\r\n\r\n");
    if (pos == std::string::npos)
    {
        pos = _cgiOutput.find("\n\n");
        sep_len = 2;
        if (pos == std::string::npos)
        {
            _response.body = std::vector<uint8_t>(_cgiOutput.begin(), _cgiOutput.end());
            return true;
        }
    }
    temp_sub_headers = _cgiOutput.substr(0, pos);
    size_t body_start = pos + sep_len;
    temp_body = _cgiOutput.substr(body_start);
    std::string headers_copy = temp_sub_headers;
    if (!parseCGIHeader(headers_copy)) {
        std::cerr << "Failed to parse CGI headers\n";
    }

    _response.body = std::vector<uint8_t>(temp_body.begin(), temp_body.end());
    _response.headers["Content-Length"] = std::to_string(_response.body.size());
    _response.closeConnection = true;
    std::cout << "CGI _cgiOutput: " << _cgiOutput << "\n";

    std::cout << "CGI headers: " << temp_sub_headers << "\n";
    std::cout << "CGI body: " << temp_body << "\n";

    return true;
}

HttpResponse CgiHandler::executeCgi()
{
    // HttpResponse response;
    try
    {
        std::cout << "entrred executeCgi" << "\n";
        buildEnvVars();
        struct stat s;
        if (stat(_scriptPath.c_str(), &s) != 0)
        {
            std::cerr << "CGI script not found: " << _scriptPath << "\n";
            _response.statusCode = static_cast<HTTP_StatusCode>(404);
            return _response;
        }
        if (!createPipes())
        {
            _response.statusCode = static_cast<HTTP_StatusCode>(500);
            return _response;
        }
        if (!createChildProcess())
        {
            _response.statusCode = static_cast<HTTP_StatusCode>(500);
            return _response;
        }
        if (!parseCgiOutputIntoHttpResponse())
        {
            _response.statusCode = static_cast<HTTP_StatusCode>(500);
            return _response;
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "CGI error." << e.what() << '\n';
        _response.statusCode = static_cast<HTTP_StatusCode>(500);
        return _response;
    }
    
    return _response;

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
        _stdInPipe[1] = -1;
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

bool CgiHandler::executeParent()
{
    std::cout << "executing parent" << "\n";
    close(_stdInPipe[0]);
    close(_stdOutPipe[1]);

    std::cout << "CGI parent request.body.size() = "
          << _request.body.size() << std::endl;

    std::cout << "CGI parent contentLength = "
            << _request.contentLength << std::endl;
    write(_stdInPipe[1], _request.body.data(), _request.body.size());
    close(_stdInPipe[1]);
    char temp_buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = read(_stdOutPipe[0], temp_buffer, sizeof(temp_buffer))) > 0)
        _cgiOutput.append(temp_buffer, bytes_read);
    // std::cout << "Bytes read from CGI: " << bytes_read << "\n";
    std::cout << "CGI output: " << _cgiOutput << "\n";
    close(_stdOutPipe[0]);
    waitpid(_childPid, nullptr, 0);
    return true;
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
        close(_stdInPipe[0]);
        close(_stdOutPipe[1]);
    }
    return true;
}