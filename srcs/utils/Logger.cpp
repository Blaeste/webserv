/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:38 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/03 14:21:07 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "Logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

// Initialisation des variables statiques
timeval Logger::_lastRequestTime = {0, 0};
std::string Logger::_lastMethod = "";
std::string Logger::_lastUri = "";
std::string Logger::_lastClientIP = "";
int Logger::_lastStatus = 0;
size_t Logger::_lastSize = 0;
int Logger::_requestCount = 0;
double Logger::_totalTime = 0.0;
double Logger::_minTime = 0.0;
double Logger::_maxTime = 0.0;
std::string Logger::_lastServerName = "";
int Logger::_lastServerPort = 0;

std::string Logger::getCurrentTime()
{
	time_t now = time(NULL);
	struct tm *tm_info = localtime(&now);
	char buffer[9];

	strftime(buffer, 9, "%H:%M:%S", tm_info);
	return std::string(buffer);
}

std::string Logger::formatSize(size_t bytes)
{
	std::stringstream ss;

	if (bytes < 1024) {
		ss << std::setw(4) << std::right << bytes << "B";
	} else if (bytes < 1024 * 1024) {
		ss << std::setw(4) << std::right << (bytes / 1024) << "K";
	} else {
		ss << std::setw(4) << std::right << (bytes / (1024 * 1024)) << "M";
	}

	return ss.str();
}

std::string Logger::getStatusColor(int statusCode)
{
	if (statusCode >= 200 && statusCode < 300) return GREEN;
	if (statusCode >= 300 && statusCode < 400) return CYAN;
	if (statusCode >= 400 && statusCode < 500) return YELLOW;
	if (statusCode >= 500) return RED;
	return RESET;
}

void Logger::flushGroupedRequests()
{
	if (_requestCount == 0)
		return;

	std::string statusColor = getStatusColor(_lastStatus);
	std::string methodColor = (_lastMethod == "GET") ? BLUE : (_lastMethod == "POST") ? MAGENTA : CYAN;

	std::string displayUri = _lastUri;
	if (displayUri.length() > 25)
		displayUri = displayUri.substr(0, 23) + "..";

	// Calculate padding for method (8 chars) and URI (25 chars)
	int methodPadding = 8 - _lastMethod.length();
	if (methodPadding < 0) methodPadding = 0;
	
	int uriPadding = 25 - displayUri.length();
	if (uriPadding < 0) uriPadding = 0;

	// Format server:port with fixed width
	std::stringstream serverStr;
	serverStr << _lastServerName << ":" << _lastServerPort;

	// Format count suffix (after IP)
	std::stringstream countStr;
	if (_requestCount > 1)
		countStr << " " << GRAY << "(" << _requestCount << ")" << RESET;

	std::cout
				<< std::setw(25) << std::left << serverStr.str() << " "
				<< "[" << getCurrentTime() << "] "
				<< GRAY << "|" << RESET << " "
				<< methodColor << BOLD << _lastMethod << RESET
				<< std::string(methodPadding, ' ') << " "
				<< displayUri
				<< std::string(uriPadding, ' ') << " "
				<< GRAY << "→" << RESET << " "
				<< statusColor << BOLD << _lastStatus << RESET << " "
				<< GRAY << "|" << RESET << " "
				<< std::setw(5) << std::right << formatSize(_lastSize) << " "
				<< GRAY << "|" << RESET << " "
				<< _lastClientIP
				<< countStr.str()
				<< std::endl;

	_requestCount = 0;
	_totalTime = 0.0;
	_minTime = 0.0;
	_maxTime = 0.0;
}

void Logger::logRequest(const std::string &method, const std::string &uri,
								const std::string &clientIP, int statusCode,
								size_t responseSize, double responseTime,
								 std::string serverName, int port)
{
	// Get time
	timeval now;
	gettimeofday(&now, NULL);

	// Check if this request is identical to the previous one
	bool isSameRequest = (_lastMethod == method && _lastUri == uri && 
	                      _lastStatus == statusCode && _lastSize == responseSize);

	// Check for inactivity (separator between bursts)
	bool isInactive = false;
	if (_lastRequestTime.tv_sec != 0) {
		long timeDiff = (now.tv_sec - _lastRequestTime.tv_sec) * 1000 +
						(now.tv_usec - _lastRequestTime.tv_usec) / 1000;
		isInactive = (timeDiff > 100);
	}

	// If different request or inactive period, flush grouped requests
	if (!isSameRequest || isInactive) {
		flushGroupedRequests();
		
		if (isInactive && _lastRequestTime.tv_sec != 0)
			std::cout << GRAY << std::string(91, '-') << RESET << std::endl;
		
		// Start new group (don't print server/timestamp yet, will be done in flush)
		_lastMethod = method;
		_lastUri = uri;
		_lastClientIP = clientIP;
		_lastStatus = statusCode;
		_lastSize = responseSize;
		_requestCount = 1;
		_totalTime = responseTime;
		_minTime = responseTime;
		_maxTime = responseTime;
		_lastServerName = serverName;
		_lastServerPort = port;
	} else {
		// Add to current group
		_requestCount++;
		_totalTime += responseTime;
		if (responseTime < _minTime)
			_minTime = responseTime;
		if (responseTime > _maxTime)
			_maxTime = responseTime;
	}
	
	_lastRequestTime = now;
}

void Logger::logError(const std::string &message)
{
	std::cout << RED << "❌ Error: " << RESET << message << std::endl;
}


