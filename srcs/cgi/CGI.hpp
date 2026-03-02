/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:10 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/02 15:47:32 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s) ******************************************************************
#include <ctime>		// time_t, std::time
#include <map>			// std::map
#include <string>		// std::string
#include <sys/types.h>	// pid_t
#include <vector>		// std::vector

// Forward declaration(s) ------------------------------------------------------
class HttpRequest;
struct RouteMatch;

// Structure(s) ****************************************************************
struct CGIProcess {
	// Attribute(s) ------------------------------------------------------------
	pid_t pid; ///< Process ID of the CGI script
	int pipeOut; ///< File descriptor to read stdout from CGI
	int pipeIn; ///< File descriptor to write stdin (for POST body)
	int pipeErr; ///< File descriptor to read stderr from CGI
	time_t startTime; ///< Timestamp when CGI process started
	std::string output; ///< Accumulated stdout output from CGI
	std::string errorOutput; ///< Accumulated stderr output from CGI
	bool inputWritten; ///< Indicates if POST body was fully written
	size_t bytesWritten; ///< Number of bytes written so far to stdin
	time_t executionTimeout; ///< Maximum execution time in seconds

	// Default constructor -----------------------------------------------------
	CGIProcess()
		: pid(-1)
		, pipeOut(-1)
		, pipeIn(-1)
		, pipeErr(-1)
		, startTime(0)
		, inputWritten(false)
		, bytesWritten(0)
	{}
};

struct CGIResult {
	// Attribute(s) ------------------------------------------------------------
	int statusCode; ///< HTTP status code from CGI response
	std::string output; ///< Response body from CGI script
	std::string contentType; ///< Content-Type header from CGI response

	// Default constructor -----------------------------------------------------
	CGIResult() : statusCode(200), contentType("text/html") {}
};

// Class ***********************************************************************
class CGI {
	private:
		// Attribute(s) --------------------------------------------------------
		std::map<std::string, std::string> _env; ///< Environment variables for CGI execution

	public:
		// Public method(s) ----------------------------------------------------
		void parseHeaders(const std::string& output, CGIResult& result);
		CGIProcess* startAsync(const RouteMatch& match, const HttpRequest& request, const std::vector<int>& fdsToClose);

	private:
		// Private method(s) ---------------------------------------------------
		void setupEnvironment(const RouteMatch& match, const HttpRequest &request);
		std::string readFromPipe(int fd);
};
