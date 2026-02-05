/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:29 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/05 13:10:30 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "ServerConfig.hpp"

// Default constructor
ServerConfig::ServerConfig()
	: _port(8080) // Default port
	, _serverName("localhost") // Default name
	, _maxBodySize(1048576) // Default size (1MB)
	, _cgiTimeout(90)
{}

// Setter(s)
void ServerConfig::setPort(int port) {
	_port = port;
}

void ServerConfig::setServerName(const std::string &name) {
	_serverName = name;
}

void ServerConfig::addErrorPage(int code, const std::string &path) {
	_errorPages[code] = path;
}

void ServerConfig::setMaxBodySize(size_t size) {
	_maxBodySize = size;
}

void ServerConfig::addLocation(const Location &location) {
	_locations.push_back(location);
}

void ServerConfig::setCgiTimeout(size_t timeout) {
	_cgiTimeout = timeout;
}

// Getters
int ServerConfig::getPort() const {
	return _port;
}

const std::string &ServerConfig::getServerName() const {
	return _serverName;
}

const std::map<int, std::string> &ServerConfig::getErrorPages() const {
	return _errorPages;
}

std::string ServerConfig::getErrorPage(int code) const {
	const std::map<int, std::string>::const_iterator it = _errorPages.find(code);
	if (it != _errorPages.end())
		return it->second;
	return "";
}

size_t ServerConfig::getMaxBodySize() const {
	return _maxBodySize;
}

const std::vector<Location> &ServerConfig::getLocations() const {
	return _locations;
}

size_t ServerConfig::getCgiTimeout() const {
	return _cgiTimeout;
}
