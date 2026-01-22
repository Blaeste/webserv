/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lmarck <lmarck@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:27 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/22 19:03:11 by lmarck           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s)
#include <string>
#include <vector>
#include <map>

// Only on Linux / Mac
#ifndef O_NOFOLLOW
#define O_NOFOLLOW 0
#endif

// Structure(s)
struct UploadedFile
{

	std::string filename;
	std::string contentType;
	std::string content;
};

// Class
class HttpRequest
{

private:
	// Attribute(s)

	std::string _method;						 // Method (GET, POST, etc.)
	std::string _uri;							 // URI (/index.html, /api/data, etc.)
	std::string _version;						 // HTTP version (HTTP/1.1)
	std::map<std::string, std::string> _headers; // Headers
	std::string _body;							 // Body
	std::string _rawData;						 // Raw request data
	bool _isComplete;							 // Is the request complete
	int _errorCode;								 // HTTP error code
	std::vector<UploadedFile> _uploadedFiles;	 // Uploaded files (for multipart/form-data)
	bool _headersParsed;						 // Have we already parsed the headers
	size_t _bodyStart;							 // Offset to body start inside _rawData
	bool _isChunked;							 // Transfer-Encoding: chunked detected
	size_t _contentLength;						 // Parsed Content-Length when present
	size_t _chunkParsePos;						 // Cursor while parsing chunked body
	size_t _chunkTotalSize;						 // Accumulated chunk payload size
	bool _chunkDone;							 // Final chunk parsed

	// Security limits
	static const size_t MAX_REQUEST_SIZE = 110 * 1024 * 1024; // 100Mb
	static const size_t MAX_URI_LENGTH = 8192;
	static const size_t MAX_HEADER_COUNT = 100;
	static const size_t MAX_HEADER_SIZE = 8192;
	static const size_t MAX_REQUEST_LINE_SIZE = 16384;
	static const size_t MAX_METHOD_LENGTH = 16;
	static const size_t MAX_CHUNK_SIZE = 5 * 1024 * 1024;  // 5Mb
	static const size_t MAX_BODY_SIZE = 110 * 1024 * 1024; // 100Mb

public:
	// Default constructor

	HttpRequest();

	// Public method(s)

	/**
	 * @brief Appends raw data to the HTTP request and attempts to parse it.
	 * @param data The raw data to append.
	 * @return true if the request is complete after appending, false otherwise.
	 */
	bool appendData(const std::string &data);

	/**
	 * @brief Checks if the HTTP request is complete.
	 * @return true if the request is complete, false otherwise.
	 */
	bool isComplete() const;

	/**
	 * @brief Gets a specific header value by key.
	 * @param key The header name (e.g., "Content-Type").
	 * @return The header value if found, empty string otherwise.
	 */
	std::string getHeader(const std::string &key) const;

	// Getters

	const std::string &getMethod() const { return _method; }
	const std::string &getUri() const { return _uri; }
	const std::string &getVersion() const { return _version; }
	const std::string &getBody() const { return _body; }
	const std::map<std::string, std::string> &getHeaders() const { return _headers; }
	const std::vector<UploadedFile> &getUploadedFiles() const { return _uploadedFiles; }
	int getErrorCode() const;
	std::map<std::string, std::string> getCookies() const;

private:
	// Private method(s)

	/**
	 * @brief Parses the request line from the header block.
	 * @param headerBlock The header block containing the request line.
	 * @return true if parsing was successful, false otherwise.
	 */
	bool parse();

	/**
	 * @brief Parses the request line from the header block.
	 * @param headerBlock The header block containing the request line.
	 * @return true if parsing was successful, false otherwise.
	 */
	bool parseRequestLine(const std::string &headerBlock);

	/**
	 * @brief Parses the headers from the header block.
	 * @param headerBlock The header block containing the headers.
	 * @return true if parsing was successful, false otherwise.
	 */
	bool parseHeaders(const std::string &headerBlock);

	/**
	 * @brief Parses the body of a chunked transfer encoded request.
	 * @param offset The offset in _rawData where the chunked body starts (after headers).
	 * @return true if parsing was successful, false otherwise.
	 */
	bool parseChunked();

	/**
	 * @brief Parses multipart/form-data body.
	 * @param boundary The boundary string used to separate parts.
	 * @return true if parsing was successful, false otherwise.
	 */
	bool parseMultipart(const std::string &boundary);

	bool setError(int code);
};
