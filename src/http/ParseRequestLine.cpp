#include "Http.hpp"


bool is_hex(char c)
{
    return ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
}

int hexCharToInt(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

// Two hex chars to one byte using bit shifting
char hexToChar(char high, char low)
{
    return (hexCharToInt(high) << 4) | hexCharToInt(low);
}

bool validateUriChars(std::string& uri)
{
    for (size_t i = 0; i < uri.size(); i++)
    {
        unsigned char c = static_cast<unsigned char>(uri[i]);

        if (c == ' ')
            return false;
        if (c < 32 || c == 127)
            return false;

        if (c == '%')
        {
            if (i + 2 >= uri.size())
                return false;

            if (!is_hex(uri[i + 1]) || !is_hex(uri[i + 2]))
                return false;
            unsigned char decoded = static_cast<unsigned char>(
                hexToChar(uri[i + 1], uri[i + 2]));
            if (decoded < 32 || decoded == 127)
                return false;
            i += 2;
        }
    }
    return true;
}

std::string decodeUri(std::string& uri)
{
    std::string decoded_uri;

    for (size_t i = 0; i < uri.size(); i++)
    {
        if (uri[i] == '%' && i + 2 < uri.size())
        {
            char converted = hexToChar(uri[i+ 1], uri[i + 2]);
            decoded_uri += converted;
            i += 2;
        }
        else if (uri[i] == '+')
            decoded_uri += ' ';
        else
        {
            decoded_uri += uri[i];
        }
    }
    uri = decoded_uri;
    return decoded_uri;
}

std::string normalizeString(std::string &uri)
{
    std::string normalized_str;
    std::stack<std::string>  stack_str;
    std::istringstream stream(uri);
    std::string segment;

    while (std::getline(stream, segment, '/'))
    {
        if (segment.empty() || segment == ".")
            continue;
        else if (segment == "..")
        {
            if (!stack_str.empty())
                stack_str.pop();
        }
        else
            stack_str.push(segment);
    }
    while (!stack_str.empty())
    {
        normalized_str = "/" + stack_str.top() + normalized_str;
        stack_str.pop();        
    }
    if (normalized_str.empty())
        normalized_str = "/";
    std::cout << "DEBUG: normalized_str: [" << normalized_str << "]\n";
    return normalized_str; 
}



ParseResult HttpRequestParser::validateUri(std::string &uri)
{
    std::string  normalized_str;
    std::string decoded_string;
    std::cout << "DEBUG: validateUri called with uri: [" << uri << "]\n";
    if (!validateUriChars(uri))
        return PARSE_ERROR;
    decoded_string = decodeUri(uri);
    if (decoded_string.find('%') != std::string::npos)
    {
        std::cout << "DEBUG: decoded_string contains %" << "\n";
        return PARSE_ERROR;
    }
    normalized_str = normalizeString(decoded_string);
    uri = normalized_str;   
    return PARSE_DONE;
}


ParseResult HttpRequestParser::parseUriChunk(std::string& requestLine, Cursor& cursor, HttpRequest& request)
{

    size_t start_pos = cursor.get_position();
    size_t end_uri = start_pos;
    size_t start_uri = start_pos;
    size_t start_query = start_pos;
    size_t end_query = start_pos;
    size_t end_unparsed_uri = start_pos;
    size_t start_schema=start_pos;
    bool has_query = false;

    std::cout << "DEBUG: parseUriChunk called with requestLine: [" << requestLine << "]\n";
    std::cout << "DEBUG: start parseUriChunk at position: " << start_pos << "\n";
    std::cout << "DEBUG: char at start: [" << requestLine[start_pos]  << "]\n";

    if (start_pos < requestLine.size() && requestLine[start_pos] == '/')
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
    {
        std::cout << "DEBUG: URI does not start with '/' or 'http://'" << "\n";
        request.parseState = ERROR;
        request.parseResult = PARSE_ERROR;
        return PARSE_ERROR;
    }
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
    std::cout << "char at  end_uri: " << requestLine[end_uri]  << "\n";
    if (has_query == true)
        end_unparsed_uri = end_query;
    else
        end_unparsed_uri = end_uri;
    request.unparsed_uri = requestLine.substr(start_schema, end_unparsed_uri - start_schema);
    request.uri_path = requestLine.substr(start_uri, end_uri - start_uri);
    std::cout << "request.uri_path: [" << request.uri_path << "]\n";
    if (request.uri_path.size() > 8192)
    {
        request.uri_too_long = true;
        request.parseState = ERROR;
        return PARSE_ERROR;
    }
    if (validateUri(request.uri_path) == PARSE_ERROR)
    {
        std::cout << "validateUri(request.uri_path) returned error" << "\n";
        request.parseState = ERROR;
        return PARSE_ERROR;
    }
    request.uri_query = requestLine.substr(start_query, end_query - start_query);
    if (validateUriChars(request.uri_query) == false)
    {
        std::cout << "validateUriChars(request.uri_query) returned error" << "\n";
        request.parseState = ERROR;
        return PARSE_ERROR;
    }
    // end_schema = cursor.get_position();
    // cursor.pos += 1;

    // std::cout << "unparsed_uri: [" << request.unparsed_uri << "]\n";
    std::cout << "DEBUG: uri_path: [" << request.uri_path << "]\n";
    // std::cout << "uri_query: [" << request.uri_query << "]\n";
    cursor.skip_spaces();
    // std::cout << "char at HTTP: [" << requestLine[cursor.get_position()]  << "]\n";
    if (requestLine.compare(cursor.get_position(), 8, "HTTP/1.1") != 0)
    {
        request.parseState = ERROR;
        return PARSE_ERROR;
    }
    // request.parseState = HEADERS;
    return PARSE_DONE;
};

HTTP_Method HttpRequestParser::parseMethodChunk(std::string& requestLine, Cursor& cursor)
{
    size_t  start = cursor.get_position();
    // std::cout << "DEBUG: start parseMethodChunk at position: " << start << "\n";
    // std::cout << "DEBUG: char at start: [" << requestLine[start]  << "]\n";

    while (!cursor.check_eof())
    {
        char c = cursor.peek_char();
        // std::cout << "DEBUG: char in parseMethodChunk: [" << c  << "]\n";
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
    // cursor.skip_spaces();
    request.method = parseMethodChunk(requestLine, cursor);
    // std::cout << "request method: " << request.method << "\n";
    if (request.method == HTTP_UNKNOWN)
    {
        request.parseState = ERROR;
        return PARSE_ERROR;
    }
    cursor.skip_spaces();
    if (parseUriChunk(requestLine, cursor, request) == PARSE_ERROR)
        return PARSE_ERROR;
    cursor.skip_spaces();
    return PARSE_DONE;
};

ParseResult HttpRequestParser::parseRequestLine(HttpRequest& request)
{
    auto pos = request.buffer.find("\r\n");
    // std::cout << "buffer: [" << request.buffer << "] at pos: " << pos << "\n";
    if (pos == std::string::npos)
        return PARSE_IN_PROGRESS;
    request.requestLine = request.buffer.substr(0, pos);
    // std::cout << "request.requestLine: [" << request.requestLine << "]\n";
    if (parseRawRequestLine(request.requestLine, request) == PARSE_ERROR)
        return PARSE_ERROR;
    request.buffer.erase(0, pos + 2);
    return PARSE_DONE;
};
