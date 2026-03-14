/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/09 15:36:15 by gdosch            #+#    #+#             */
/*   Updated: 2026/03/14 21:04:50 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIGPARSER_HPP
# define CONFIGPARSER_HPP

// Include(s) ------------------------------------------------------------------

# include "ServerBlock.hpp"
# include <string>			// std::string
# include <vector>			// std::vector

// Structure(s) ----------------------------------------------------------------

struct	BlockInfo
{
	std::string	content;	// Block content without braces
	size_t		startPos;	// Start position in original file
	size_t		endPos;		// End position in original file
};

// Typedef(s) ------------------------------------------------------------------

typedef	std::vector<BlockInfo>		blockVector;

// Class -----------------------------------------------------------------------

class	ConfigParser
{
	private:

		// Constant(s)

		static const int			MAX_PORT_NUMBER = 65535;	// Highest valid TCP port number

		// Attribute(s)

		serverBlockVector			_servers;	// List of parsed server configurations
		std::string					_filePath;	// Path to the configuration file

		// Private Method(s)

		/** @brief Strips lines starting with '#' from the raw config content. */
		std::string					removeComments(const std::string& content) const;

		/** @brief Extracts all blocks matching keyword (e.g., "server", "location") with their positions. */
		blockVector					extractBlocks(const std::string& content, const std::string& keyword) const;

		/** @brief Parses a server block and fills the given ServerBlock. */
		void						parseServerBlock(const std::string& block, ServerBlock& server, size_t serverIndex) const;

		/** @brief Parses a location block and fills the given Location. */
		void						parseLocationBlock(const std::string& block, Location& location) const;

		/** @brief Validates the parsed configuration (ports, locations, etc.). */
		bool						validate() const;

		/**
		 * @brief Parses a human-readable size string into bytes.
		 * @param sizeStr Value with optional unit suffix: B, K, M, G (e.g., "10M", "500K").
		 */
		size_t						parseSize(const std::string& sizeStr, const std::string& context) const;


	public:

		// Getter(s)

		const serverBlockVector&	getServers() const	{ return _servers; }

		// Public Method(s)

		/** @brief Loads and parses the config file at the given path, returns false on error. */
		bool						parse(const std::string& filePath);

};

#endif
