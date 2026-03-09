/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:38 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/09 14:51:31 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVERCONFIG_HPP
# define SERVERCONFIG_HPP

// Include(s) ------------------------------------------------------------------

# include "Location.hpp"
# include <map>				// std::map
# include <string>			// std::string
# include <vector>			// std::vector

// Typedef(s) ------------------------------------------------------------------

typedef	std::map<int, std::string>	errorPageMap;

// Class -----------------------------------------------------------------------

class ServerConfig
{
	private:

		// Constant(s)

		enum {
			DEFAULT_PORT			= 8080,
			DEFAULT_MAX_BODY_SIZE	= 1048576,	// 1 MB
			DEFAULT_CGI_TIMEOUT		= 90		// 90 seconds
		};

		// Attribute(s)
		int						_port;			// Server port (e.g., 8080)
		std::string				_serverName;	// Server name (e.g., "localhost")
		errorPageMap			_errorPages;	// Error pages by status code
		size_t					_maxBodySize;	// Max request body size in bytes
		LocationVector			_locations;		// Location configurations
		size_t					_cgiTimeout;	// CGI execution timeout in seconds

	public:

		// Default constructor

		ServerConfig();

		// Getter(s)

		int						getPort() const									{ return _port; }
		const std::string&		getServerName() const							{ return _serverName; }
		const errorPageMap&		getErrorPages() const							{ return _errorPages; }
		size_t					getMaxBodySize() const							{ return _maxBodySize; }
		const LocationVector&	getLocations() const							{ return _locations; }
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

typedef	std::vector<ServerConfig>	serverVector;

#endif
