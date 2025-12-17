/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:29 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/17 14:30:32 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// =============================================================================
// Includes

#include "ServerConfig.hpp"


// =============================================================================
// Handlers

// Default constructor
ServerConfig::ServerConfig() :
	_port(8080), // Default port
	_serverName("localhost"), // Default name
	_maxBodySize(1048576) // Default size (1MB)
{
}

// Destructor
ServerConfig::~ServerConfig()
{
}


// =============================================================================
// Setters

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


// =============================================================================
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

	return ""; // TODO: throw exception
}

size_t ServerConfig::getMaxBodySize() const
{
	return _maxBodySize;
}

const std::vector<Location> &ServerConfig::getLocations() const
{
	return _locations;
}


// =============================================================================
// Methods

const Location *ServerConfig::matchLocation(const std::string &uri) const
{
	const Location *bestMatch = NULL;
	size_t bestLength = 0;

	for (size_t i = 0; i < _locations.size(); i++)
	{
		const std::string &path = _locations[i].getPath();

		// if URI match path
		if (uri.find(path) == 0)
		{
			// and length > bestLength better match
			if (path.length() > bestLength)
			{
				bestMatch = &_locations[i];
				bestLength = path.length();
			}
		}
	}

	return bestMatch; // Can be NULL if no match!
}
