#include "Http.hpp"


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
            return PARSE_DONE;
        }
        // else if (request.parseState == BODY)
        // {
        // }
    }
    return PARSE_DONE;
};
