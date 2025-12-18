/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:38 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/18 09:53:18 by eschwart         ###   ########.fr       */
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

	// Default constructor
	ServerConfig();

	// Destructor
	~ServerConfig();

	// =========================================================================
	// Setters => set replace val, add add val

	/**
	 * @brief Sets the port number the server listens on.
	 * @param port The port number.
	 */
	void setPort(int port);

	/**
	 * @brief Sets the server name.
	 * @param name The server name.
	 */
	void setServerName(const std::string &name);

	/**
	 * @brief Adds an error page for a specific HTTP status code.
	 * @param code The HTTP status code.
	 * @param path The path to the error page.
	 */
	void addErrorPage(int code, const std::string &path);

	/**
	 * @brief Sets the maximum allowed body size for requests.
	 * @param size The maximum body size in bytes.
	 */
	void setMaxBodySize(size_t size);

	/**
	 * @brief Adds a Location to the server configuration.
	 * @param location The Location object to add.
	 */
	void addLocation(const Location &location);

	// =========================================================================
	// Getters

	/**
	 * @brief Gets the port number the server listens on.
	 * @return The port number.
	 */
	int getPort() const;

	/**
	 * @brief Gets the server name.
	 * @return The server name as a string.
	 */
	const std::string &getServerName() const;

	/**
	 * @brief Gets the map of error pages.
	 * @return A constant reference to the map of error pages.
	 */
	const std::map<int, std::string> &getErrorPages() const;

	/**
	 * @brief Gets the error page path for a specific HTTP status code.
	 * @param code The HTTP status code.
	 * @return The path to the error page, or an empty string if not found.
	 */
	std::string getErrorPage(int code) const;

	/**
	 * @brief Gets the maximum allowed body size for requests.
	 * @return The maximum body size in bytes.
	 */
	size_t getMaxBodySize() const;

	/**
	 * @brief Gets the list of configured locations.
	 * @return A constant reference to a vector of Location objects.
	 */
	const std::vector<Location> &getLocations() const;

	// =========================================================================
	// Methods

	/**
	 * @brief Matches a URI to the most appropriate Location.
	 * @param uri The request URI.
	 * @return A pointer to the matched Location, or nullptr if no match is found.
	 */
	const Location *matchLocation(const std::string &uri) const;
};

