/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 14:23:32 by gdosch            #+#    #+#             */
/*   Updated: 2026/02/12 10:28:06 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s) ******************************************************************
#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include <string>

// Structure(s) ****************************************************************
struct RouteMatch
{
	// Default constructor -----------------------------------------------------
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

	// Attribute(s) ------------------------------------------------------------
	const Location *location; ///< Matched location configuration
	std::string filePath; ///< Resolved file path for the request
	std::string pathInfo; ///< Relative path for CGI PATH_INFO
	bool isRedirect; ///< Indicates if this is a redirect response
	std::string redirectUrl; ///< Redirect destination URL
	int statusCode; ///< HTTP status code for the response
	bool isCGI; ///< Indicates if this request should be handled by CGI
	std::string serverName; ///< Server name for this request
	int serverPort; ///< Server port for this request
};

// Class ***********************************************************************
class Router
{
	public:
		// Public method(s) ----------------------------------------------------
		RouteMatch matchRoute(const ServerConfig &config, const HttpRequest &request) const;

	private:
		// Private method(s) ---------------------------------------------------
		const Location *findMatchingLocation(const ServerConfig &config, const std::string &uri) const;
};
