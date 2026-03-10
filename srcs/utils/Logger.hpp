/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/10 11:29:10 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOGGER_HPP
# define LOGGER_HPP

// Include(s) ------------------------------------------------------------------

# include <limits>	// std::numeric_limits
# include <map>		// std::map
# include <string>	// std::string
# include <ctime>	// time_t

// Define(s) -------------------------------------------------------------------

#define RESET		"\033[0m"
#define RED			"\033[31m"
#define GREEN		"\033[32m"
#define YELLOW		"\033[33m"
#define CYAN		"\033[36m"
#define GREY		"\033[90m"
#define BOLD		"\033[1m"
#define CLEARLINE	"\033[K"

// Structure(s) ----------------------------------------------------------------

struct RequestData
{
	std::string		method;				// HTTP method (GET, POST, etc.)
	std::string		uri;				// Request URI path
	std::string		clientIP;			// Client IP address
	std::string		serverName;			// Matched server name
	int				serverPort;			// Listening port
	size_t			declaredSize;		// Content-Length from request headers
	std::string		requestStartTime;	// Timestamp when request was received
	int				displayLine;		// Terminal line number for in-place update
};

// Typedef(s) ------------------------------------------------------------------

typedef	std::map<int, RequestData>	requestMap;

// Class -----------------------------------------------------------------------

class Logger
{
	private:

		// Constant(s)

		enum {
			TIME_BUFFER_SIZE			= 10,				// "HH:MM:SS\0" fits in 10 bytes
			LOG_SEPARATOR_WIDTH			= 145,				// Dash separator line width in characters
			SERVER_PORT_FIELD_WIDTH		= 20,				// Column width for "server:port"
			IP_FIELD_WIDTH				= 15,				// Column width for client IP address
			METHOD_FIELD_WIDTH			= 7,				// Column width for HTTP method (e.g., "DELETE")
			URI_FIELD_WIDTH				= 55,				// Column width for request URI
			RESPONSE_SIZE_FIELD_WIDTH	= 4,				// Column width for response body size
			STATUS_FIELD_WIDTH			= 5					// Column width for HTTP status code
		};

		// Attribute(s)

		static requestMap		_activeRequests;			// Track each request's data by socket
		static std::string		_lastMethod;				// Last logged HTTP method
		static std::string		_lastUri;					// Last logged URI
		static std::string		_lastClientIP;				// Last logged client IP address
		static size_t			_requestCount;				// Number of grouped identical requests
		static time_t			_minTime;					// Minimum response time in group
		static time_t			_maxTime;					// Maximum response time in group
		static std::string		_lastServerName;			// Last logged server name
		static int				_lastServerPort;			// Last logged server port
		static size_t			_lastEndRequestSize;		// Last completed request body size
		static int				_lastEndStatus;				// Last completed response status code
		static size_t			_groupEndCount;				// Number of completions in current visual group
		static bool				_firstLog;					// Indicates if this is the first log entry
		static bool				_pendingRequest;			// Request started but not completed
		static std::string		_lastRequestStartTime;		// Store timestamp from logRequestStart
		static int				_lastDisplayedRequestId;	// Track which request displayed the last line
		static int				_currentLine;				// Current terminal line number for cursor movement
		static bool				_s_logging;					// True while Logger is writing to stdout (guards safeClose output)

		// Private method(s)

		static std::string		getCurrentTime();
		static std::string		formatSize(size_t bytes);
		static std::string		getStatusColor(int statusCode);
		static void				printSeparator();

		/** @brief Renders one log line to stdout; appends status and timing if includeCompletion. */
		static void				flushRequestLine(int requestId, bool includeCompletion, int status, size_t requestSize, size_t responseSize);

	public:

		// Public method(s)

		/** @brief Records request start and displays the pending log line. */
		static void				logRequestStart(int requestId, const std::string& method, const std::string& uri, const std::string& clientIP,
									const std::string& serverName, int port, size_t declaredSize = std::numeric_limits<size_t>::max());

		/** @brief Completes the log line with status, sizes and response time. */
		static void				logRequestEnd(int requestId, int statusCode, size_t requestSize, size_t responseSize, time_t responseTime);

		/** @brief Prints a free-form message to stdout (warnings, errors). */
		static void				logMessage(const std::string& message);

		/** @brief Returns true while Logger is actively writing to stdout. */
		static bool				isLogging();
};

#endif
