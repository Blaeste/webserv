/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:17 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/06 12:17:38 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Includes
#include "ServerConfig.hpp"
#include <string>
#include <vector>

// Typedef
typedef std::vector<std::string> SVector;


class Config
{

private:

	// Attributes
	std::vector<ServerConfig> _servers;
	std::string _filePath;

public:

	// Public Methods

		/**
		 * @brief Parses the configuration file at the given path.
		 * @param filePath The path to the configuration file.
		 * @return true if parsing was successful, false otherwise.
		 */
		bool parse(const std::string &filePath);

		/**
		 * @brief Gets the list of configured servers.
		 * @return A constant reference to a vector of ServerConfig objects.
		 */
		const std::vector<ServerConfig> &getServers() const;

private:

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
