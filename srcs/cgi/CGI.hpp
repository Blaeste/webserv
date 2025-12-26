/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:10 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/26 15:03:29 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "../http/HttpRequest.hpp"
#include "../server/Router.hpp"
#include <string>
#include <map>

struct CGIResult {

	int statusCode;
	std::string output;

};

class CGI {

	private:

		// TODO: std::string _scriptPath;
		// TODO: std::string _cgiPath;
		std::map<std::string, std::string> _env;

	public:

		CGI();
		~CGI();

		CGIResult execute(const RouteMatch& match, const HttpRequest& request);

	private:

		void setupEnvironment(const RouteMatch& match, const HttpRequest &request);
		std::string readFromPipe(int fd);
		void parseHeaders(const std::string& output, CGIResult& result);
		// TODO: void writeToPipe(int fd, const std::string &data);

};
