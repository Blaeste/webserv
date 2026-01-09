/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 14:23:30 by gdosch            #+#    #+#             */
/*   Updated: 2026/01/09 13:00:40 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "Router.hpp"
#include "../utils/utils.hpp"
#include <iostream>

// Private method(s)
// Find location with longest matching path prefix
const Location* Router::findMatchingLocation(const ServerConfig& config, const std::string& uri) const {
	const std::vector<Location>& locations = config.getLocations();
	const Location* bestMatch = NULL;
	size_t longestMatch = 0;
	for (size_t i = 0; i < locations.size(); i++) {
		const std::string& path = locations[i].getPath();

		// Check if URI starts with location path
		if (uri.find(path) == 0 && path.length() > longestMatch) {
				longestMatch = path.length();
				bestMatch = &locations[i];
		}
	}
	return bestMatch;
}

// Public method(s)
// Match request to appropriate route and determine response type
RouteMatch Router::matchRoute(const ServerConfig& config, const HttpRequest& request) const {
	std::string uri = request.getUri();
	RouteMatch match;
	match.serverName = config.getServerName();
	match.serverPort = config.getPort();
	match.statusCode = 200;
	match.isRedirect = false;
	match.isCGI = false;

	// Security: block path traversal in URI (both raw and encoded)
	if (uri.find("..") != std::string::npos ||
		uri.find("%2e%2e") != std::string::npos ||
		uri.find("%2E%2E") != std::string::npos)
	{
		match.statusCode = 403;
		return match;
	}

	// Find matching location block
	match.location = findMatchingLocation(config, uri);
	if (!match.location)
		match.statusCode = 404;
	else {
		std::string method = request.getMethod();

		// Check if method is implemented
		if (method != "GET" && method != "POST" && method != "DELETE" && method != "PUT" && method != "HEAD" && method != "OPTIONS")
			match.statusCode = 501;

		// Check if HTTP method is allowed
		else if (!match.location->isMethodAllowed(method))
			match.statusCode = 405;

		else if (!(match.redirectUrl = match.location->getRedirect()).empty()) {
			match.isRedirect = true;
			match.statusCode = 301;
		}

		// Only proceed with file resolution if no error yet
		if (match.statusCode == 200) {
			// Try index files for root or directory paths
			if (uri == "/" || uri.empty()) {
				const std::vector<std::string>& indexes = match.location->getIndex();
				for (size_t i = 0; i < indexes.size(); i++) {
					std::string indexPath = match.location->getRoot() + "/" + indexes[i];
					if (fileExists(indexPath)) {
						uri = "/" + indexes[i];
						break;
					}
				}
			}

			// Remove query string from path
			std::string pathPart = uri;
			size_t queryPos = uri.find('?');
			if (queryPos != std::string::npos)
				pathPart = uri.substr(0, queryPos);

			// Build full file path
			match.filePath = match.location->getRoot() + pathPart;

			// Security check
			if (!isPathSafe(match.filePath))
			{
				match.statusCode = 403;
				return match;
			}

			// Check if request should be handled by CGI
			std::string cgiExt = match.location->getCgiExtension();
			if (!cgiExt.empty() && cgiExt == getFileExtension(match.filePath))
				match.isCGI = true;

			// Check if file exists (skip for POST and CGI)
			if (match.statusCode == 200 && !match.isCGI && request.getMethod() != "POST"
				&& (!fileExists(match.filePath) || (isDirectory(match.filePath) && !match.location->getAutoIndex())))
				match.statusCode = 404;
		}
	}
	return match;
}
