#include "Http.hpp"

std::string HttpRequestParser::parseUriChunk(std::string& requestLine, Cursor& cursor)
{
    size_t start_uri;
    // size_t  end_uri;

    size_t start_pos = cursor.get_position();
    if (requestLine.compare(start_pos, 7, "http://") == 0)
    {
        cursor.pos+=7;
        char c = cursor.get_char();
        while (c != '/')
            c = cursor.get_char();
        start_uri = cursor.get_position() - 1;
    }
    else if (requestLine[start_pos] == '/')
    {
        start_uri = start_pos;
    }
    
    char c = cursor.get_char();
    std::cout << "c : [" << c << "]\n";
    std::cout << "start_uri : [" << start_uri << "]\n";


    while (c != ' ')
    {
        c = cursor.get_char();
    }
    size_t uri_len = cursor.get_position() - start_uri - 1;

    std::cout << "uri: [" << requestLine.substr(start_uri, uri_len) << "]\n";
    return requestLine.substr(start_uri, uri_len);
};

HTTP_Method HttpRequestParser::parseMethodChunk(std::string& requestLine, Cursor& cursor)
{
    size_t  start = cursor.get_position();

    while (!cursor.check_eof())
    {
        char c = cursor.peek_char();

        if (c == ' ')
            break;
        if (!std::isupper(c))
            return HTTP_UNKNOWN;
        cursor.get_char();
    }

    size_t len = cursor.get_position() - start;

    if (len == 3)
    {
        if (requestLine.compare(start, len, "GET") == 0)
            return HTTP_GET;
    }
    else if (len == 4)
    {
        if (requestLine.compare(start, len, "POST") == 0)
            return HTTP_POST;
    }
    else if (len == 6)
    {
        if (requestLine.compare(start, len, "DELETE") == 0)
            return HTTP_DELETE;
    }

    return HTTP_UNKNOWN;
};

ParseResult HttpRequestParser::parseRawRequestLine(std::string& requestLine, HttpRequest& request)
{
    Cursor cursor(request.requestLine);
    // auto pos = requestLine.find(' ');
    // if (pos == std::string::npos)
    //     return PARSE_ERROR;
    // std::string methodChunk = requestLine.substr(0, pos);
    cursor.skip_spaces();
    request.method = parseMethodChunk(requestLine, cursor);
    std::cout << "request method: " << request.method << "\n";
    if (request.method == HTTP_UNKNOWN)
        return PARSE_ERROR;
    cursor.skip_spaces();
    request.unparsed_uri = parseUriChunk(requestLine, cursor);
    cursor.skip_spaces();
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
        request.requestLine = request.buffer.substr(0, pos);
        if (parseRawRequestLine(request.requestLine, request) == PARSE_ERROR)
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
