/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:27 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/23 11:02:42 by eschwart         ###   ########.fr       */
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

	std::string							_method;
	std::string							_uri;
	std::string							_version;
	std::map<std::string, std::string>	_headers;
	std::string							_body;
	std::string							_rawData;
	bool								_isComplete;
	// TODO: bool _isChunked;

public:

	// =========================================================================
	// Handlers
	HttpRequest();
	~HttpRequest();

	// =========================================================================
	// Public methods

	bool appendData(const std::string &data);

    // Vérifie si la requête est complète
    bool isComplete() const;

    // =========================================================================
	// Getters

    const std::string &getMethod() const { return _method; }
    const std::string &getUri() const { return _uri; }
    const std::string &getVersion() const { return _version; }
    const std::map<std::string, std::string> &getHeaders() const { return _headers; }

private:

	// =========================================================================
	// Methods

	bool parse();
	bool parseRequestLine(const std::string &headerBlock);
	bool parseHeaders(const std::string &headerBlock);
	// TODO: bool parseBody(const std::string &bodyData);
	// TODO: bool parseChunkedBody(const std::string &chunkedData);

};
