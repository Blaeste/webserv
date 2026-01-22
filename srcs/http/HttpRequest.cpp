/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lmarck <lmarck@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:18 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/22 19:03:11 by lmarck           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "HttpRequest.hpp"
#include "../utils/utils.hpp"
#include <cstdlib>
#include <cctype>
#include <iostream>
#include <sstream>
#include <cerrno>

// Default constructor
HttpRequest::HttpRequest()
	: _isComplete(false), _errorCode(0), _headersParsed(false), _bodyStart(0), _isChunked(false), _contentLength(0), _chunkParsePos(0), _chunkTotalSize(0), _chunkDone(false)
{
}

// Public method(s)
int HttpRequest::getErrorCode() const
{
	return _errorCode;
}

bool HttpRequest::setError(int code)
{
	_errorCode = code;
	return false;
}

std::map<std::string, std::string> HttpRequest::getCookies() const
{
	std::map<std::string, std::string> cookies;
	std::string cookieHeader = getHeader("Cookie");
	if (cookieHeader.empty())
		return cookies;

	std::istringstream stream(cookieHeader);
	std::string pair;
	while (std::getline(stream, pair, ';'))
	{
		size_t eqPos = pair.find('=');
		if (eqPos != std::string::npos)
		{

			std::string key = trim(pair.substr(0, eqPos));
			std::string value = trim(pair.substr(eqPos + 1));
			cookies[key] = value;
		}
	}
	return cookies;
}

bool HttpRequest::appendData(const std::string &data)
{
	if (_isComplete)
		return true;

	// Security: prevent mem exhaustion attack
	if (_rawData.length() + data.length() > MAX_REQUEST_SIZE)
	{
		_errorCode = 413; // Payload too large
		_isComplete = true;
		return true;
	}

	_rawData += data;

	// try to parse with the new data
	_isComplete = parse();

	// If parsing failed with an error, mark as complete anyway
	// so we can send the error response immediately
	if (!_isComplete && _errorCode)
		_isComplete = true;

	return _isComplete;
}

bool HttpRequest::isComplete() const
{
	return _isComplete;
}

std::string HttpRequest::getHeader(const std::string &key) const
{
	std::map<std::string, std::string>::const_iterator it = _headers.find(normalizeHeaderKey(key));
	if (it != _headers.end())
		return it->second;
	return "";
}

// Private method(s)
bool HttpRequest::parseRequestLine(const std::string &headerBlock)
{
	// Search the first line
	size_t firstLineEnd = headerBlock.find("\r\n");
	if (firstLineEnd == std::string::npos)
		return false;

	// Security: Check request line size
	if (firstLineEnd > MAX_REQUEST_LINE_SIZE)
		return setError(414); // URI too long

	std::string requestLine = headerBlock.substr(0, firstLineEnd);

	// Split on " " : "GET /index.html HTTP/1.1"
	std::vector<std::string> parts = splitTokens(requestLine, ' ');
	if (parts.size() != 3)
		return setError(400); // Bad request

	_method = parts[0];
	_uri = parts[1];
	_version = parts[2];

	// Security: Validate method (length + only aphabetic char)
	if (_method.length() > MAX_METHOD_LENGTH || _method.empty())
		return setError(400); // Bad request

	for (size_t i = 0; i < _method.length(); i++)
	{
		unsigned char c = static_cast<unsigned char>(_method[i]);
		if (!std::isalpha(c))
			return setError(400); // Bad request
	}

	// Security: Validate URI (length + start with "/")
	if (_uri.length() > MAX_URI_LENGTH)
		return setError(414); // URI too long

	if (_uri.empty() || _uri[0] != '/')
		return setError(400); // Bad request

	// Security: Check for null bytes in URI
	if (_uri.find('\0') != std::string::npos)
		return setError(400); // Bad request

	// Security: Validate HTTP version
	if (_version != "HTTP/1.1" && _version != "HTTP/1.0")
		return setError(505); // HTTP version not supported

	return true;
}

bool HttpRequest::parseHeaders(const std::string &headerBlock)
{
	// skip first line (request line)
	size_t pos = headerBlock.find("\r\n") + 2;
	size_t headerCount = 0;

	while (pos < headerBlock.length())
	{
		size_t lineEnd = headerBlock.find("\r\n", pos);
		if (lineEnd == std::string::npos)
			break;

		// Security: Check header line size
		if (lineEnd - pos > MAX_HEADER_SIZE)
			return setError(431); // Request Header Fields Too Large

		std::string line = headerBlock.substr(pos, lineEnd - pos);

		// Split on ": "
		size_t colonPos = line.find(':');
		if (colonPos != std::string::npos)
		{
			std::string key = line.substr(0, colonPos);
			std::string value = line.substr(colonPos + 1);

			// remove " " if necessary
			value = trim(value);

			// Security: Validate header key format
			if (key.empty())
				return setError(400); // Bad request

			for (size_t i = 0; i < key.length(); i++)
			{
				unsigned char c = static_cast<unsigned char>(key[i]);
				bool isValid = (c >= 'a' && c <= 'z') || // lettres minuscules
							   (c >= 'A' && c <= 'Z') || // lettres majuscules
							   (c >= '0' && c <= '9') || // chiffres
							   c == '!' || c == '#' || c == '$' || c == '%' || c == '&' ||
							   c == '\'' || c == '*' || c == '+' || c == '-' || c == '.' ||
							   c == '^' || c == '_' || c == '`' || c == '|' || c == '~';

				if (!isValid)
					return setError(400); // Bad request
			}

			// Security: Limit number of headers
			headerCount++;
			if (headerCount > MAX_HEADER_COUNT)
				return setError(431); // Request Header Fields Too Large

			_headers[normalizeHeaderKey(key)] = value;
		}

		pos = lineEnd + 2;
	}

	return true;
}

bool HttpRequest::parseChunked()
{
	std::string &data = _rawData;
	size_t pos = _chunkParsePos;

	while (true)
	{
		size_t sizeLineStart = pos;
		// Need a full chunk size line
		size_t lineEnd = data.find("\r\n", pos);
		if (lineEnd == std::string::npos)
		{
			_chunkParsePos = pos;
			return false; // incomplete chunk size line
		}

		std::string sizeStr = data.substr(pos, lineEnd - pos);

		// Support chunk extensions: "HEX;ext=..."
		size_t semi = sizeStr.find(';');
		if (semi != std::string::npos)
			sizeStr = sizeStr.substr(0, semi);

		// Security: Validate chunk size format (must be hex)
		if (sizeStr.empty() || sizeStr.length() > 16)
			return setError(400); // Bad request

		for (size_t i = 0; i < sizeStr.length(); i++)
		{
			unsigned char x = static_cast<unsigned char>(sizeStr[i]);
			if (!std::isxdigit(x))
				return setError(400); // Bad request
		}

		errno = 0;
		unsigned long v = std::strtoul(sizeStr.c_str(), NULL, 16);
		if (errno)
			return setError(400); // Bad request

		size_t chunkSize = static_cast<size_t>(v);
		pos = lineEnd + 2; // skip "\r\n"

		// Last chunk (size = 0)
		if (!chunkSize)
		{
			// Need final CRLF after the zero-size chunk
			if (pos + 2 > data.length())
			{
				_chunkParsePos = sizeLineStart; // wait for more data
				return false;
			}

			if (data.compare(pos, 2, "\r\n") != 0)
				return setError(400); // Bad request

			_chunkParsePos = pos + 2;
			_chunkDone = true;
			_isComplete = true;
			return true;
		}

		// Security: Check individual chunk size
		if (chunkSize > MAX_CHUNK_SIZE)
			return setError(413); // Payload Too Large

		// Security: Check total accumulated body size
		if (_chunkTotalSize + chunkSize > MAX_BODY_SIZE)
			return setError(413); // Payload Too Large

		// Need full chunk payload + trailing CRLF
		if (pos + chunkSize + 2 > data.length())
		{
			_chunkParsePos = sizeLineStart; // retry from size line when more data arrives
			return false;
		}

		if (data.compare(pos + chunkSize, 2, "\r\n"))
			return setError(400); // Bad request

		// Append chunk payload and advance
		_body.append(data, pos, chunkSize);
		_chunkTotalSize += chunkSize;
		pos += chunkSize + 2;
		_chunkParsePos = pos;
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

		if (pos >= _body.length())
			return setError(400); // Bad request

		// Skip \r\n after boundary
		if (pos + 1 < _body.length() && !_body.compare(pos, 2, "\r\n"))
			pos += 2;

		// Check if end delimiter
		if (_body.substr(boundaryPos, endDelimiter.length()) == endDelimiter)
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

		// Extract content (remove trailing \r\n)
		if (nextBoundary < pos + 2)
			return setError(400); // Bad request

		std::string content = _body.substr(pos, (nextBoundary - pos) - 2);

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
	// 1) Parse headers once
	if (!_headersParsed)
	{
		size_t headersEnd = _rawData.find("\r\n\r\n");
		if (headersEnd == std::string::npos)
			return false;

		std::string headersBlock = _rawData.substr(0, headersEnd + 2); // Include first \r\n

		if (!parseRequestLine(headersBlock))
		{
			if (_errorCode)
				_isComplete = true;
			return _isComplete;
		}
		if (!parseHeaders(headersBlock))
		{
			if (_errorCode)
				_isComplete = true;
			return _isComplete;
		}

		// Security: check if Host is mandatory in HTTP/1.1
		if (_version == "HTTP/1.1" && getHeader("host").empty())
		{
			_isComplete = true;
			return setError(400); // Bad request
		}

		_bodyStart = headersEnd + 4; // +4 for "\r\n\r\n"
		std::string te = toLowercase(trim(getHeader("transfer-encoding")));
		std::string clStr = getHeader("content-length");

		// If both TE and CL (anti request smuggling) ignore Content-Length
		if (!te.empty())
			clStr.clear();

		if (!te.empty())
		{
			if (te.find("chunked") == std::string::npos)
			{
				_isComplete = true;
				return setError(501); // Not implemented
			}
			_isChunked = true;
			// Drop headers to keep buffer small for incremental chunk parsing
			_rawData.erase(0, _bodyStart);
			_bodyStart = 0;
			_chunkParsePos = 0;
			_chunkTotalSize = 0;
			_chunkDone = false;
		}
		else
		{
			_isChunked = false;
			_contentLength = 0;

			if (!clStr.empty())
			{
				for (size_t i = 0; i < clStr.length(); i++)
				{
					unsigned char d = static_cast<unsigned char>(clStr[i]);
					if (!std::isdigit(d))
					{
						_isComplete = true;
						return setError(400); // Bad request
					}
				}

				// Security: Check for overflow (max 20 digits)
				if (clStr.length() > 20)
				{
					_isComplete = true;
					return setError(413); // Payload Too Large
				}

				size_t contentLength = parseIntSafe(clStr.c_str(), "Content-Length header");

				// Security: Check Content-Length against max body size
				if (contentLength > MAX_BODY_SIZE)
				{
					_isComplete = true;
					return setError(413); // Payload Too Large
				}

				_contentLength = contentLength;
			}
		}

		_headersParsed = true;
	}

	if (_errorCode)
	{
		_isComplete = true;
		return true;
	}

	// 2) Parse body depending on mode
	if (_isChunked)
	{
		bool parsed = parseChunked();
		if (_errorCode)
			_isComplete = true;

		// When the final chunk is parsed, the body is ready
		if (_isComplete && !_errorCode)
		{
			// Check multipart/form-data only once when body is complete
			std::string contentType = getHeader("content-type");
			if (!_uploadedFiles.empty() || contentType.empty())
				return true;

			if (contentType.find("multipart/form-data") != std::string::npos)
			{
				size_t boundaryPos = contentType.find("boundary=");
				if (boundaryPos != std::string::npos)
				{
					std::string boundary = contentType.substr(boundaryPos + 9);
					if (!parseMultipart(boundary))
						return false;
				}
			}
			return true;
		}

		return parsed;
	}

	// Content-Length body
	if (_contentLength > 0)
	{
		if (_bodyStart > _rawData.length() || _contentLength > _rawData.length() - _bodyStart)
			return false; // incomplete body

		_body = _rawData.substr(_bodyStart, _contentLength);
		_isComplete = true;

		std::string contentType = getHeader("content-type");
		if (!contentType.empty())
		{
			if (contentType.find("multipart/form-data") != std::string::npos)
			{
				size_t boundaryPos = contentType.find("boundary=");
				if (boundaryPos != std::string::npos)
				{
					std::string boundary = contentType.substr(boundaryPos + 9);
					if (!parseMultipart(boundary))
						return false;
				}
			}
		}
		return true;
	}

	// No body
	_body = "";
	_isComplete = true;
	return true;
}
