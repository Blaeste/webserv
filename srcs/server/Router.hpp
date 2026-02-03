/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lmarck <lmarck@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 14:23:32 by gdosch            #+#    #+#             */
/*   Updated: 2026/02/03 12:36:11 by lmarck           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s)
#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include <string>

// Structure(s)
struct RouteMatch
{

	RouteMatch()
		: location(NULL), filePath(), pathInfo(), isRedirect(false), redirectUrl(), statusCode(200), isCGI(false), serverName(), serverPort(0)
	{
	}

	const Location *location;
	std::string filePath;
	std::string pathInfo; // Relative path for CGI PATH_INFO
	bool isRedirect;
	std::string redirectUrl;
	int statusCode;
	bool isCGI;
	std::string serverName;
	int serverPort;
};

// Class
class Router
{

public:
	// Public method(s)

	RouteMatch matchRoute(const ServerConfig &config, const HttpRequest &request) const;

private:
	// Private method(s)

	const Location *findMatchingLocation(const ServerConfig &config, const std::string &uri) const;
};
