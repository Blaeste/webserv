/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:33:36 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/16 13:39:16 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Construit une reponse HTTP
// Gere les status codes
// Headers automatiaues (Content-Length, etc.)
// Lit les fichiers statiques

#pragma once

#include <string>
#include <map>

class HttpResponse
{
private:
	// TODO: int _statusCode;
	// TODO: std::string _statusMessage;
	// TODO: std::map<std::string, std::string> _headers;
	// TODO: std::string _body;

public:
	HttpResponse();
	~HttpResponse();

	// TODO: void setStatus(int code);
	// TODO: void setHeader(const std::string &key, const std::string &value);
	// TODO: void setBody(const std::string &body);
	// TODO: std::string build() const;

	// TODO: void serveFile(const std::string &path);
	// TODO: void serveError(int code, const std::string &errorPagePath);
	// TODO: void serveDirectoryListing(const std::string &path);
};
