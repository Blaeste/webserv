/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:10 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/05 11:51:31 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s)
#include <ctime>
#include <map>
#include <string>
#include <sys/types.h>

// Forward declaration(s)
class HttpRequest;
struct RouteMatch;

// Structure(s)
struct CGIProcess {

	// Attribute(s)
	pid_t pid;
	int pipeOut; // fd to read stdout from CGI
	int pipeIn; // fd to write stdin (for POST body)
	int pipeErr; // fd to read stderr from CGI
	time_t startTime;
	std::string output;
	std::string errorOutput; // stderr from CGI
	bool inputWritten; // true when POST body fully written
	size_t bytesWritten; // Number of bytes written so far
	time_t executionTimeout;

	// Default constructor
	CGIProcess() : pid(-1), pipeOut(-1), pipeIn(-1), pipeErr(-1), startTime(0), inputWritten(false), bytesWritten(0) {}

};

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

			void parseHeaders(const std::string& output, CGIResult& result);
			CGIProcess* startAsync(const RouteMatch& match, const HttpRequest& request);

	private:

		// Private method(s)

			void setupEnvironment(const RouteMatch& match, const HttpRequest &request);
			std::string readFromPipe(int fd);

};
