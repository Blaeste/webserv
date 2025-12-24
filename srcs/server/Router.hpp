/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Router.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 14:23:32 by gdosch            #+#    #+#             */
/*   Updated: 2025/12/24 18:47:50 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "../config/ServerConfig.hpp"
#include "../http/HttpRequest.hpp"
#include <string>

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

class Router {

	private:

		// const ServerConfig& _config;

		const Location* findMatchingLocation(const ServerConfig& config, const std::string& uri) const;
		bool isMethodAllowed(const Location& loc, const std::string& method) const;

	public:
	
		Router();
		~Router();

		RouteMatch matchRoute(const ServerConfig& config, const HttpRequest& request) const;

};
