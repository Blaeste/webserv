/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 14:23:32 by gdosch            #+#    #+#             */
/*   Updated: 2026/03/14 22:50:38 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ROUTER_HPP
# define ROUTER_HPP

// Include(s) ------------------------------------------------------------------

# include <string>	// std::string

// Forward declaration(s) ------------------------------------------------------

class	HttpRequest;
class	Location;
class	ServerBlock;

// Structure(s) ----------------------------------------------------------------

struct	RouteMatch
{
	// Attribute(s)
	const Location*	location;		// Matched location configuration
	std::string		filePath;		// Resolved file path for the request
	std::string		pathInfo;		// Relative path for CGI PATH_INFO
	bool			isRedirect;		// Indicates if this is a redirect response
	std::string		redirectUrl;	// Redirect destination URL
	int				statusCode;		// HTTP status code for the response
	bool			isCGI;			// Indicates if this request should be handled by CGI
	std::string		serverName;		// Server name for this request
	int				serverPort;		// Server port for this request

	// Default constructor
	RouteMatch()
		: location(NULL)
		, filePath()
		, pathInfo()
		, isRedirect(false)
		, redirectUrl()
		, statusCode(200)
		, isCGI(false)
		, serverName()
		, serverPort(0)
	{}
};

// Class -----------------------------------------------------------------------

class	Router
{
	private:

		// Private method(s)

		/** @brief Returns the location block with the longest matching prefix for uri, or NULL. */
		const Location*	findMatchingLocation(const ServerBlock& server, const std::string& uri) const;

	public:

		// Public method(s)

		/** @brief Resolves request to a RouteMatch (file path, CGI flag, status code, etc.). */
		RouteMatch		matchRoute(const ServerBlock& server, const HttpRequest& request) const;


};

#endif
