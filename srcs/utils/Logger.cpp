/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:38 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/05 11:51:31 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "Logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

// Static variables initialization
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

	int uriFieldWidth = 50;
	
	// Format count suffix and calculate its actual length
	std::stringstream countStr;
	int actualCountLen = 0;
	if (_requestCount > 1) {
		countStr << GRAY << "(" << _requestCount << ")" << RESET;
		// Calculate actual count length (without color codes)
		std::stringstream plainCount;
		plainCount << "(" << _requestCount << ")";
		actualCountLen = plainCount.str().length();
	}
	
	// Calculate available space for URI (field width minus count and 1 space)
	int maxUriLen = uriFieldWidth - actualCountLen;
	if (actualCountLen > 0)
		maxUriLen--; // Reserve 1 space between URI and count
	
	// Ensure minimum
	if (maxUriLen < 2) maxUriLen = 2; // Minimum for ".."
	
	// Truncate URI if necessary
	std::string displayUri = _lastUri;
	if ((int)displayUri.length() > maxUriLen) {
		if (maxUriLen >= 2)
			displayUri = displayUri.substr(0, maxUriLen - 2) + "..";
		else
			displayUri = ".."; // Fallback if really too small
	}

	// Calculate padding for method (8 chars)
	int methodPadding = 8 - _lastMethod.length();
	if (methodPadding < 0) methodPadding = 0;

	// Format server:port with fixed width
	std::stringstream serverStr;
	serverStr << _lastServerName << ":" << _lastServerPort;

	// Format timing
	std::stringstream timingStr;
	if (_requestCount == 1) {
		// Single request: show one time
		if (_minTime < 1.0)
			timingStr << std::fixed << std::setprecision(0) << (_minTime * 1000) << "µs";
		else
			timingStr << std::fixed << std::setprecision(1) << _minTime << "ms";
	} else {
		// Multiple requests: show min-max
		if (_maxTime < 1.0) {
			// Both in microseconds
			timingStr << std::fixed << std::setprecision(0) 
					  << (_minTime * 1000) << "-" << (_maxTime * 1000) << "µs";
		} else if (_minTime >= 1.0) {
			// Both in milliseconds
			timingStr << std::fixed << std::setprecision(1) 
					  << _minTime << "-" << _maxTime << "ms";
		} else {
			// Mixed: min in µs, max in ms
			timingStr << std::fixed << std::setprecision(0) << (_minTime * 1000) << "µs-"
					  << std::fixed << std::setprecision(1) << _maxTime << "ms";
		}
	}
	
	// Build URI+count field with proper alignment (always exactly uriFieldWidth)
	std::stringstream uriField;
	uriField << displayUri;
	if (_requestCount > 1) {
		// Calculate actual combined length and padding needed
		int combinedLen = displayUri.length() + 1 + actualCountLen; // URI + space + count
		
		// If it would overflow, we need to re-truncate URI further
		if (combinedLen > uriFieldWidth && displayUri.length() > 2) {
			int excess = combinedLen - uriFieldWidth;
			int newUriLen = displayUri.length() - excess;
			if (newUriLen >= 2) {
				displayUri = displayUri.substr(0, newUriLen - 2) + "..";
				uriField.str(""); // Clear
				uriField << displayUri;
			}
		}
		
		// Now add padding and count
		int padding = uriFieldWidth - displayUri.length() - actualCountLen;
		if (padding > 0)
			uriField << std::string(padding, ' ');
		else
			uriField << ' '; // At minimum 1 space
		uriField << countStr.str();
	} else {
		int padding = uriFieldWidth - displayUri.length();
		if (padding > 0)
			uriField << std::string(padding, ' ');
	}

	std::cout << "\r"
				<< std::setw(25) << std::left << serverStr.str() << " "
				<< GRAY << "|" << RESET << " "
				<< "[" << getCurrentTime() << "] "
				<< GRAY << "|" << RESET << " "
				<< _lastClientIP << " "
				<< GRAY << "|" << RESET << " "
				<< methodColor << BOLD << _lastMethod << RESET
				<< std::string(methodPadding, ' ') << " "
				<< uriField.str() << " "
				<< GRAY << "|" << RESET << " "
				<< std::setw(5) << std::right << formatSize(_lastSize) << " "
				<< GRAY << "|" << RESET << " "
				<< GRAY << "→" << RESET << " "
				<< statusColor << BOLD << _lastStatus << RESET << " "
				<< GRAY << "|" << RESET << " "
				<< std::setw(6) << std::right << timingStr.str()
				<< "\033[K"; // ANSI escape: clear to end of line
	std::cout.flush();
}

void Logger::finalizeGroupedRequests()
{
	if (_requestCount > 0)
		std::cout << std::endl;
	
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

	// If different request or inactive period, finalize and start new group
	if (!isSameRequest || isInactive) {
		finalizeGroupedRequests();
		
		if (isInactive && _lastRequestTime.tv_sec != 0)
			std::cout << GRAY << std::string(91, '-') << RESET << std::endl;
		
		// Start new group
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
		
		// Display immediately
		flushGroupedRequests();
	} else {
		// Add to current group and update display
		_requestCount++;
		_totalTime += responseTime;
		if (responseTime < _minTime)
			_minTime = responseTime;
		if (responseTime > _maxTime)
			_maxTime = responseTime;
		
		// Update display in place
		flushGroupedRequests();
	}
	
	_lastRequestTime = now;
}

void Logger::logError(const std::string &message)
{
	std::cout << RED << "❌ Error: " << RESET << message << std::endl;
}

void Logger::logStderr(const std::string &stderrOutput)
{
	if (stderrOutput.empty())
		return;
	
	// Force finalization to ensure log line is complete
	if (_requestCount > 0)
	{
		std::cout << std::endl;
		_requestCount = 0;
		_totalTime = 0.0;
		_minTime = 0.0;
		_maxTime = 0.0;
	}
	
	// Display stderr output
	std::cout << stderrOutput;
	if (stderrOutput[stderrOutput.length() - 1] != '\n')
		std::cout << std::endl;
	std::cout.flush();
}


