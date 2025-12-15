#include "Client.hpp"


Client::Client()
    :   _socketFd(-1),
        _bytesSent(0),
        _bytesReceived(0),
        _receiveBuffer(""),
        _bufferLimit(4096),
        _sendBuffer(""),
        _sendLimit(4096)
{
    std::cout << "Client default Constructor called.\n";
}

Client::Client(int socketFd)
    :   _socketFd(socketFd),
        _bytesSent(0),
        _bytesReceived(0),
        _receiveBuffer(""),
        _bufferLimit(4096),
        _sendBuffer(""),
        _sendLimit(4096)
{
    std::cout << "Client Constructor called.\n";
}

Client& Client::operator=(const Client& other)
{
    if (this != &other) {
        _socketFd = other._socketFd;
        _bytesSent = other._bytesSent;
        _bytesReceived = other._bytesReceived;
        _receiveBuffer = other._receiveBuffer;
        _bufferLimit = other._bufferLimit;
        _sendBuffer = other._sendBuffer;
        _sendLimit = other._sendLimit;
    }
    return *this;
}
Client::~Client()
{
    std::cout << "Client Destructor called.\n";
}

void Client::setSocketFd(int socketFd)
{
    _socketFd = socketFd;
}

int Client::getSocketFd() const
{
    return _socketFd;
}

void Client::addBytesSent(size_t bytes)
{
    _bytesSent += bytes;
}

size_t Client::getBytesSent() const
{
    return _bytesSent;
}

void Client::addBytesReceived(size_t bytes)
{
    _bytesReceived += bytes;
}

size_t Client::getBytesReceived() const
{
    return _bytesReceived;
}

void Client::appendToReceiveBuffer(const std::string& data)
{
    _receiveBuffer += data;
}
const std::string& Client::getReceiveBuffer() const
{
    return _receiveBuffer;
}

void Client::appendToSendBuffer(const std::string& data)
{
    _sendBuffer += data;
}

const std::string& Client::getSendBuffer() const
{
    return _sendBuffer;
}

void Client::setBufferLimit(size_t limit)
{
    _bufferLimit = limit;
}

size_t Client::getBufferLimit() const
{
    return _bufferLimit;
}

void Client::setSendLimit(size_t limit)
{
    _sendLimit = limit;
}

size_t Client::getSendLimit() const
{
    return _sendLimit;
}
