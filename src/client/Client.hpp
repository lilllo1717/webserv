#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <stdio.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <iostream>
# include <iomanip>
# include <map>
# include "../http/Http.hpp"
# include "../cgi/CgiState.hpp"



class Client
{
    private:
        int         _socketFd;
        size_t      _bytesSent;
        ssize_t      _bytesReceived;
        // std::string _receiveBuffer;
        ssize_t      _bufferLimit;
        std::string _sendBuffer;
        size_t      _sendLimit;
        struct HttpRequest _httpRequest;
        struct HttpResponse _httpResponse;
        CgiState* _cgiState;

    public:
        Client();
        Client(int socketFd);
        Client& operator=(const Client& other);
        virtual ~Client();

        void setSocketFd(int socketFd);
        int getSocketFd() const;
        CgiState* getCgiState();
        void initializeCgiState(CgiState& state);
        void cleanupCgiState();

        void addBytesSent(size_t bytes);
        void addBytesReceived(ssize_t bytes);
        size_t getBytesSent() const;
        
        // void addBytesReceived(size_t bytes);
        ssize_t getBytesReceived() const;

        // void appendToReceiveBuffer(const std::string& data);
        // const std::string& getReceiveBuffer() const;

        void appendToSendBuffer(const std::string& data);
        const std::string& getSendBuffer() const;

        void setBufferLimit(size_t limit);
        size_t getBufferLimit() const;

        void setSendLimit(size_t limit);
        size_t getSendLimit() const;

        void clearBuffer(size_t bytes);

        HttpRequest& getHttpRequest();
        HttpResponse& getHttpResponse();

        // void    parseHttpRequest();

};

#endif