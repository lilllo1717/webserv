#include "Http.hpp"

void trim(std::string& header)
{
    size_t start = 0;
    size_t end = header.size();

    while (start < header.size() && (header[start] == ' ' || header[start] == '\t'))
        start++;
    while (end > start && (header[end - 1] == ' ' || header[end - 1] == '\t'))
        end--;

    header.erase(0, start);
    header.erase(end);
}


static void lowerLettersInHeaders(std::string& header)
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
    std::string headers_key = buffer.substr(0, col_pos);
    std::string headers_val = buffer.substr(col_pos + 1);
    trim(headers_key);
    lowerLettersInHeaders(headers_key);
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
        std::cout << "parse header: string_to_check: " << string_to_check << "\n";

        if (parseSingleHeader(string_to_check, request) == PARSE_ERROR)
            return PARSE_ERROR;
        request.buffer.erase(0, pos + 2);
    }
    return PARSE_DONE;
};
