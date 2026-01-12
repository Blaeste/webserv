/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:29 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/12 11:40:44 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "ServerConfig.hpp"

// Default constructor
ServerConfig::ServerConfig() :
	_port(8080), // Default port
	_serverName("localhost"), // Default name
	_maxBodySize(1048576) // Default size (1MB)
{
}

// Setter(s)
void ServerConfig::setPort(int port)
{
	_port = port;
}

void ServerConfig::setServerName(const std::string &name)
{
	_serverName = name;
}

void ServerConfig::addErrorPage(int code, const std::string &path)
{
	_errorPages[code] = path;
}

void ServerConfig::setMaxBodySize(size_t size)
{
	_maxBodySize = size;
}

void ServerConfig::addLocation(const Location &location)
{
	_locations.push_back(location);
}

// Getters
int ServerConfig::getPort() const
{
	return _port;
}

const std::string &ServerConfig::getServerName() const
{
	return _serverName;
}

const std::map<int, std::string> &ServerConfig::getErrorPages() const
{
	return _errorPages;
}

std::string ServerConfig::getErrorPage(int code) const
{
	const std::map<int, std::string>::const_iterator it = _errorPages.find(code);

	if (it != _errorPages.end())
		return it->second;

	return "";
}

size_t ServerConfig::getMaxBodySize() const
{
	return _maxBodySize;
}

const std::vector<Location> &ServerConfig::getLocations() const
{
	return _locations;
}

// Public method(s)
const Location *ServerConfig::matchLocation(const std::string &uri) const
{
	const Location *bestMatch = NULL;
	size_t bestLength = 0;

	// Check all config locations
	for (size_t i = 0; i < _locations.size(); i++)
	{
		const std::string &path = _locations[i].getPath();

		// if URI start with path of location
		// URI = "/uploads/file.txt" => "/uploads"
		if (uri.find(path) == 0)
		{
			size_t pathLen = path.length();

			// CASE 1 exact match
			if (uri.length() == pathLen)
			{
				// keep the longest match
				if (pathLen > bestLength)
				{
					bestMatch = &_locations[i];
					bestLength = pathLen;
				}
			}

			// CASE 2 uri longer than path, check follow /
			else if (uri[pathLen] == '/')
			{
				if (pathLen > bestLength)
				{
					bestMatch = &_locations[i];
					bestLength = pathLen;
				}
			}
		}
	}

	return bestMatch; // Can be NULL if no match!
}
