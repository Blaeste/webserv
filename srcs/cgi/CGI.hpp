/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:10 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/12 10:55:48 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s)
#include <string>
#include <map>

// Forward declaration(s)
class HttpRequest;
struct RouteMatch;

// Structure(s)
struct CGIResult {

	// Attribute(s)
	int statusCode;
	std::string output;
	std::string contentType;

	// Default constructor
	CGIResult() : statusCode(200), contentType("text/html") {}

};

// Class
class CGI {

	private:

		// Attribute(s)

		std::map<std::string, std::string> _env;

	public:

		// Public method(s)

		CGIResult execute(const RouteMatch& match, const HttpRequest& request);

	private:

		// Private method(s)

		void setupEnvironment(const RouteMatch& match, const HttpRequest &request);
		std::string readFromPipe(int fd);
		void parseHeaders(const std::string& output, CGIResult& result);


};
