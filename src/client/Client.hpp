
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <iomanip>
#include <map>

class Client
{
    private:
        int         _socketFd;
        size_t      _bytesSent;
        size_t      _bytesReceived;
        std::string _receiveBuffer;
        size_t      _bufferLimit;
        std::string _sendBuffer;
        size_t      _sendLimit;

    public:
        Client();
        Client(int socketFd);
        virtual ~Client();

        void setSocketFd(int socketFd);
        int getSocketFd() const;

        void addBytesSent(size_t bytes);
        size_t getBytesSent() const;
        
        void addBytesReceived(size_t bytes);
        size_t getBytesReceived() const;

        void appendToReceiveBuffer(const std::string& data);
        const std::string& getReceiveBuffer() const;

        void appendToSendBuffer(const std::string& data);
        const std::string& getSendBuffer() const;

        void setBufferLimit(size_t limit);
        size_t getBufferLimit() const;

        void setSendLimit(size_t limit);
        size_t getSendLimit() const;

};