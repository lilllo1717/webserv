#ifndef CGI_HPP
#define CGI_HPP

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <iomanip>
#include <map>
#include <unordered_map>
#include <memory>
#include <vector>
#include <string>
#include <string_view>
#include <cctype>
#include <algorithm>
#include <charconv>
// #include "../manager/Manager.hpp"
// #include "webserv.hpp"
#include "../http/Http.hpp"
#include "../configParser/parser.hpp"

struct RequestMatchResult;


class CgiHandler
{
    private:
        pid_t   _childPid;
        int     _stdInPipe[2];
        int     _stdOutPipe[2];

        std::vector<std::string>    _envStrings;
        std::vector<char*> _envp;
        std::string _scriptPath;
        std::string _interpreter;
        std::string _cgiOutput;

        const HttpRequest&   _request;
        const HttpResponse&   _response;
        const routeConfig&  _config;
        const RequestMatchResult& _configResult;

    public:
        CgiHandler() = delete;
        CgiHandler(const HttpRequest& request, const HttpResponse& response, const routeConfig& config, const RequestMatchResult& configResult);
        virtual ~CgiHandler();
        CgiHandler& operator=(const CgiHandler&) = delete;


        HttpResponse executeCgi();

    private:
        void buildEnvVars();
        bool createPipes();
        bool createChildProcess();
        bool executeChild();
        bool executeParent();
        bool parseCgiOutputIntoHttpResponse();
        void closePipes();

};



#endif
