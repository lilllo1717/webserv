#include "Http.hpp"

HTTP_Method HttpRequestParser::parseMethodChunk(const std::string& methodChunk)
{
    if (methodChunk == "GET")
        return (HTTP_GET);
    else if (methodChunk == "POST")
        return (HTTP_POST);
    else if (methodChunk == "DELETE")
        return (HTTP_DELETE);
    else
        return (HTTP_UNKNOWN);
};

ParseResult HttpRequestParser::parseRawRequestLine(std::string& requestLine, HttpRequest& request)
{
    auto pos = requestLine.find(' ');
    if (pos == std::string::npos)
        return PARSE_ERROR;
    std::string methodChunk = requestLine.substr(0, pos);
    request.method = parseMethodChunk(methodChunk);
    if (request.method == HTTP_UNKNOWN)
        return PARSE_ERROR;
    return PARSE_DONE;

};


ParseResult HttpRequestParser::parseRequestLine(HttpRequest& request)
{
        auto pos = request.buffer.find("\r\n");
        if (pos == std::string::npos)
        {
            return PARSE_IN_PROGRESS;
            // break;
        }
        std::string requestLine = request.buffer.substr(0, pos);
        if (parseRawRequestLine(requestLine, request) == PARSE_ERROR)
            return PARSE_ERROR;
        request.buffer.erase(0, pos + 2);
        return PARSE_DONE;
};

// ParseResult HttpRequestParser::parseHeader(HttpRequest& request)
// {
    
// };

// ParseResult HttpRequestParser::parseBody(HttpRequest& request)
// {
    
// };

ParseResult HttpRequestParser::parse(HttpRequest& request)
{
    while (true)
    {
        if (request.parseState == REQ_LINE)
        {
            ParseResult parseRes = parseRequestLine(request);
            return parseRes;
            // if (parseRes != PARSE_DONE)
            //     return parseRes;
            // request.parseState = HEADERS;
            // continue;

        }
        // else if (request.parseState == HEADERS)
        // {

        // }
        // else if (request.parseState == BODY)
        // {

        // }
    }
};
