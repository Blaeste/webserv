/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 14:23:30 by gdosch            #+#    #+#             */
/*   Updated: 2025/12/26 12:37:48 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Router.hpp"
#include "../utils/utils.hpp"

Router::Router() {}

Router::~Router() {}

const Location* Router::findMatchingLocation(const ServerConfig& config, const std::string& uri) const {
	const std::vector<Location>& locations = config.getLocations();
	const Location* bestMatch = NULL;
	size_t longestMatch = 0;
	for (size_t i = 0; i < locations.size(); i++) {
		const std::string& path = locations[i].getPath();
		if (uri.find(path) == 0 && path.length() > longestMatch) {
				longestMatch = path.length();
				bestMatch = &locations[i];
		}
	}
	return bestMatch;
}

bool Router::isMethodAllowed(const Location& loc, const std::string& method) const {
	const std::vector<std::string>& methods = loc.getAllowedMethods();
	for (size_t i = 0; i < methods.size(); i++)
		if (methods[i] == method)
			return true;
	return false;
}

RouteMatch Router::matchRoute(const ServerConfig& config, const HttpRequest& request) const {
	std::string uri = request.getUri();
	RouteMatch match;
	match.serverName = config.getServerName();
	match.serverPort = config.getPort();
	match.statusCode = 200;
	match.isRedirect = false;
	match.isCGI = false;
	match.location = findMatchingLocation(config, uri);
	if (!match.location)
		match.statusCode = 404;
	else if (!isMethodAllowed(*match.location, request.getMethod())) // Check if HTTP method is allowed
		match.statusCode = 405;
	else if (!(match.redirectUrl = match.location->getRedirect()).empty()) {
		match.isRedirect = true;
		match.statusCode = 301;
	}
	else {
		if (uri == "/" || uri.empty())
			uri = "/index.html";
		std::string pathPart = uri;
		size_t queryPos = uri.find('?');
		if (queryPos != std::string::npos)
			pathPart = uri.substr(0, queryPos);
		match.filePath = match.location->getRoot() + pathPart;
		std::string cgiExt = match.location->getCgiExtension();
		if (!cgiExt.empty() && cgiExt == getFileExtension(match.filePath))
			match.isCGI = true;
		if (!match.isCGI && request.getMethod() != "POST" && (!fileExists(match.filePath) || isDirectory(match.filePath)))
			match.statusCode = 404;
	}
	return match;
}
