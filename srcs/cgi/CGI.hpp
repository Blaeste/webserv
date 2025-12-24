/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:10 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/24 17:26:21 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Fork + exec du script
// Prepare lenvironnement
// Gere les pipes stdin/stdout
// Timeout du CGI

#pragma once

#include "../http/HttpRequest.hpp"
#include "../server/Router.hpp"
#include <string>
#include <map>

class CGI {

	private:

		// TODO: std::string _scriptPath;
		// TODO: std::string _cgiPath;
		std::map<std::string, std::string> _env;

	public:

		CGI();
		~CGI();

		std::string execute(const RouteMatch& match, const HttpRequest& request);

	private:

		void setupEnvironment(const RouteMatch& match, const HttpRequest &request);
		std::string readFromPipe(int fd);
		// TODO: void writeToPipe(int fd, const std::string &data);

};
