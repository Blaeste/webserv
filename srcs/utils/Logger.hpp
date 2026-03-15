/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/14 23:54:34 by gdosch           ###   ########.fr       */
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

struct	RequestInfo
{
	// Fields used by logRequestStart
	int			 requestId;			// Socket fd used as unique key
	std::string	 method;			// HTTP method (GET, POST, etc.)
	std::string	 uri;				// Request URI path
	std::string	 clientIP;			// Client IP address
	std::string	 serverName;		// Matched server name
	int			 port;				// Listening port
	size_t		 declaredSize;		// Content-Length (max = unknown)

	// Fields used by logRequestEnd
	int			 statusCode;		// HTTP response status code
	size_t		 requestSize;		// Request body size (max = unknown)
	size_t		 responseSize;		// Response body size in bytes
	time_t		 responseTime;		// Elapsed time in seconds

	RequestInfo()
		: requestId(-1)
		, port(0)
		, declaredSize(std::numeric_limits<size_t>::max())
		, statusCode(0)
		, requestSize(std::numeric_limits<size_t>::max())
		, responseSize(0)
		, responseTime(0)
	{}
};

struct	RequestData
{
	RequestInfo	info;				// Full request info snapshot
	std::string	requestStartTime;	// Timestamp when request was received
	int			displayLine;		// Terminal line number for in-place update
	size_t		finalGroupCount;	// Group count snapshot saved when this request's group is interrupted
};

// Typedef(s) ------------------------------------------------------------------

typedef	std::map<int, RequestData>	requestMap;

// Enum(s) ---------------------------------------------------------------------

enum	LastLineType
{
	LINE_NONE,		// nothing printed yet
	LINE_REQUEST,	// a request line (complete or pending)
	LINE_SEPARATOR,	// a separator ---
	LINE_MESSAGE	// a logMessage content line
};

// Structure(s) ----------------------------------------------------------------

struct LoggerState
{
	requestMap		activeRequests;			// Track each request's data by socket
	std::string		lastMethod;				// Last logged HTTP method
	std::string		lastUri;				// Last logged URI
	std::string		lastClientIP;			// Last logged client IP address
	size_t			requestCount;			// Number of grouped identical requests
	time_t			minTime; 				// Minimum response time in group
	time_t			maxTime;				// Maximum response time in group
	std::string		lastServerName;			// Last logged server name
	int				lastServerPort;			// Last logged server port
	size_t			lastEndRequestSize; 	// Last completed request body size
	int				lastEndStatus;			// Last completed response status code
	size_t			groupEndCount;			// Number of completions in current visual group
	bool			pendingRequest;			// Request started but not completed
	std::string		lastRequestStartTime;	// Store timestamp from logRequestStart
	int				lastDisplayedRequestId;	// Track which request displayed the last line
	int				currentLine;			// Current terminal line number for cursor movement
	bool			logging;				// True while Logger is writing to stdout (guards safeClose output)
	LastLineType	lastLineType;			// Type of the last physically printed line

	LoggerState()
		: requestCount(0)
		, minTime(std::numeric_limits<time_t>::max())
		, maxTime(0)
		, lastServerPort(0)
		, lastEndRequestSize(std::numeric_limits<size_t>::max())
		, lastEndStatus(-1)
		, groupEndCount(0)
		, pendingRequest(false)
		, lastDisplayedRequestId(-1)
		, currentLine(0)
		, logging(false)
		, lastLineType(LINE_NONE)
	{}
};

// Class -----------------------------------------------------------------------

class Logger
{
	private:

		// Constant(s)

		static const size_t	TIME_BUFFER_SIZE			= 10;			// "HH:MM:SS\0" fits in 10 bytes
		static const size_t	LOG_SEPARATOR_WIDTH			= 145;			// Dash separator line width in characters
		static const size_t	SERVER_PORT_FIELD_WIDTH		= 20;			// Column width for "server:port"
		static const size_t	IP_FIELD_WIDTH				= 15;			// Column width for client IP address
		static const size_t	METHOD_FIELD_WIDTH			= 7;			// Column width for HTTP method (e.g., "DELETE")
		static const size_t	URI_FIELD_WIDTH				= 55;			// Column width for request URI
		static const size_t	RESPONSE_SIZE_FIELD_WIDTH	= 4;			// Column width for response body size
		static const size_t	STATUS_FIELD_WIDTH			= 5;			// Column width for HTTP status code

		// Attribute(s)

		static LoggerState		_state;		

		// Private method(s)

		static std::string		getCurrentTime();
		static std::string		formatSize(size_t bytes);
		static std::string		getStatusColor(int statusCode);
		static void				printSeparator();

		/** @brief Renders one log line to stdout; appends status and timing if includeCompletion. */
		static void				flushRequestLine(int requestId, bool includeCompletion, int status, size_t requestSize, size_t responseSize);

		/** @brief Resets timing and completion counters for the current request group. */
		static void				resetTimingState();

		/** @brief Resets all grouping state so the next request starts a fresh group. */
		static void				resetGroupState();

		/** @brief Formats "servername:port" truncated to SERVER_PORT_FIELD_WIDTH. */
		static std::string		formatServerPort(const std::string& serverName, int serverPort);

		/** @brief Formats the URI column with size hint, group count and padding to URI_FIELD_WIDTH. */
		static std::string		formatUriField(const std::string& uri, size_t declaredSize, size_t requestSize, bool includeCompletion);

	public:

		// Public method(s)

		/** @brief Records request start and displays the pending log line. */
		static void				requestStart(const RequestInfo& info);

		/** @brief Completes the log line with status, sizes and response time. */
		static void				requestEnd(const RequestInfo& info);

		/** @brief Prints a free-form message to stdout (warnings, errors). */
		static void				logMessage(const std::string& message);

		/** @brief Returns true once the logger has been started (first logRequestStart call). */
		static bool				hasStarted();
};

#endif
