/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:38 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/17 14:26:27 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Port decoute
// Server name
// Error pages
// Max body size
// vector<Location> locations

// =============================================================================
// Includes

#pragma once

#include <string>
#include <vector>
#include <map>

#include "Location.hpp"

// =============================================================================
// Typedef

// =============================================================================
// Defines

// =============================================================================
// Class ServerConfig

class ServerConfig
{

private:
	// =========================================================================
	// Attributs

	// 8080
	int _port;

	// "localhost"
	std::string _serverName;

	// {404: "/error_pages/404.html", 500: ...}
	std::map<int, std::string> _errorPages;

	// 10485760 (10M en bytes)
	size_t _maxBodySize;

	// Liste des locations
	std::vector<Location> _locations;

public:
	// =========================================================================
	// Handlers
	ServerConfig();
	~ServerConfig();

	// =========================================================================
	// Setters => set replace val, add add val
	void setPort(int port);
	void setServerName(const std::string &name);
	void addErrorPage(int code, const std::string &path);
	void setMaxBodySize(size_t size);
	void addLocation(const Location &location);

	// =========================================================================
	// Getters
	int getPort() const;
	const std::string &getServerName() const;
	const std::map<int, std::string> &getErrorPages() const;
	std::string getErrorPage(int code) const;
	size_t getMaxBodySize() const;
	const std::vector<Location> &getLocations() const;

	// =========================================================================
	// Methods
	const Location *matchLocation(const std::string &uri) const;
};

