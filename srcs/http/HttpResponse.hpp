/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:33:36 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/23 11:02:21 by eschwart         ###   ########.fr       */
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

	int									_statusCode;
	std::string							_statusMessage;
	std::map<std::string, std::string>	_headers;
	std::string							_body;

public:

	// =========================================================================
	// Handlers

	HttpResponse();
	~HttpResponse();

	// =========================================================================
	// Public methods

	void setStatus(int code);
	void setHeader(const std::string &key, const std::string &value);
	void setBody(const std::string &body);
	std::string build() const;

	void serveFile(const std::string &path);
	void serveError(int code, const std::string &errorPagePath);
	void serveDirectoryListing(const std::string &path);

private:

	// =========================================================================
	// Methods

	std::string getStatusMessage(int code) const;
};
