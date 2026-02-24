#include "Http.hpp"

static ParseResult parseLengthBody(HttpRequest& request)
{
    size_t body_len = request.contentLength;
    if (request.buffer.size() < body_len)
    {
        std::cout << "error here" << "\n";
        return PARSE_IN_PROGRESS;
    }
    request.body.assign(request.buffer.begin(), request.buffer.begin() + body_len);
    request.buffer.erase(0, body_len);
    std::cout << "request.body: [";
    for (size_t i = 0; i < request.body.size(); i++)
    {
        std::cout << static_cast<char>(request.body[i]);
    }
    std::cout << "]\n";
    return PARSE_DONE;
}

ParseResult HttpRequestParser::parseBody(HttpRequest& request)
{
    std::cout << "ParseBody func entered." << "\n";
    while (true)
    {
        std::cout << "request.bodyType: " << request.bodyType << "\n";

        if (request.bodyType == CONTENT_LENGTH)
        {
            std::cout << "body type condition." << "\n";

            if (parseLengthBody(request) == PARSE_DONE)
            {
                request.parseState = DONE;
                return PARSE_DONE;
            }
            continue;   
        }
    }
    // if (request.bodyType == CHUNKED)
    //     parseChunkedBody(request);
    return PARSE_DONE;

}