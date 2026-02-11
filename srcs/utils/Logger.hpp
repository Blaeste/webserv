/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/11 13:45:13 by eschwart         ###   ########.fr       */
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
	// Attribute(s) ------------------------------------------------------------
	// static timeval _lastRequestTime;
	static std::string _lastMethod;
	static std::string _lastUri;
	static std::string _lastClientIP;
	static int _lastStatus;
	static size_t _lastSize;
	static int _requestCount;
	static double _totalTime;
	static double _minTime;
	static double _maxTime;
	static std::string _lastServerName;
	static int _lastServerPort;
	static bool _firstLog;

	// Private method(s) -------------------------------------------------------
	static std::string getCurrentTime();
	static std::string formatSize(size_t bytes);
	static std::string getStatusColor(int statusCode);
	static void flushGroupedRequests();
	static void finalizeGroupedRequests();
	static void printSeparator();

public:
	// Public method(s) --------------------------------------------------------
	static void logRequest(const std::string &method, const std::string &uri,
							const std::string &clientIP, int statusCode,
							size_t responseSize, double responseTime,
							std::string serverName, int port);
	static void logMessage(const std::string &message);
};
