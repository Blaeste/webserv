/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:27 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/23 16:30:55 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Parse une requete HTTP brute
// Extrait method, uri, headers, body
// Gere chunked encoding
// Validation

// =============================================================================
// Includes

#pragma once

#include <string>
#include <vector>
#include <map>

// =============================================================================
// Typedef

// =============================================================================
// Structure

struct UploadedFile
{
	std::string filename;
	std::string contentType;
	std::string content;
};

// =============================================================================
// Class HttpRequest

class HttpRequest
{

private:

	// =========================================================================
	// Attributs

	// Method (GET, POST, etc.)
	std::string _method;
	// URI (/index.html, /api/data, etc.)
	std::string _uri;
	// HTTP version (HTTP/1.1)
	std::string _version;
	// Headers
	std::map<std::string, std::string> _headers;
	// Body
	std::string _body;
	// Raw request data
	std::string _rawData;
	// Is the request complete
	bool _isComplete;
	// Uploaded files (for multipart/form-data)
	std::vector<UploadedFile> _uploadedFiles;

public:

	// =========================================================================
	// Handlers

	// Default constructor
	HttpRequest();

	// Destructor
	~HttpRequest();

	// =========================================================================
	// Public methods

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

    // =========================================================================
	// Getters

    const std::string &getMethod() const { return _method; }

    const std::string &getUri() const { return _uri; }

    const std::string &getVersion() const { return _version; }

    const std::map<std::string, std::string> &getHeaders() const { return _headers; }

	const std::vector<UploadedFile> &getUploadedFiles() const { return _uploadedFiles; }

private:

	// =========================================================================
	// Methods

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
	 * @return true if parsing was successful, false otherwise.
	 */
	bool parseChunked();

	/**
	 * @brief Parses multipart/form-data body.
	 * @param boundary The boundary string used to separate parts.
	 * @return true if parsing was successful, false otherwise.
	 */
	bool parseMultipart(const std::string &boundary);
};
