/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/01 16:20:45 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s) ******************************************************************
#include <string>
#include <ctime>
#include <map>
#include <limits>

// Defines *********************************************************************
#define RESET		"\033[0m"
#define RED			"\033[31m"
#define GREEN		"\033[32m"
#define YELLOW		"\033[33m"
#define BLUE		"\033[34m"
#define MAGENTA		"\033[35m"
#define CYAN		"\033[36m"
#define GREY		"\033[90m"
#define BOLD		"\033[1m"
#define NOBOLD		"\033[22m"
#define CLEARLINE	"\033[K"

// Struct **********************************************************************
struct RequestData {
	std::string method;
	std::string uri;
	std::string clientIP;
	std::string serverName;
	int serverPort;
	size_t declaredSize;
	std::string requestStartTime;
	int displayLine;
};

// Class ***********************************************************************
class Logger {
	private:
		// Attribute(s) --------------------------------------------------------
		static const size_t URI_FIELD_WIDTH = 55;
		static std::map<int, RequestData> _activeRequests; // Track each request's data by socket
		static std::string _lastMethod; // Last logged HTTP method
		static std::string _lastUri; // Last logged URI
		static std::string _lastClientIP; // Last logged client IP address
		static size_t _lastSize; // Last logged response size
		static size_t _requestCount; // Number of grouped identical requests
		static double _minTime; // Minimum response time in group
		static double _maxTime; // Maximum response time in group
		static std::string _lastServerName; // Last logged server name
		static int _lastServerPort; // Last logged server port
		static size_t _lastEndRequestSize; // Last completed request body size
		static int _lastEndStatus; // Last completed response status code
		static size_t _groupEndCount; // Number of completions in current visual group
		static bool _firstLog; // Indicates if this is the first log entry
		static bool _pendingRequest; // Request started but not completed
		static std::string _lastRequestStartTime; // Store timestamp from logRequestStart
		static int _lastDisplayedRequestId; // Track which request displayed the last line
		static int _currentLine; // Current terminal line number for cursor movement

		// Private method(s) ---------------------------------------------------
		static std::string getCurrentTime();
		static std::string formatSize(size_t bytes);
		static std::string getStatusColor(int statusCode);
		static void printSeparator();
		static void flushRequestLine(int requestId, bool includeCompletion, int status, size_t requestSize, size_t responseSize);

	public:
		// Public method(s) ----------------------------------------------------
		static void logRequestStart(int requestId, const std::string &method, const std::string &uri,
							const std::string &clientIP, std::string serverName, int port,
							size_t declaredSize = std::numeric_limits<size_t>::max());
		static void logRequestEnd(int requestId, int statusCode, size_t requestSize, size_t responseSize, double responseTime);
		static void logMessage(const std::string &message);
};
