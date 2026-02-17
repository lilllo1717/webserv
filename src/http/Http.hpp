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

enum ParseResult
{
    PARSE_DONE,
    PARSE_IN_PROGRESS,
    PARSE_ERROR
};

enum HTTP_Method
{
    HTTP_GET,
    HTTP_POST,
    HTTP_DELETE,
    HTTP_UNKNOWN
};

struct HttpRequest
{
    uint8_t httpMajor = 1;
    uint8_t httpMinor = 1;

    /*    --------  Request line  ---------   */

    /*     GET[SP]/index.html[SP]HTTP/1.1\r\n                              */
    /*     Request-Line   = Method SP Request-URI SP HTTP-Version CRLF     */
    /*     POST /cgi-bin/upload.py?user=tanja HTTP/1.1                     */

    enum HTTP_Method method = HTTP_UNKNOWN;                          // GET POST DELETE
    std::string unparsed_uri;                          // "/path%2F..../?a=1"
    std::string uri_path;                        // parsed path
    std::string uri_query;                       // part after '?'

    /*    --------   Headers  ---------   */
    std::string host;                           // "www.example.com"
    std::map<std::string, std::string> headers; // headers["user_agent"] = "Mozilla/5.0"
    bool keepAlive = false;
    std::string contentType;

    /*    --------   Body  ---------   */
    std::vector<uint8_t> body;
    std::size_t contentLength = 0;
    enum BodyType bodyType = BodyType::NONE;

    /*    --------   Parsing  ---------   */
    enum ParseState parseState = REQ_LINE;
    enum ParseResult parseResult = PARSE_IN_PROGRESS;
    std::string buffer;

    /*    --------   Chunks  ---------   */
    std::size_t chunkRemainingSize = 0; // for chunked transfer encoding, size of the current chunk being processed
    enum ChunkState chunkState = CHUNK_SIZE;



};

struct HttpResponse
{
    /*    --------  Status line  ---------   */ // HTTP/1.1 200 OK
    int statusCode = 200;                            // 200, 404, etc.
    
    /*    --------   Headers  ---------   */
    std::map<std::string, std::string> headers;

    /*    --------   Body  ---------   */
    std::vector<uint8_t> body;

    bool closeConnection = false;

};

class HttpRequestParser
{
    public:
        static ParseResult parse(HttpRequest& request);

    private:
        static ParseResult parseRequestLine(HttpRequest& request);
        static ParseResult parseRawRequestLine(std::string& requestLine, HttpRequest& request);
        static ParseResult parseHeader(HttpRequest& request);
        static ParseResult parseBody(HttpRequest& request);
        static HTTP_Method parseMethodChunk(const std::string& methodChunk);


};


#endif