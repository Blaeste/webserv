/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lmarck <lmarck@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 14:23:30 by gdosch            #+#    #+#             */
/*   Updated: 2026/01/22 14:07:36 by lmarck           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "Router.hpp"
#include "../utils/utils.hpp"
#include <iostream>

// Private method(s)
// Find location with longest matching path prefix
const Location *Router::findMatchingLocation(const ServerConfig &config, const std::string &uri) const
{
	const std::vector<Location> &locations = config.getLocations();
	const Location *bestMatch = NULL;
	size_t longestMatch = 0;
	for (size_t i = 0; i < locations.size(); i++)
	{
		const std::string &path = locations[i].getPath();

		// Check if URI starts with location path
		// Must be exact match or followed by '/' to avoid false matches
		// e.g., /cgi-bin/php should not match /cgi-bin/py
		if (uri.find(path) == 0 && path.length() > longestMatch)
		{
			// Ensure it's a valid path prefix (either exact match or followed by '/')
			if (uri.length() == path.length() || uri[path.length()] == '/' || path[path.length() - 1] == '/')
			{
				longestMatch = path.length();
				bestMatch = &locations[i];
			}
		}
	}
	return bestMatch;
}

// Public method(s)
// Match request to appropriate route and determine response type
RouteMatch Router::matchRoute(const ServerConfig &config, const HttpRequest &request) const
{
	std::string uri = request.getUri();
	RouteMatch match;
	match.serverName = config.getServerName();
	match.serverPort = config.getPort();
	match.statusCode = 200;
	match.isRedirect = false;
	match.isCGI = false;

	// Extract path without query string
	std::string pathPart = uri;
	size_t queryPos = uri.find('?');
	if (queryPos != std::string::npos)
		pathPart = uri.substr(0, queryPos);

	// Security: block path traversal in URI (both raw and encoded)
	if (pathPart.find("..") != std::string::npos ||
		pathPart.find("%2e%2e") != std::string::npos ||
		pathPart.find("%2E%2E") != std::string::npos)
	{
		match.statusCode = 403;
		return match;
	}

	// Find matching location block
	match.location = findMatchingLocation(config, pathPart);
	if (!match.location)
		match.statusCode = 404;
	else
	{
		std::string method = request.getMethod();

		// Check if method is implemented
		if (method != "GET" && method != "POST" && method != "DELETE" && method != "PUT" && method != "HEAD" && method != "OPTIONS")
			match.statusCode = 501;

		// Check if HTTP method is allowed
		else if (!match.location->isMethodAllowed(method))
			match.statusCode = 405;

		else if (!(match.redirectUrl = match.location->getRedirect()).empty())
		{
			match.isRedirect = true;
			match.statusCode = 301;
		}

		// Only proceed with file resolution if no error yet
		if (match.statusCode == 200)
		{
			// Try index files for root or directory paths
			if (pathPart == "/" || pathPart.empty())
			{
				const std::vector<std::string> &indexes = match.location->getIndex();
				for (size_t i = 0; i < indexes.size(); i++)
				{
					std::string indexPath = match.location->getRoot() + "/" + indexes[i];
					if (fileExists(indexPath))
					{
						pathPart = "/" + indexes[i];
						break;
					}
				}
			}

			// Decode percent-encoded char
			std::string decodedPath = urlDecode(pathPart);

			// Build full file path (strip location prefix before joining with root)
			std::string locationPath = match.location->getPath();
			std::string relativePath = decodedPath;
			if (relativePath.find(locationPath) == 0)
				relativePath = decodedPath.substr(locationPath.length());
			// If the request exactly matches the location path, stay at that location root
			if (relativePath.empty())
				relativePath = "/";
			if (relativePath.empty() || relativePath[0] != '/')
				relativePath = "/" + relativePath;

			// If we are at the location root, try its index files
			if (relativePath == "/")
			{
				const std::vector<std::string> &locIndexes = match.location->getIndex();
				for (size_t i = 0; i < locIndexes.size(); i++)
				{
					std::string idxPath = match.location->getRoot() + "/" + locIndexes[i];
					if (fileExists(idxPath))
					{
						relativePath = "/" + locIndexes[i];
						break;
					}
				}
			}

			match.filePath = match.location->getRoot() + relativePath;

			std::cout << "PATH:" << match.filePath << std::endl; // DEBUG

			// Security check bound to the matched location root
			if (!isPathSafe(match.filePath, match.location->getRoot()))
			{
				match.statusCode = 403;
				return match;
			}

			// If the resolved path is a directory, try serving an index file inside it
			if (isDirectory(match.filePath))
			{
				const std::vector<std::string> &locIndexes = match.location->getIndex();
				for (size_t i = 0; i < locIndexes.size(); ++i)
				{
					std::string idxCandidate = match.filePath + "/" + locIndexes[i];
					if (fileExists(idxCandidate))
					{
						match.filePath = idxCandidate;
						break;
					}
				}
			}

			// Check if request should be handled by CGI
			std::string cgiExt = match.location->getCgiExtension();
			if (!cgiExt.empty())
			{
				try
				{
					if (cgiExt == getFileExtension(match.filePath))
						match.isCGI = true;
				}
				catch (const std::exception &e)
				{
					// If getFileExtension fails, not a CGI request
				}
			}

			// Check if file exists (skip for POST and CGI)
			if (match.statusCode == 200 && !match.isCGI && request.getMethod() != "POST" && (!fileExists(match.filePath) || (isDirectory(match.filePath) && !match.location->getAutoIndex())))
				match.statusCode = 404;
		}
	}
	return match;
}
