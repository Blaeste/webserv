/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:18 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/23 16:32:58 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// =============================================================================
// Includes

#include "HttpRequest.hpp"
#include "../utils/utils.hpp"

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

std::string HttpRequest::getHeader(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it = _headers.find(key);
    if (it != _headers.end())
        return it->second;
    return "";
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

bool HttpRequest::parseChunked()
{
    std::string &data = _rawData;
    std::string body;
    size_t pos = 0;

    while (true)
    {
        // Find chunk size line
        size_t lineEnd = data.find("\r\n", pos);
        if (lineEnd == std::string::npos)
            return false; // Incomplete chunk

        // Parse chunk size (hexa)
        std::string sizeStr = data.substr(pos, lineEnd - pos);
        size_t chunkSize = std::strtol(sizeStr.c_str(), NULL, 16);

        pos = lineEnd + 2; // skip "\r\n"

        // Last chunk (size = 0)
        if (chunkSize == 0)
        {
            _body = body;
            return true;
        }

        // Check if chunk is complet (full data)
        if (pos + chunkSize + 2 > data.length())
            return false; // Incomplete chunk data

        // Extract chunk data
        //body += data.substr(pos, chunkSize);
        body.append(data, pos, chunkSize);
        pos += chunkSize + 2; // Skip data + \r\n
    }
}

bool HttpRequest::parseMultipart(const std::string &boundary)
{
    std::string delimiter = "--" + boundary;
    std::string endDelimiter = delimiter + "--";

    size_t pos = 0;

    while (true)
    {
        // Find next boundary
        size_t boundaryPos = _body.find(delimiter, pos);
        if (boundaryPos == std::string::npos)
            break;

        // add delimiter length
        pos = boundaryPos + delimiter.length();

        // Skip \r\n after boundary
        if (_body[pos] == '\r' && _body[pos + 1] == '\n')
            pos += 2;

        // Check if end delimiter
        if (_body.substr(boundaryPos, endDelimiter.length()) ==  endDelimiter)
            break;

        // Find headers end (\r\n\r\n)
        size_t headersEnd = _body.find("\r\n\r\n", pos);
        if (headersEnd == std::string::npos)
            return false;

        // Extract part headers
        std::string partHeaders = _body.substr(pos, headersEnd - pos);
        pos = headersEnd + 4;

        // Find next boundary to get content
        size_t nextBoundary = _body.find(delimiter, pos);
        if (nextBoundary == std::string::npos)
            return false;

        // Extract content (remove trailling \r\n)
        std::string content = _body.substr(pos, nextBoundary - pos - 2);

        // Parse part headers to extract filename and content type
        UploadedFile file;
        file.content = content;

        // Extract filename from Content-Disposition
        size_t filenamePos = partHeaders.find("filename=\"");
        if (filenamePos != std::string::npos)
        {
            filenamePos += 10; // 'skip filename="'
            size_t filenameEnd = partHeaders.find("\"", filenamePos);
            file.filename = partHeaders.substr(filenamePos, filenameEnd - filenamePos);
        }

        // Extract Content-Type
        size_t ctPos = partHeaders.find("Content-Type: ");
        if (ctPos != std::string::npos)
        {
            ctPos += 14; // Skip 'Content-Type: '
            size_t ctEnd = partHeaders.find("\r\n", ctPos);
            file.contentType = partHeaders.substr(ctPos, ctEnd - ctPos);
        }
        else
            file.contentType = "application/octet-stream";

        _uploadedFiles.push_back(file);
        pos = nextBoundary;
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

    // Check if body type (chunked or Content-Length)
    size_t bodyStart = headersEnd + 4; // +4 for "\r\n\r\n"

    // Case 1 Chunked "Transfer-Encoding"
    if (_headers.find("Transfer-Encoding") != _headers.end() &&
        _headers["Transfer-Encoding"] == "chunked")
    {
        // Remove headers from _rawdata befor parsing chunk
        _rawData.erase(0, bodyStart);
        return parseChunked();
    }

    // Cas 2 Content Length
    else if (_headers.find("Content-Length") != _headers.end())
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

    // Check if multipart/form-data
    if (_headers.find("Content-Type") != _headers.end())
    {
        std::string contentType = _headers["Content-Type"];
        if (contentType.find("multipart/form-data") != std::string::npos)
        {
            // Extract boundary
            size_t boundaryPos = contentType.find("boundary=");
            if (boundaryPos != std::string::npos)
            {
                std::string boundary = contentType.substr(boundaryPos + 9);
                parseMultipart(boundary);
            }
        }
    }

    return true;
}

