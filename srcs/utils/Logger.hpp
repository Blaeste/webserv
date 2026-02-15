/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/15 12:41:03 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s) ******************************************************************
#include <string>
#include <sys/time.h>

// Defines *********************************************************************
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define GREY    "\033[90m"
#define BOLD    "\033[1m"

// Class ***********************************************************************
class Logger {
	private:
		// Attribute(s) --------------------------------------------------------
		// static timeval _lastRequestTime;
		static std::string _lastMethod; ///< Last logged HTTP method
		static std::string _lastUri; ///< Last logged URI
		static std::string _lastClientIP; ///< Last logged client IP address
		static int _lastStatus; ///< Last logged HTTP status code
		static size_t _lastSize; ///< Last logged response size
		static size_t _requestCount; ///< Number of grouped identical requests
		static double _totalTime; ///< Total response time for grouped requests
		static double _minTime; ///< Minimum response time in group
		static double _maxTime; ///< Maximum response time in group
		static std::string _lastServerName; ///< Last logged server name
		static int _lastServerPort; ///< Last logged server port
		static bool _firstLog; ///< Indicates if this is the first log entry
		static bool _pendingRequest; ///< Request started but not completed
		static bool _currentIsSameAsPrevious; ///< Current request identical to previous

		// Private method(s) ---------------------------------------------------
		static std::string getCurrentTime();
		static std::string formatSize(size_t bytes);
		static std::string getStatusColor(int statusCode);
		static void flushGroupedRequests();
		static void finalizeGroupedRequests();
		static void printSeparator();
		static void formatRequestLine(std::stringstream &output, bool includeCompletion);

	public:
		// Public method(s) ----------------------------------------------------
		static void logRequestStart(const std::string &method, const std::string &uri,
							const std::string &clientIP, std::string serverName, int port);
		static void logRequestEnd(int statusCode, size_t responseSize, double responseTime);
		static void logRequest(const std::string &method, const std::string &uri,
							const std::string &clientIP, int statusCode,
							size_t responseSize, double responseTime,
							std::string serverName, int port);
		static void logMessage(const std::string &message);
};
