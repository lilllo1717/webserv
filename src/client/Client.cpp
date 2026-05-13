#include "Client.hpp"


Client::Client()
    :   _socketFd(-1),
        _bytesSent(0),
        _bytesReceived(0),
        // _receiveBuffer(""),
        _bufferLimit(4096),
        _sendBuffer(""),
        _sendLimit(4096),
        _httpRequest(),
        _httpResponse(),
        _cgiState(nullptr)
{
    std::cout << "Client default Constructor called.\n";
}

Client::Client(int socketFd)
    :   _socketFd(socketFd),
        _bytesSent(0),
        _bytesReceived(0),
        // _receiveBuffer(""),
        _bufferLimit(4096),
        _sendBuffer(""),
        _sendLimit(4096),
        _httpRequest(),
        _httpResponse(),
        _cgiState(nullptr)

{
    std::cout << "Client Constructor called.\n";
}

Client& Client::operator=(const Client& other)
{
    if (this != &other) {
        _socketFd = other._socketFd;
        _bytesSent = other._bytesSent;
        _bytesReceived = other._bytesReceived;
        // _receiveBuffer = other._receiveBuffer;
        _bufferLimit = other._bufferLimit;
        _sendBuffer = other._sendBuffer;
        _sendLimit = other._sendLimit;
        _httpRequest = other._httpRequest;
        _httpResponse = other._httpResponse;
        
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

CgiState* Client::getCgiState()
{
    return _cgiState;
}

void Client::initializeCgiState()
{
    if (_cgiState == nullptr)
        _cgiState = new CgiState();
}

void Client::cleanupCgiState()
{
    if (_cgiState != nullptr)
    {
        delete _cgiState;
        _cgiState = nullptr;
    }
}

void Client::addBytesSent(size_t bytes)
{
    _bytesSent += bytes;
}

size_t Client::getBytesSent() const
{
    return _bytesSent;
}

// void Client::parseHttpRequest()
// {
//     while (true)
//     {
//         if (_httpRequest.parseState == REQ_LINE)
//         {
//             auto pos = _httpRequest.buffer.find("\r\n");
//             if (pos == std::string::npos)
//             {
//                 _httpRequest.parseResult = PARSE_IN_PROGRESS;
//                 break;
//             }
//             std::string requestLine = _httpRequest.buffer.substr(0, pos);

//             //parse line
//             //erase in buffer
//             //change state

//         }
//         else if (_httpRequest.parseState == HEADERS)
//         {

//         }
//         else if (_httpRequest.parseState == BODY)
//         {

//         }
//     }
// }

void Client::addBytesReceived(ssize_t bytes)
{
    _bytesReceived += bytes;
}

ssize_t Client::getBytesReceived() const
{
    return _bytesReceived;
}

// void Client::appendToReceiveBuffer(const std::string& data)
// {
//     _receiveBuffer += data;
//     _bytesReceived += data.size();
// }
// const std::string& Client::getReceiveBuffer() const
// {
//     return _receiveBuffer;
// }

void Client::appendToSendBuffer(const std::string& data)
{
    _sendBuffer += data;
    _bytesSent += data.size();
    
}

const std::string& Client::getSendBuffer() const
{
    return _sendBuffer;
}

CgiState* Client::getCgiState()
{
    return _cgiState;
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

void Client::clearBuffer(size_t bytes)
{
    if (bytes >= _sendBuffer.size())
    {
        _sendBuffer.clear();
    }
    else
    {
        _sendBuffer.erase(0, bytes);
    }
}

HttpRequest& Client::getHttpRequest()
{
    return _httpRequest;
}

HttpResponse& Client::getHttpResponse()
{
    return _httpResponse;
}
