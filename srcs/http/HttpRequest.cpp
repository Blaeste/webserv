/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:18 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/23 10:57:37 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// =============================================================================
// Includes

#include "HttpRequest.hpp"
#include "../utils/utils.hpp"

#include <vector>
#include <cstdlib>

// =============================================================================
// Handlers

// Default constructor
HttpRequest::HttpRequest() : _isComplete(false)
{
}

// Destructor
HttpRequest::~HttpRequest()
{
}

// =============================================================================
// Setters

// =============================================================================
// Getters

// =============================================================================
// Methods

bool HttpRequest::appendData(const std::string &data)
{
    _rawData += data;

    // if not complete try to parse
    if (!_isComplete)
        _isComplete = parse();

    return _isComplete;
}

bool HttpRequest::isComplete() const
{
    return _isComplete;
}

bool HttpRequest::parseRequestLine(const std::string &headerBlock)
{
    // Search the first line
    size_t firstLineEnd = headerBlock.find("\r\n");
    if (firstLineEnd == std::string::npos)
        return false;

    std::string requestLine = headerBlock.substr(0, firstLineEnd);

    // Split on " " : "GET /index.html HTTP/1.1"
    std::vector<std::string> parts = split(requestLine, ' ');
    if (parts.size() != 3)
        return false;

    _method = parts[0];
    _uri = parts[1];
    _version = parts[2];

    return true;
}

bool HttpRequest::parseHeaders(const std::string &headerBlock)
{
    // skip first line (request line)
    size_t pos = headerBlock.find("\r\n") + 2;

    while (pos < headerBlock.length())
    {
        size_t lineEnd = headerBlock.find("\r\n", pos);
        if (lineEnd == std::string::npos)
            break;

        std::string line = headerBlock.substr(pos, lineEnd - pos);

        // Split on ": "
        size_t colonPos = line.find(": ");
        if (colonPos != std::string::npos)
        {
            std::string key = line.substr(0, colonPos);
            std::string value = line.substr(colonPos + 2);
            _headers[key] = value;
        }

        pos = lineEnd  + 2;
    }

    return true;
}

bool HttpRequest::parse()
{
    // Search for end of headers
    size_t headersEnd = _rawData.find("\r\n\r\n");
    if (headersEnd == std::string::npos)
        return false;

    // Extract headers
    std::string headersBlock = _rawData.substr(0, headersEnd);

    // Parse request line + headers
    if (!parseRequestLine(headersBlock))
        return false;
    if (!parseHeaders(headersBlock))
        return false;

    // Check if body exists
    size_t bodyStart = headersEnd + 4; // +4 for "\r\n\r\n"

    // if content length exist, check size
    if (_headers.find("Content-Length") != _headers.end())
    {
        size_t contentLength = atoi(_headers["Content-Length"].c_str());

        if (_rawData.length() < bodyStart + contentLength)
            return false; // incomplet body

        // Extract body with exact content length
        _body = _rawData.substr(bodyStart, contentLength);
    }
    else
    {
        // No content length = no body
        _body = "";
    }

    return true;
}
