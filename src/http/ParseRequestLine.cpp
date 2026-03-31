#include "Http.hpp"


bool is_hex(char c)
{
    return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}
// TODO: validate that the path does not go beyond the root


ParseResult HttpRequestParser::validateUri(std::string &uri)
{
    // invalid:
    //     ASCII < 32
    //     ASCII = 127
    //     space
    //     NULL byte

    //     % not followed by exactly 2 hex digits

    for (size_t i; i < uri.size(); i++)
    {
        unsigned char c = static_cast<unsigned char>(uri[i]);

        if (c == ' ')
            return PARSE_ERROR;
        if (c < 32 || c == 127)
            return PARSE_ERROR;
        if (!is_hex(uri[i+1]) || !is_hex(uri[i + 2]))
            return PARSE_ERROR;
    }
    return PARSE_DONE;

}


ParseResult HttpRequestParser::parseUriChunk(std::string& requestLine, Cursor& cursor, HttpRequest& request)
{
    // size_t end_schema;
    
    size_t start_pos = cursor.get_position();
    size_t end_uri = start_pos;
    size_t start_uri = start_pos;
    size_t start_query = start_pos;
    size_t end_query = start_pos;
    size_t end_unparsed_uri = start_pos;
    size_t start_schema=start_pos;
    bool has_query = false;

    if ( start_pos < requestLine.size() && requestLine[start_pos] == '/')
    {
        start_uri = start_pos;
        // return requestLine.substr(start_uri, requestLine.size());

    }

    else if (requestLine.compare(start_pos, 7, "http://") == 0)
    {
        cursor.pos += 7;
        start_schema = start_pos;
        while (cursor.peek_char() != '/' && !cursor.check_eof())
            cursor.pos++;
        start_uri = cursor.get_position();
        // std::cout << "end_schema : [" << requestLine.substr(start_uri, end_uri - start_uri) << "]\n";
        // return requestLine.substr(start_schema, end_schema - start_schema);  
    }
    else
        return PARSE_ERROR;
    while (cursor.peek_char() != '?' && cursor.peek_char() != ' ' && !cursor.check_eof())
    {
        cursor.pos++;
    }
    end_uri = cursor.get_position();
    if (cursor.peek_char() == '?')
    {
        has_query = true;
        start_query = cursor.get_position() + 1;
        while (cursor.peek_char() != ' ' && !cursor.check_eof())
        {
            cursor.pos++;
        }
        end_query = cursor.get_position();
    }
    // std::cout << "char at  end_uri: " << requestLine[ end_uri]  << "\n";
    if (has_query == true)
        end_unparsed_uri = end_query;
    else
        end_unparsed_uri = end_uri;
    request.unparsed_uri = requestLine.substr(start_schema, end_unparsed_uri - start_schema);
    request.uri_path = requestLine.substr(start_uri, end_uri - start_uri);
    std::cout << "request.uri_path: [" << request.uri_path << "]\n";
    if (validateUri(request.uri_path) == PARSE_ERROR)
    {
        std::cout << "validateUri(request.uri_path) returned error" << "\n";

        return PARSE_ERROR;
    }
    request.uri_query = requestLine.substr(start_query, end_query - start_query);
    if (validateUri(request.uri_query) == PARSE_ERROR)
    {
        std::cout << "validateUri(request.uri_query) returned error" << "\n";
        return PARSE_ERROR;
    }
    // end_schema = cursor.get_position();
    // cursor.pos += 1;

    std::cout << "unparsed_uri: [" << request.unparsed_uri << "]\n";
    std::cout << "uri_path: [" << request.uri_path << "]\n";
    std::cout << "uri_query: [" << request.uri_query << "]\n";
    cursor.skip_spaces();
    // std::cout << "char at HTTP: [" << requestLine[cursor.get_position()]  << "]\n";
    if (requestLine.compare(cursor.get_position(), 8, "HTTP/1.1") != 0)
        return PARSE_ERROR;
    // request.parseState = HEADERS;
    return PARSE_DONE;
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
    Cursor cursor(requestLine);
    std::cout << "parseRawRequestLine -> requestLine: " << requestLine << "\n";
    // auto pos = requestLine.find(' ');
    // if (pos == std::string::npos)
    //     return PARSE_ERROR;
    // std::string methodChunk = requestLine.substr(0, pos);
    cursor.skip_spaces();
    request.method = parseMethodChunk(requestLine, cursor);
    // std::cout << "request method: " << request.method << "\n";
    if (request.method == HTTP_UNKNOWN)
        return PARSE_ERROR;
    cursor.skip_spaces();
    if (parseUriChunk(requestLine, cursor, request) == PARSE_ERROR)
        return PARSE_ERROR;
    cursor.skip_spaces();
    return PARSE_DONE;

};

ParseResult HttpRequestParser::parseRequestLine(HttpRequest& request)
{
    auto pos = request.buffer.find("\r\n");
    std::cout << "parseRequestLine -> request.requestLine: " << request.requestLine << "\n";
    std::cout << "parseRequestLine -> request.buffer: " << request.buffer << "\n";


    if (pos == std::string::npos)
    {
        // request.requestLine.append(request.buffer);
        std::cout << "parseRequestLine2 -> request.requestLine: " << request.requestLine << "\n";

        std::cout << "no rn" << "\n";
        return PARSE_IN_PROGRESS;
        // break;
    }
    request.requestLine = request.buffer.substr(0, pos);

    std::cout << "request.requestLine: [" << request.requestLine << "]\n";
    if (parseRawRequestLine(request.requestLine, request) == PARSE_ERROR)
        return PARSE_ERROR;
    request.buffer.erase(0, pos + 2);
    return PARSE_DONE;
};
