/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:27 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/16 13:14:41 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Parse une requete HTTP brute
// Extrait method, uri, headers, body
// Gere chunked encoding
// Validation

#pragma once

#include <string>
#include <map>

class HttpRequest
{
private:
	// TODO: std::string _method;
	// TODO: std::string _uri;
	// TODO: std::string _version;
	// TODO: std::map<std::string, std::string> _headers;
	// TODO: std::string _body;
	// TODO: bool _isComplete;
	// TODO: bool _isChunked;

public:
	HttpRequest();
	~HttpRequest();

	// TODO: bool parse(const std::string &rawData);
	// TODO: bool isComplete() const;

	// TODO: Getters for all attributes

private:
	// TODO: bool parseRequestLine(const std::string &line);
	// TODO: bool parseHeaders(const std::string &headerBlock);
	// TODO: bool parseBody(const std::string &bodyData);
	// TODO: bool parseChunkedBody(const std::string &chunkedData);
};
