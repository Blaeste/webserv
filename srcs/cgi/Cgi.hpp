/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:10 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/14 15:03:42 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CGI_HPP
# define CGI_HPP

// Include(s) ------------------------------------------------------------------

# include "../webserv.hpp"
# include <map>				// std::map
# include <string>			// std::string
# include <vector>			// std::vector
# include <ctime>			// time_t, std::time
# include <sys/types.h>		// pid_t

// Forward declaration(s) ------------------------------------------------------

class	HttpRequest;
struct	RouteMatch;

// Typedef(s) ------------------------------------------------------------------

typedef	std::map<std::string, std::string>	envMap;

// Structure(s) ----------------------------------------------------------------

struct	CgiProcess
{
	// Attribute(s)
	pid_t		pid;				// Process ID of the CGI script
	int			pipeOut;			// File descriptor to read stdout from CGI
	int			pipeIn;				// File descriptor to write stdin (for POST body)
	int			pipeErr;			// File descriptor to read stderr from CGI
	time_t		startTime;			// Timestamp when CGI process started
	std::string	output;				// Accumulated stdout output from CGI
	std::string	errorOutput;		// Accumulated stderr output from CGI
	bool		inputWritten;		// Indicates if POST body was fully written
	size_t		bytesWritten;		// Number of bytes written so far to stdin
	time_t		executionTimeout;	// Maximum execution time in seconds

	// Default constructor
	CgiProcess()
		: pid(-1)
		, pipeOut(-1)
		, pipeIn(-1)
		, pipeErr(-1)
		, startTime(0)
		, inputWritten(false)
		, bytesWritten(0)
	{}
};

struct	CgiResult
{
	// Attribute(s)
	int			statusCode;		// HTTP status code from CGI response
	std::string	output;			// Response body from CGI script
	std::string	contentType;	// Content-Type header from CGI response

	// Default constructor
	CgiResult()
		: statusCode(200)
		, contentType("text/html")
	{}
};

// Class -----------------------------------------------------------------------

class	Cgi
{
	private:

		// Attribute(s)

		envMap 				_env;	// Environment variables for CGI execution

		// Private method(s)

		/** @brief Populates _env with CGI/1.1 variables derived from the request and route. */
		void				setupEnvironment(const RouteMatch& match, const HttpRequest& request);

	public:

		// Public method(s)

		/** @brief Parses CGI response headers (Status, Content-Type) and splits body into result. */
		static void			parseHeaders(const std::string& output, CgiResult& result);

		/**
		 * @brief Forks a CGI process, sets up stdin/stdout/stderr pipes, and returns the process handle.
		 * @param fdsToClose List of server fds to close in the child to prevent fd leaks across CGI processes.
		 * @return Heap-allocated CgiProcess on success, NULL on fork/pipe/access failure.
		 */
		CgiProcess*			startAsync(const RouteMatch& match, const HttpRequest& request, const std::vector<int>& fdsToClose);
};

#endif
