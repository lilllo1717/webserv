#include "Http.hpp"

// if chunked -> bodytype chunked -> parse state body
// else if content len -> bodytypelen 
// else no body -> parsing done


static std::vector<std::string> splitTransferEncodingValue(std::string valString)
{
    std::vector<std::string> result;
    size_t start = 0;
    while (true)
    {
        auto comma_pos = valString.find(',', start);
        std::string word;
        if (comma_pos == std::string::npos)
        {
            word = valString.substr(start);
            trim(word);
            result.push_back(word);
            break;
        }
        word = valString.substr(start, comma_pos);
        trim(word);
        result.push_back(word);
        start = comma_pos + 1;
    }
    return result;
}

static BodyType establishBodyType(HttpRequest& request)
{
    auto it = request.headers.find("transfer-encoding");
    if (it != request.headers.end())
    {
        std::string temp = it->second;
        std::vector<std::string> split_values = splitTransferEncodingValue(temp);
        bool has_chunk = false;

        for (size_t i = 0; i < split_values.size(); i++)
        {
            trim(split_values[i]);
            if (split_values[i] == "chunked")
            {
                std::cout << "Wehave chunked body!" << "\n";
                has_chunk = true;
            }
        }
        if (has_chunk == true)
            return CHUNKED;
        else
            return UNSUPPORTED;
    }
    auto it2 = request.headers.find("content-length");
    if (it2 != request.headers.end())
    {
        int res_value = 0;
        std::string& num_str = it2->second;
        auto res = std::from_chars(num_str.data(), num_str.data() + num_str.size(), res_value);
        if (res.ec == std::errc() || res.ptr !=  num_str.data() + num_str.size() || res_value < 0)
            return UNSUPPORTED;
        request.contentLength = res_value;
        std::cout << "Content-Length: " << request.headers["content-length"] << "\n";
        request.parseState = BODY;
        return CONTENT_LENGTH;
    }
    return NONE;
}



ParseResult HttpRequestParser::parse(HttpRequest& request)
{
    while (true)
    {
        if (request.parseState == REQ_LINE)
        {
            ParseResult parseRes = parseRequestLine(request);
            if (parseRes == PARSE_ERROR || parseRes == PARSE_IN_PROGRESS)
            {
                std::cout << "error after request line" << "\n";
                return parseRes;
            }
            // if (parseRes != PARSE_DONE)
            //     return parseRes;
            request.parseState = HEADERS;
            continue;

        }
        else if (request.parseState == HEADERS)
        {
            std::cout << "entered headers parsing" << "\n";

            ParseResult parseRes = parseHeader(request);
            if (parseRes == PARSE_ERROR || parseRes == PARSE_IN_PROGRESS)
                return parseRes;
            BodyType typeRes = establishBodyType(request);
            if (typeRes == NONE)
                return PARSE_DONE;
            else if (typeRes == UNSUPPORTED)
                return PARSE_ERROR;
            else
                request.parseState = BODY;
            continue;
        }
        else if (request.parseState == BODY)
        {
            std::cout << "entered body parsing" << "\n";

        }
    }
    return PARSE_DONE;
};
