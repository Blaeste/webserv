/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:17 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/09 15:07:02 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_HPP
# define CONFIG_HPP

// Include(s) ------------------------------------------------------------------

# include "ServerConfig.hpp"
# include <string>				// std::string
# include <vector>				// std::vector

// Typedef(s) ------------------------------------------------------------------

typedef	std::vector<BlockInfo>		blockVector;

// Structure(s) ----------------------------------------------------------------

struct	BlockInfo
{
	std::string	content;	// Block content without braces
	size_t		startPos;	// Start position in original file
	size_t		endPos;		// End position in original file
};

// Class -----------------------------------------------------------------------

class	Config
{
	private:

		// Constant(s)

		enum { MAX_PORT_NUMBER = 65535 };

		// Attribute(s)

		serverVector		_servers;	// List of parsed server configurations
		std::string			_filePath;	// Path to the configuration file

		// Private Method(s)

		/** @brief Strips lines starting with '#' from the raw config content. */
		std::string			removeComments(const std::string& content);

		/** @brief Extracts all blocks matching keyword (e.g., "server", "location") with their positions. */
		blockVector			extractBlocks(const std::string& content, const std::string& keyword);

		/** @brief Parses a server block and fills the given ServerConfig. */
		void				parseServerBlock(const std::string& block, ServerConfig& server, size_t serverIndex);

		/** @brief Parses a location block and fills the given Location. */
		void				parseLocationBlock(const std::string& block, Location& location);

		/** @brief Validates the parsed configuration (ports, locations, etc.). */
		bool				validate() const;

		/**
		 * @brief Parses a human-readable size string into bytes.
		 * @param sizeStr Value with optional unit suffix: B, K, M, G (e.g., "10M", "500K").
		 */
		size_t				parseSize(const std::string& sizeStr, const std::string& context);


	public:

		// Getter(s)

		const serverVector&	getServers() const	{ return _servers; }

		// Public Method(s)

		/** @brief Loads and parses the config file at the given path, returns false on error. */
		bool				parse(const std::string& filePath);

};

#endif
