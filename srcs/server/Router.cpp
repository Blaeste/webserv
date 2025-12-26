/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 14:23:30 by gdosch            #+#    #+#             */
/*   Updated: 2025/12/26 15:37:52 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Router.hpp"
#include "../utils/utils.hpp"

Router::Router() {}

Router::~Router() {}

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

// Match request to appropriate route and determine response type
RouteMatch Router::matchRoute(const ServerConfig& config, const HttpRequest& request) const {
	std::string uri = request.getUri();
	RouteMatch match;
	match.serverName = config.getServerName();
	match.serverPort = config.getPort();
	match.statusCode = 200;
	match.isRedirect = false;
	match.isCGI = false;

	// Find matching location block
	match.location = findMatchingLocation(config, uri);
	if (!match.location)
		match.statusCode = 404;
	else if (!match.location->isMethodAllowed(request.getMethod())) // Check if HTTP method is allowed
		match.statusCode = 405;
	else if (!(match.redirectUrl = match.location->getRedirect()).empty()) {
		match.isRedirect = true;
		match.statusCode = 301;
	}
	else {
		// Default to index.html for root path
		if (uri == "/" || uri.empty())
			uri = "/index.html";

		// Remove query string from path
		std::string pathPart = uri;
		size_t queryPos = uri.find('?');
		if (queryPos != std::string::npos)
			pathPart = uri.substr(0, queryPos);

		// Build full file path
		match.filePath = match.location->getRoot() + pathPart;

		// Check if request should be handled by CGI
		std::string cgiExt = match.location->getCgiExtension();
		if (!cgiExt.empty() && cgiExt == getFileExtension(match.filePath))
			match.isCGI = true;

		// Check if file exists (skip for POST and CGI)
		if (!match.isCGI && request.getMethod() != "POST"
			&& (!fileExists(match.filePath) || (isDirectory(match.filePath) && !match.location->getAutoIndex())))
			match.statusCode = 404;
	}
	return match;
}
