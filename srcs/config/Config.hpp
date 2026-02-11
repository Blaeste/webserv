/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:17 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/11 13:26:32 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s) ******************************************************************
#include "ServerConfig.hpp"
#include <string>
#include <vector>

/**
 * @brief Represents a configuration block with its content and position.
 * Used during parsing to track block locations in the config file.
 */
struct BlockInfo {
	std::string content; ///< Block content without braces
	size_t startPos; ///< Start position in original file
	size_t endPos; ///< End position in original file
};

// Class ***********************************************************************
class Config
{
	private:
		// Attribute(s) --------------------------------------------------------

		std::vector<ServerConfig> _servers; ///< List of parsed server configurations
		std::string _filePath; ///< Path to the configuration file

	public:
		// Public Method(s) ----------------------------------------------------

		/**
		 * @brief Parses the configuration file at the given path.
		 * @param filePath The path to the configuration file.
		 * @return true if parsing was successful, false otherwise.
		 */
		bool parse(const std::string &filePath);

		// Getter(s) -----------------------------------------------------------
		const std::vector<ServerConfig> &getServers() const { return _servers; }

	private:
		// Private Method(s) ---------------------------------------------------

		/**
		 * @brief Removes comments (lines starting with #) from content.
		 * @param content The configuration file content.
		 * @return Content without comments.
		 */
		std::string removeComments(const std::string &content);

		/**
		 * @brief Extracts blocks of configuration from the content based on the given keyword.
		 * @param content The full configuration content as a string.
		 * @param keyword The keyword to identify blocks (e.g., "server", "location").
		 * @return A vector of strings, each representing a configuration block.
		 */
		std::vector<BlockInfo> extractBlocks(const std::string &content, const std::string &keyword);

		/**
		 * @brief Parses a server configuration block and populates the ServerConfig object.
		 * @param block The server configuration block as a string.
		 * @param server The ServerConfig object to populate.
		 */
		void parseServerBlock(const std::string &block, ServerConfig &server, size_t serverIndex);

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
		 * @param context the context for log error.
		 * @return The size in bytes.
		 */
		size_t parseSize(const std::string &sizeStr, const std::string &context);
};
