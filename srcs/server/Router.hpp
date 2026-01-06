/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 14:23:32 by gdosch            #+#    #+#             */
/*   Updated: 2026/01/06 13:54:03 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s)
#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include <string>

// Structure(s)
struct RouteMatch {

	const Location* location;
	std::string filePath;
	bool isRedirect;
	std::string redirectUrl;
	int statusCode;
	bool isCGI;
	std::string serverName;
	int serverPort;

};

// Class
class Router {

	public:
	
		// Public method(s)
	
			RouteMatch matchRoute(const ServerConfig& config, const HttpRequest& request) const;

	private:

		// Private method(s)
	
			const Location* findMatchingLocation(const ServerConfig& config, const std::string& uri) const;

};
