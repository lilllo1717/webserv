#include "Http.hpp"

static bool convertHexToSizeT(const std::string& s, size_t& out)
{
    out = 0;
    auto r = std::from_chars(s.data(), s.data() + s.size(), out, 16);
    return (r.ec == std::errc() && r.ptr == s.data() + s.size());
}

static ParseResult parseChunkSize(HttpRequest& request)
{
    if (request.buffer.empty())
        return PARSE_IN_PROGRESS;
    char c = request.buffer[0];
    if (is_hex(c))
    {
        request.sizeBuffer += c;
        request.buffer.erase(0, 1);
        return PARSE_IN_PROGRESS;
    }
    else if (c == '\r')
    {
        if (request.buffer.empty())
            return PARSE_ERROR;
        size_t converted;
        if (!convertHexToSizeT(request.sizeBuffer, converted))
            return PARSE_ERROR;
        request.chunkRemainingSize = converted;
        request.sizeBuffer.clear();
        request.buffer.erase(0, 1);
        request.chunkState = CHUNK_SIZE_CRLF;
        return PARSE_IN_PROGRESS;
    }
    else
        return PARSE_ERROR;
    return PARSE_IN_PROGRESS;
}

static ParseResult parseChunkSizeCRLF(HttpRequest& request)
{
    if (request.buffer.empty())
        return PARSE_IN_PROGRESS;
    if (request.buffer[0] != '\n')
        return PARSE_ERROR;
    request.buffer.erase(0, 1);
    if (request.chunkRemainingSize == 0)
        request.chunkState = CHUNK_TRAILERS;
    else
        request.chunkState = CHUNK_DATA;
    return PARSE_IN_PROGRESS;
}

static ParseResult parseChunkData(HttpRequest& request)
{
    if (request.chunkRemainingSize == 0)
        return PARSE_ERROR;
    if (request.buffer.empty())
        return PARSE_IN_PROGRESS;
    size_t toConsume = std::min(request.buffer.size(), request.chunkRemainingSize);
    request.body.insert(request.body.end(), request.buffer.begin(), request.buffer.begin() + toConsume);
    request.buffer.erase(0, toConsume);
    request.chunkRemainingSize -= toConsume;
    if (request.chunkRemainingSize == 0)
        request.chunkState = CHUNK_DATA_CRLF;
    return PARSE_IN_PROGRESS;
}

static ParseResult parseChunkDataCRLF(HttpRequest& request)
{
    if (request.buffer.size() < 2)
        return PARSE_IN_PROGRESS;
    if (request.buffer[0] != '\r' || request.buffer[1] != '\n')
        return PARSE_ERROR;
    request.buffer.erase(0, 2);
    request.chunkState = CHUNK_SIZE;
    return PARSE_IN_PROGRESS;
}

static ParseResult parseSingleTrailer(const std::string& headerLine, HttpRequest& request)
{
    auto col_pos = headerLine.find(':');
    if (col_pos == std::string::npos)
        return PARSE_ERROR;
    std::string trailers_key = headerLine.substr(0, col_pos);
    std::string trailers_val = headerLine.substr(col_pos + 1);
    trim(trailers_key);
    trim(trailers_val);
    lowerLettersInHeaders(trailers_key);
    request.trailers[trailers_key] = trailers_val;
    return PARSE_DONE;
}

static ParseResult parseTrailers(HttpRequest& request)
{
    auto pos = request.buffer.find("\r\n");
        if (pos == std::string::npos)
            return PARSE_IN_PROGRESS;
    if (pos == 0)
    {
        request.buffer.erase(0, 2);
        request.chunkState = CHUNK_DONE;
        return PARSE_DONE;
    }
    std::string headerLine = request.buffer.substr(0, pos);
    ParseResult res = parseSingleTrailer(headerLine, request);
    if (res == PARSE_ERROR)
        return PARSE_ERROR;
    request.buffer.erase(0, pos + 2);
    return PARSE_IN_PROGRESS;

}

static ParseResult parseChunkedBody(HttpRequest& request)
{
    while (true)
    {
        if (request.chunkState == CHUNK_DONE)
        {
            std::cout << "Decoded body: [";
            for (size_t i = 0; i < request.body.size(); ++i)
                std::cout << static_cast<char>(request.body[i]);
            std::cout << "]\n";
            return PARSE_DONE;
        }
        if (request.buffer.empty())
            return PARSE_IN_PROGRESS;
        ParseResult result = PARSE_IN_PROGRESS;
        switch (request.chunkState)
        {
            case CHUNK_SIZE:
                result = parseChunkSize(request);
                break;

            case CHUNK_SIZE_CRLF:
               result = parseChunkSizeCRLF(request);
                break;
            case CHUNK_DATA:
                result = parseChunkData(request);
                break;
            case CHUNK_DATA_CRLF:
                result = parseChunkDataCRLF(request);
                break;
            case CHUNK_TRAILERS:
                result = parseTrailers(request);
                break;
            case CHUNK_DONE:
                return PARSE_DONE;
        }
        if (result ==  PARSE_ERROR)
            return PARSE_ERROR;
    }
}


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
    std::cout << "request.bodyType: " << request.bodyType << "\n";

    if (request.bodyType == CONTENT_LENGTH)
    {
        // std::cout << "body type condition." << "\n";
        ParseResult res = parseLengthBody(request);
        if (res == PARSE_DONE)
        {
            request.parseState = DONE;
            return PARSE_DONE;
        }
        if (res == PARSE_ERROR)
            return PARSE_ERROR;
        return PARSE_IN_PROGRESS;
        
    }
    else if (request.bodyType == CHUNKED)
    {
        std::cout << "ParseBody chunked." << "\n";

        ParseResult res = parseChunkedBody(request);
        if (res == PARSE_DONE)
        {
            request.parseState = DONE;
            return PARSE_DONE;
        }
        
        if (res == PARSE_ERROR)
        {
            std::cout << "ParseBody chunked error" << "\n";
            return PARSE_ERROR;
        }
        return PARSE_IN_PROGRESS;
    }
    else if (request.bodyType == NONE)
    {
        request.parseState = DONE;
        return PARSE_DONE;
    }
    return PARSE_ERROR;
}