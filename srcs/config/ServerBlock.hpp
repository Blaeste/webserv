/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerBlock.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/09 15:36:32 by gdosch            #+#    #+#             */
/*   Updated: 2026/03/14 21:05:56 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERBLOCK_HPP
# define SERVERBLOCK_HPP

// Include(s) ------------------------------------------------------------------

# include "Location.hpp"
# include <map>				// std::map
# include <string>			// std::string
# include <vector>			// std::vector

// Typedef(s) ------------------------------------------------------------------

typedef	std::map<int, std::string>	errorPageMap;

// Class -----------------------------------------------------------------------

class	ServerBlock
{
	private:

		// Constant(s)

		static const int	DEFAULT_PORT			= 8080;		// Default listening port
		static const size_t	DEFAULT_MAX_BODY_SIZE	= 1048576;	// 1 MB
		static const size_t	DEFAULT_CGI_TIMEOUT		= 90;		// 90 seconds

		// Attribute(s)
		int						_port;			// Server port (e.g., 8080)
		std::string				_serverName;	// Server name (e.g., "localhost")
		errorPageMap			_errorPages;	// Error pages by status code
		size_t					_maxBodySize;	// Max request body size in bytes
		locationVector			_locations;		// Location configurations
		size_t					_cgiTimeout;	// CGI execution timeout in seconds

	public:

		// Default constructor

		ServerBlock();

		// Getter(s)

		int						getPort() const									{ return _port; }
		const std::string&		getServerName() const							{ return _serverName; }
		const errorPageMap&		getErrorPages() const							{ return _errorPages; }
		size_t					getMaxBodySize() const							{ return _maxBodySize; }
		const locationVector&	getLocations() const							{ return _locations; }
		size_t					getCgiTimeout() const							{ return _cgiTimeout; }

		std::string				getErrorPage(int code) const;

		// Setter(s)

		void					setPort(int port)								{ _port = port; }
		void					setServerName(const std::string& name)			{ _serverName = name; }
		void					addErrorPage(int code, const std::string& path)	{ _errorPages[code] = path; }
		void					setMaxBodySize(size_t size)						{ _maxBodySize = size; }
		void					addLocation(const Location& location)			{ _locations.push_back(location); }
		void					setCgiTimeout(size_t timeout)					{ _cgiTimeout = timeout; }
};

// Typedef(s) - class-dependent ------------------------------------------------

typedef	std::vector<ServerBlock>	serverBlockVector;

#endif
