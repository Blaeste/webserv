/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 14:23:30 by gdosch            #+#    #+#             */
/*   Updated: 2026/01/16 12:28:38 by gdosch           ###   ########.fr       */
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

		// Special case: if location path ends with '/', also match URI without trailing slash
		// e.g., location "/directory/" should match URI "/directory"
		bool isMatch = false;
		if (path[path.length() - 1] == '/' && path.length() > 1)
		{
			std::string pathWithoutSlash = path.substr(0, path.length() - 1);
			if (uri == pathWithoutSlash)
				isMatch = true;
		}
		
		// Check if URI starts with location path
		// Must be exact match or followed by '/' to avoid false matches
		// e.g., /cgi-bin/php should not match /cgi-bin/py
		if (!isMatch && uri.find(path) == 0)
		{
			// Ensure it's a valid path prefix (either exact match or followed by '/')
			if (uri.length() == path.length() || uri[path.length()] == '/' || path[path.length() - 1] == '/')
				isMatch = true;
		}
		
		if (isMatch && path.length() > longestMatch)
		{
			std::cerr << "[DEBUG findMatchingLocation] URI '" << uri << "' matches location '" << path << "'" << std::endl;
			longestMatch = path.length();
			bestMatch = &locations[i];
		}
	}
	if (bestMatch)
		std::cerr << "[DEBUG findMatchingLocation] Best match for URI '" << uri << "' is '" << bestMatch->getPath() << "'" << std::endl;
	else
		std::cerr << "[DEBUG findMatchingLocation] No match found for URI '" << uri << "'" << std::endl;
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
	{
		std::cerr << "[DEBUG Router] No location matched for URI: " << uri << std::endl;
		match.statusCode = 404;
	}
	else
	{
		std::cerr << "[DEBUG Router] Matched location: " << match.location->getPath() << " for URI: " << uri << std::endl;
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
			if (uri == "/" || uri.empty())
			{
				const std::vector<std::string> &indexes = match.location->getIndex();
				for (size_t i = 0; i < indexes.size(); i++)
				{
					std::string indexPath = match.location->getRoot() + "/" + indexes[i];
					if (fileExists(indexPath))
					{
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

		// Strip location path from URI to get relative path
		std::string relativePath = pathPart;
		const std::string &locationPath = match.location->getPath();
		std::cerr << "[DEBUG Router] pathPart: '" << pathPart << "', locationPath: '" << locationPath << "'" << std::endl;
		
		// Handle location paths with trailing slash
		std::string locationPathToMatch = locationPath;
		if (locationPath.length() > 1 && locationPath[locationPath.length() - 1] == '/')
			locationPathToMatch = locationPath.substr(0, locationPath.length() - 1);
		
		if (pathPart.find(locationPathToMatch) == 0)
		{
			relativePath = pathPart.substr(locationPathToMatch.length());
			std::cerr << "[DEBUG Router] After stripping location path, relativePath: '" << relativePath << "'" << std::endl;
			// If relativePath is empty or doesn't start with '/', it means we're at the location root
			if (relativePath.empty() || relativePath[0] != '/')
				relativePath = "/" + relativePath;
			std::cerr << "[DEBUG Router] After normalization, relativePath: '" << relativePath << "'" << std::endl;
		}

		// Build full file path
		match.filePath = match.location->getRoot() + relativePath;
		std::cerr << "[DEBUG Router] Final filePath: '" << match.filePath << "' (root: '" << match.location->getRoot() << "')" << std::endl;

		// Security check
		if (!isPathSafe(match.filePath))
		{
			std::cerr << "[DEBUG Router] Path NOT SAFE: " << match.filePath << std::endl;
			match.statusCode = 403;
			return match;
		}
		std::cerr << "[DEBUG Router] Path is SAFE" << std::endl;

		// If filePath is a directory, try index files
		if (isDirectory(match.filePath))
		{
			std::cerr << "[DEBUG Router] filePath is a directory, trying index files" << std::endl;
			const std::vector<std::string> &indexes = match.location->getIndex();
			bool indexFound = false;
			for (size_t i = 0; i < indexes.size(); i++)
			{
				std::string indexPath = match.filePath + indexes[i];
				std::cerr << "[DEBUG Router] Trying index: " << indexPath << std::endl;
				if (fileExists(indexPath))
				{
					std::cerr << "[DEBUG Router] Index file found: " << indexPath << std::endl;
					match.filePath = indexPath;
					indexFound = true;
					break;
				}
			}
			if (!indexFound)
				std::cerr << "[DEBUG Router] No index file found" << std::endl;
		}

		// Check if request should be handled by CGI
			std::string cgiExt = match.location->getCgiExtension();
			if (!cgiExt.empty())
			{
				try {
					if (cgiExt == getFileExtension(match.filePath))
						match.isCGI = true;
				} catch (const std::exception &e) {
					// If getFileExtension fails, not a CGI request
				}
			}

			// Check if file exists (skip for POST and CGI)
		if (match.statusCode == 200 && !match.isCGI && request.getMethod() != "POST")
		{
			std::cerr << "[DEBUG Router] Checking file existence for: " << match.filePath << std::endl;
			bool exists = fileExists(match.filePath);
			bool isDir = isDirectory(match.filePath);
			std::cerr << "[DEBUG Router] fileExists: " << exists << ", isDirectory: " << isDir << std::endl;
			
			// Directory without trailing slash → redirect to add slash
			if (isDir && uri[uri.length() - 1] != '/')
			{
				std::cerr << "[DEBUG Router] Directory without slash, redirecting to: " << uri + "/" << std::endl;
				match.isRedirect = true;
				match.statusCode = 301;
				match.redirectUrl = uri + "/";
			}
			// File doesn't exist or directory without autoindex
			else if (!exists || (isDir && !match.location->getAutoIndex()))
			{
				std::cerr << "[DEBUG Router] Setting 404: exists=" << exists << ", isDir=" << isDir << ", autoindex=" << match.location->getAutoIndex() << std::endl;
				match.statusCode = 404;
			}
		}
	}
	}
	return match;
}
