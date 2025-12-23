/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:27 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/23 14:36:03 by eschwart         ###   ########.fr       */
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
#include <map>

// =============================================================================
// Typedef

// =============================================================================
// Defines

// =============================================================================
// Class HttpRequest

class HttpRequest
{

private:

	// =========================================================================
	// Attributs

	// Method (GET, POST, etc.)
	std::string							_method;

	// URI (/index.html, /api/data, etc.)
	std::string							_uri;

	// HTTP version (HTTP/1.1)
	std::string							_version;

	// Headers
	std::map<std::string, std::string>	_headers;

	// Body
	std::string							_body;

	// Raw request data
	std::string							_rawData;

	// Is the request complete
	bool								_isComplete;

	// TODO: bool _isChunked;

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

    // =========================================================================
	// Getters

	/**
	 * @brief Gets the HTTP method of the request.
	 * @return The HTTP method as a string.
	 */
    const std::string &getMethod() const { return _method; }

	/**
	 * @brief Gets the URI of the request.
	 * @return The URI as a string.
	 */
    const std::string &getUri() const { return _uri; }

	/**
	 * @brief Gets the HTTP version of the request.
	 * @return The HTTP version as a string.
	 */
    const std::string &getVersion() const { return _version; }

	/**
	 * @brief Gets the headers of the request.
	 * @return A constant reference to the map of headers.
	 */
    const std::map<std::string, std::string> &getHeaders() const { return _headers; }


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
};
