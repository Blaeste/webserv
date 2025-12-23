/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:33:36 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/23 13:10:25 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Construit une reponse HTTP
// Gere les status codes
// Headers automatiaues (Content-Length, etc.)
// Lit les fichiers statiques

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
// Class HttpResponse

class HttpResponse
{

private:

	// =========================================================================
	// Attributs

	// HTTP status code
	int									_statusCode;

	// HTTP status message
	std::string							_statusMessage;

	// Headers
	std::map<std::string, std::string>	_headers;

	// Body
	std::string							_body;

public:

	// =========================================================================
	// Handlers

	// Default constructor
	HttpResponse();

	// Destructor
	~HttpResponse();

	// =========================================================================
	// Public methods

	/**
	 * @brief Sets the HTTP status code and corresponding message.
	 * @param code The HTTP status code.
	 */
	void setStatus(int code);

	/**
	 * @brief Sets a header key-value pair.
	 * @param key The header field name.
	 * @param value The header field value.
	 */
	void setHeader(const std::string &key, const std::string &value);

	/**
	 * @brief Sets the body of the HTTP response.
	 * @param body The body content as a string.
	 */
	void setBody(const std::string &body);

	/**
	 * @brief Builds the complete HTTP response as a raw string.
	 * @return The raw HTTP response string.
	 */
	std::string build() const;

	/**
	 * @brief Serves a static file as the HTTP response body.
	 * @param path The path to the file to serve.
	 */
	void serveFile(const std::string &path);

	/**
	 * @brief Serves an error page for the given HTTP status code.
	 * @param code The HTTP status code.
	 * @param errorPagePath The path to a custom error page file (optional).
	 */
	void serveError(int code, const std::string &errorPagePath);

	/**
	 * @brief Serves a directory listing for the given path.
	 * @param path The directory path.
	 */
	void serveDirectoryListing(const std::string &path);

private:

	// =========================================================================
	// Methods

	/**
	 * @brief Gets the standard status message for a given HTTP status code.
	 * @param code The HTTP status code.
	 * @return The corresponding status message as a string.
	 */
	std::string getStatusMessage(int code) const;
};
