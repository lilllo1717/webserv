#ifndef CGI_STATE_HPP
#define CGI_STATE_HPP

#include <string>
#include <unistd.h>

struct CgiState
{
    pid_t childPid;
    int     stdInFd;
    int     stdoutFd;
    size_t  bodyWritten;
    std::string output;
    bool stdInDone;
    bool    done;

    CgiState() :
        childPid(-1),
        stdInFd(-1),
        stdoutFd(-1),
        bodyWritten(0),
        output(""),
        stdInDone(false),
        done(false)
        {}

};

#endif