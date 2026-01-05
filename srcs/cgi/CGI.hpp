/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:10 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/05 15:07:25 by eschwart         ###   ########.fr       */
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
	std::string contentType;

	CGIResult() : statusCode(200), contentType("text/html") {}
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
