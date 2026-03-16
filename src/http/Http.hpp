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
#include <string>
#include <string_view>
#include <cctype>
#include <algorithm>
#include <charconv>
// #include "../manager/Manager.hpp"
#include "webserv.hpp"



enum BodyType
{
    NONE,
    CHUNKED,
    CONTENT_LENGTH,
    UNSUPPORTED
};

enum ChunkState
{
    CHUNK_SIZE,
    CHUNK_SIZE_CRLF,
    CHUNK_DATA,
    CHUNK_DATA_CRLF,
    CHUNK_TRAILERS,
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

enum class HTTP_StatusCode : int
{
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,

    MOVED_PERMANENTLY = 301,
    FOUND = 302,

    BAD_REQUEST = 400,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    PAYLOAD_TOO_LARGE = 413,
    URI_TOO_LONG = 414,
    UNPROCESSABLE_ENTITY = 422,

    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    GATEWAY_TIMEOUT = 504
    
};



struct RequestMatchResult
{
    const Server* selectedServer = nullptr;
    const routeConfig* bestMatchRouteConfig = nullptr;
    std::string stringifiedMethod;
    std::string remoteAddress;
    serverConfig selectedServerCon;
    std::string interpreter;

};


struct HttpRequest
{
    uint8_t httpMajor = 1;
    uint8_t httpMinor = 1;

    /*    --------  Request line  ---------   */

    /*     GET[SP]/index.html[SP]HTTP/1.1\r\n                                           */
    /*     Request-Line   = Method SP Request-URI SP HTTP-Version CRLF                  */
    /*     POST /cgi-bin/upload.py?user=tanja HTTP/1.1                                  */
    /*     http_URL = "http:" "//" host [ ":" port ] [ abs_path [ "?" query ]]          */
    /*     Request-URI    = "*" | absoluteURI | abs_path | authority                    */

    std::string requestLine;
    enum HTTP_Method method = HTTP_UNKNOWN; // GET POST DELETE
    std::string unparsed_uri;               // "/path%2F..../?a=1"
    std::string uri_path;                   // parsed path
    std::string uri_query;                  // part after '?'

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
    std::string sizeBuffer;
    // std::string body;
    enum ChunkState chunkState = CHUNK_SIZE;

    /*    --------   Trailers  ---------   */
    std::map<std::string, std::string> trailers;
};

struct HttpResponse
{
    /*    --------  Status line  ---------   */ // HTTP/1.1 200 OK
    /*   Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF   */
    HTTP_StatusCode statusCode = HTTP_StatusCode::OK;                      // 200, 404, etc.

    /*    --------   Headers  ---------   */
    std::map<std::string, std::string> headers;

    /*    --------   Body  ---------   */
    std::vector<uint8_t> body;

    bool closeConnection = false;
};

class HttpRequestParser
{
public:
    static ParseResult parse(HttpRequest &request);

    struct Cursor
    {
        std::size_t pos;
        std::string_view str_view;

        Cursor(std::string_view str)
        {
            pos = 0;
            str_view = str;
        }

        bool check_eof() const
        {
            if (pos >= str_view.size())
                return true;
            return false;
        }

        char peek_char() const
        {
            if (check_eof())
                return '\0';
            return str_view[pos];
        }

        char get_char()
        {
            if (check_eof())
                return '\0';
            char c = str_view[pos];
            pos++;
            return c;
        }

        void skip_spaces()
        {
            while (!check_eof() && str_view[pos] == ' ')
                pos++;
        }

        std::size_t get_position()
        {
            return pos;
        }
    };

    private:
        static ParseResult parseRequestLine(HttpRequest &request);
        static ParseResult parseRawRequestLine(std::string &requestLine, HttpRequest &request);
        static ParseResult parseHeader(HttpRequest &request);
        static HTTP_Method parseMethodChunk(std::string &requestLine, Cursor &cursor);
        static ParseResult parseUriChunk(std::string &requestLine, Cursor &cursor, HttpRequest &request);
        static ParseResult validateUri(std::string &uri);
        static ParseResult parseSingleHeader(std::string &buffer, HttpRequest& request);
        static ParseResult parseBody(HttpRequest& request);
    
};

class Router
{
    public:
        HttpResponse handleRequest(HttpRequest& request, const Listener& listener, const std::string& remoteAddr);

};


void trim(std::string& header);
bool is_hex(char c);
void lowerLettersInHeaders(std::string& header);
HttpResponse constructResponse(HttpResponse& response);



constexpr std::string_view reasonPhrase(HTTP_StatusCode status)
{
    switch (status)
    {
        case HTTP_StatusCode::OK: return "OK";
        case HTTP_StatusCode::CREATED: return "Created";
        case HTTP_StatusCode::NO_CONTENT: return "No Content";
        case HTTP_StatusCode::MOVED_PERMANENTLY: return "Moved Permanently";
        case HTTP_StatusCode::FOUND: return "Found";
        case HTTP_StatusCode::BAD_REQUEST: return "Bad Request";
        case HTTP_StatusCode::FORBIDDEN: return "Forbidden";
        case HTTP_StatusCode::NOT_FOUND: return "Not Found";
        case HTTP_StatusCode::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HTTP_StatusCode::PAYLOAD_TOO_LARGE: return "Payload Too Large";
        case HTTP_StatusCode::URI_TOO_LONG: return "URI Too Long";
        case HTTP_StatusCode::UNPROCESSABLE_ENTITY: return "Unprocessable Entity";
        case HTTP_StatusCode::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HTTP_StatusCode::NOT_IMPLEMENTED: return "Not Implemented";
        case HTTP_StatusCode::BAD_GATEWAY: return "Bad Gateway";
        case HTTP_StatusCode::GATEWAY_TIMEOUT: return "Gateway Timeout";
        default: return "Unknown";
    }
};

std::string	serializeHttpResponse(const HttpResponse& response);

#endif
