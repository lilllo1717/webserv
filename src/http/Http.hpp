#ifndef HTTP_HPP
#define HTTP_HPP

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

enum BodyType
{
    NONE,
    CHUNKED,
    CONTENT_LENGTH
};

enum ChunkState
{
    CHUNK_SIZE,
    CHUNK_DATA,
    CHUNK_CRLF,
    CHUNK_DONE
};

enum ParseState
{
    REQ_LINE,
    HEADERS,
    BODY,
    DONE,
    ERROR
};

struct HttpRequest
{

    /*    --------  Request line  ---------   */ // POST /cgi-bin/upload.py?user=tanja HTTP/1.1
    std::string method;                          // GET POST DELETE
    std::string uriRaw;                          // "/path%2F..../?a=1"
    std::string uri_path;                        // parsed path
    std::string uri_query;                       // part after '?'

    /*    --------   Headers  ---------   */
    std::string host;                           // "www.example.com"
    std::map<std::string, std::string> headers; // headers["User-Agent:"] = "Mozilla/5.0"
    bool keepAlive = true;
    std::string contentType;

    /*    --------   Body  ---------   */
    std::vector<uint8_t> body;
    std::size_t contentLength = 0;
    enum BodyType bodyType = BodyType::NONE;
    enum ParseState parseState = REQ_LINE;

    std::string buffer;

    std::size_t chunkRemainingSize = 0; // for chunked transfer encoding, size of the current chunk being processed
    enum ChunkState chunkState = CHUNK_SIZE;


};


struct HttpResponse
{
    /*    --------  Status line  ---------   */ // HTTP/1.1 200 OK
    int statusCode;                            // 200, 404, etc.
    
    /*    --------   Headers  ---------   */
    std::map<std::string, std::string> headers;

    /*    --------   Body  ---------   */
    std::vector<uint8_t> body;

    bool closeConnection = false;

};


#endif