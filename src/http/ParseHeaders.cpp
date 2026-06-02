#include "Http.hpp"


void trim(std::string& s)
{
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t'))
        start++;

    if (start == s.size()) 
    {
        s.clear();
        return;
    }

    size_t end = s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t'))
        end--;

    s = s.substr(start, end - start);
}

void lowerLettersInHeaders(std::string& header)
{
    std::transform(header.begin(), header.end(), header.begin(), ::tolower);
}

ParseResult HttpRequestParser::parseSingleHeader(std::string &buffer, HttpRequest& request)
{
    
    auto col_pos = buffer.find(':');
    if (col_pos == std::string::npos)
    {
        return PARSE_ERROR;
    }
    // buffer.erase(0, col_pos + 1);
    std::string headers_key = buffer.substr(0, col_pos);
    std::string headers_val = buffer.substr(col_pos + 1);
    trim(headers_key);
    trim(headers_val);
    lowerLettersInHeaders(headers_key);
    if (headers_key == "host")
        request.host = headers_val;
    if (headers_key == "content-type")
        request.contentType = headers_val;
    if (headers_key == "connection")
    {
        std::string val = headers_val;
        lowerLettersInHeaders(val);
        request.keepAlive = (val.find("keep-alive") != std::string::npos);
    }
    request.headers[headers_key] = headers_val;
    std::cout << "!![" << headers_key << "]" << ":" << headers_val << "\n";
    return PARSE_DONE;
}

ParseResult HttpRequestParser::parseHeader(HttpRequest& request)
{
    while (true)
    {
        auto pos = request.buffer.find("\r\n");
        if (pos == std::string::npos)
            return PARSE_IN_PROGRESS;
        if (pos == 0)
        {
            request.buffer.erase(0, 2);
            return PARSE_DONE;
        }
        std::string string_to_check = request.buffer.substr(0, pos);
        // std::cout << "string_to_check: " << string_to_check << "\n";
        if (parseSingleHeader(string_to_check, request) == PARSE_ERROR)
            return PARSE_ERROR;
        request.buffer.erase(0, pos + 2);
    }
    return PARSE_DONE;
};

