/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:10 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/06 11:46:01 by gdosch           ###   ########.fr       */
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

		std::map<std::string, std::string> _env;

	public:

		CGIResult execute(const RouteMatch& match, const HttpRequest& request);

	private:

		void setupEnvironment(const RouteMatch& match, const HttpRequest &request);
		std::string readFromPipe(int fd);
		void parseHeaders(const std::string& output, CGIResult& result);

};
