/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:17 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/18 14:49:13 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Parse le fichier de config
// Contient vector<ServerConfig>
// Valide la configuration

// =============================================================================
// Includes

#pragma once

#include <string>
#include <vector>

#include "ServerConfig.hpp"

// =============================================================================
// Typedef

typedef std::vector<std::string> SVector;

// =============================================================================
// Defines

// =============================================================================
// Class Config

class Config
{
private:

	// =========================================================================
	// Attributs

	// Liste des serveurs
	std::vector<ServerConfig> _servers;

	// Path du fichier de config
	std::string _filePath;

public:

	// =========================================================================
	// Handlers

	// Default constructor
	Config();

	// Destructor
	~Config();

	// =========================================================================
	// Public Methods

	/**
	 * @brief Parses the configuration file at the given path.
	 * @param filePath The path to the configuration file.
	 * @return true if parsing was successful, false otherwise.
	 */
	bool parse(const std::string &filePath);

	// =========================================================================
	// Getters

	/**
	 * @brief Gets the list of configured servers.
	 * @return A constant reference to a vector of ServerConfig objects.
	 */
	const std::vector<ServerConfig> &getServers() const;

private:

	// =========================================================================
	// Private Methods

	/**
	 * @brief Extracts blocks of configuration from the content based on the given keyword.
	 * @param content The full configuration content as a string.
	 * @param keyword The keyword to identify blocks (e.g., "server", "location").
	 * @return A vector of strings, each representing a configuration block.
	 */
	std::vector<std::string> extractBlocks(const std::string &content, const std::string &keyword);

	/**
	 * @brief Parses a server configuration block and populates the ServerConfig object.
	 * @param block The server configuration block as a string.
	 * @param server The ServerConfig object to populate.
	 */
	void parseServerBlock(const std::string &block, ServerConfig &server);

	/**
	 * @brief Parses a location configuration block and populates the Location object.
	 * @param block The location configuration block as a string.
	 * @param location The Location object to populate.
	 */
	void parseLocationBlock(const std::string &block, Location &location);

	/**
	 * @brief Validates the parsed configuration for correctness.
	 * @return true if the configuration is valid, false otherwise.
	 */
	bool validate() const;

	/**
	 * @brief Parses a size string with optional units (e.g., "10M", "500K") into bytes.
	 * @param sizeStr The size string to parse.
	 * @return The size in bytes.
	 */
	size_t parseSize(const std::string &sizeStr);
};
