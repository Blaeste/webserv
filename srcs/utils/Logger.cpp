/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:38 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/15 15:27:45 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------
#include "Logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>

// Static variables initialization ---------------------------------------------
std::string Logger::_lastMethod = "";
std::string Logger::_lastUri = "";
std::string Logger::_lastClientIP = "";
int Logger::_lastStatus = 0;
size_t Logger::_lastSize = 0;
size_t Logger::_requestCount = 0;
double Logger::_minTime = std::numeric_limits<double>::max();
double Logger::_maxTime = 0.0;
std::string Logger::_lastServerName = "";
int Logger::_lastServerPort = 0;
bool Logger::_firstLog = true;
bool Logger::_pendingRequest = false;
std::string Logger::_lastRequestStartTime = "";

// Private method(s) -----------------------------------------------------------
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
	const size_t KB = 1024;
	const size_t MB = KB * 1024;
	const size_t GB = MB * 1024;
	const size_t TB = GB * 1024;

	std::stringstream ss;
	if (bytes < KB)
		ss << std::setw(3) << std::right << bytes << "B";
	else if (bytes < MB)
		ss << std::setw(3) << std::right << (bytes / KB) << "K";
	else if (bytes < GB)
		ss << std::setw(3) << std::right << (bytes / MB) << "M";
	else if (bytes < TB)
		ss << std::setw(3) << std::right << (bytes / GB) << "G";
	else
		ss << std::setw(3) << std::right << (bytes / TB) << "T";
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

void Logger::flushRequestLine(bool includeCompletion)
{
	std::stringstream output;
	std::string statusColor = includeCompletion ? getStatusColor(_lastStatus) : RESET;
	std::string methodColor = (_lastMethod == "GET") ? GREEN : 
							  (_lastMethod == "HEAD") ? CYAN : 
							  (_lastMethod == "POST") ? YELLOW : 
							  (_lastMethod == "DELETE") ? RED : GREY;
	int uriFieldWidth = 42;

	// Format count suffix
	std::stringstream countStr;
	int actualCountLen = 0;
	if (_requestCount > 1) {
		countStr << GREY << "(" << _requestCount << ")" << RESET;
		std::stringstream plainCount;
		plainCount << "(" << _requestCount << ")";
		actualCountLen = plainCount.str().length();
	}

	// Calculate available space for URI
	int maxUriLen = uriFieldWidth - actualCountLen;
	if (actualCountLen > 0)
		maxUriLen--;
	if (maxUriLen < 2) maxUriLen = 2;

	// Center-align client IP
	int ipFieldWidth = 15;
	int ipLen = _lastClientIP.length();
	int ipPadding = (ipFieldWidth - ipLen) / 2;
	std::string centeredIP = std::string(ipPadding, ' ') + _lastClientIP + 
							 std::string(ipFieldWidth - ipLen - ipPadding, ' ');

	// Clean URI: replace non-printable characters with '?'
	std::string displayUri = _lastUri;
	for (size_t i = 0; i < displayUri.length(); i++) {
		if (displayUri[i] < 32 || displayUri[i] == 127)
			displayUri[i] = '?';
	}
	
	// Truncate URI if necessary
	if ((int)displayUri.length() > maxUriLen) {
		if (maxUriLen >= 2)
			displayUri = displayUri.substr(0, maxUriLen - 2) + "..";
		else
			displayUri = "..";
	}

	// Format server:port
	int serverFieldWidth = 20;
	std::stringstream portStr;
	portStr << ":" << _lastServerPort;
	std::string portPart = portStr.str();
	int maxServerNameLen = serverFieldWidth - portPart.length();
	std::string serverName = _lastServerName;

	if ((int)serverName.length() > maxServerNameLen && maxServerNameLen >= 2) {
		serverName = serverName.substr(0, maxServerNameLen - 2) + "..";
	} else if ((int)serverName.length() > maxServerNameLen) {
		serverName = serverName.substr(0, maxServerNameLen);
	}
	std::string serverPortStr = serverName + portPart;

	// Format timing (only if completion)
	std::stringstream timingStr;
	if (includeCompletion) {
		if (_requestCount > 1) {
			enum Unit { US, MS, S };
			Unit minUnit = (_minTime < 1.0) ? US : (_minTime < 1000.0) ? MS : S;
			Unit maxUnit = (_maxTime < 1.0) ? US : (_maxTime < 1000.0) ? MS : S;
			
			if (minUnit == maxUnit) {
				if (minUnit == US)
					timingStr << std::fixed << std::setprecision(0) << (_minTime * 1000);
				else if (minUnit == MS)
					timingStr << std::fixed << std::setprecision(1) << _minTime;
				else
					timingStr << std::fixed << std::setprecision(1) << (_minTime / 1000.0);
			} else {
				if (minUnit == US)
					timingStr << std::fixed << std::setprecision(0) << (_minTime * 1000) << "µs";
				else
					timingStr << std::fixed << std::setprecision(1) << _minTime << "ms";
			}
			timingStr << "-";
		}
		
		if (_maxTime < 1.0)
			timingStr << std::fixed << std::setprecision(0) << (_maxTime * 1000) << "µs";
		else if (_maxTime < 1000.0)
			timingStr << std::fixed << std::setprecision(1) << _maxTime << "ms";
		else
			timingStr << std::fixed << std::setprecision(1) << (_maxTime / 1000.0) << "s";
	}

	// Build URI+count field
	std::stringstream uriField;
	uriField << displayUri;
	if (_requestCount > 1) {
		int combinedLen = displayUri.length() + 1 + actualCountLen;
		if (combinedLen > uriFieldWidth && displayUri.length() > 2) {
			int excess = combinedLen - uriFieldWidth;
			int newUriLen = displayUri.length() - excess;
			if (newUriLen >= 2) {
				displayUri = displayUri.substr(0, newUriLen - 2) + "..";
				uriField.str("");
				uriField << displayUri;
			}
		}
		int padding = uriFieldWidth - displayUri.length() - actualCountLen;
		if (padding > 0)
			uriField << std::string(padding, ' ');
		else
			uriField << ' ';
		uriField << countStr.str();
	} else {
		int padding = uriFieldWidth - displayUri.length();
		if (padding > 0)
			uriField << std::string(padding, ' ');
	}

	// Build output
	output << "\r"
		   << std::setw(20) << std::left << serverPortStr << " "
		   << GREY << "|" << RESET << " "
		   << "[" << _lastRequestStartTime << "] "
		   << GREY << "|" << RESET << " "
		   << centeredIP << " "
		   << GREY << "|" << RESET << " "
		   << methodColor << BOLD << std::setw(7) << std::right << _lastMethod << RESET << " "
		   << uriField.str() << " "
		   << GREY << "|" << RESET;

	if (includeCompletion) {
		output << " " << formatSize(_lastSize) << " "
			   << GREY << "|" << RESET << " "
			   << GREY << "→" << RESET << " "
			   << statusColor << BOLD << _lastStatus << RESET << " "
			   << GREY << "|" << RESET << " "
			   << std::left << timingStr.str();
	}
	
	output << "\033[K";
	std::cout << output.str();
	std::cout.flush();
}

// Public method(s) ------------------------------------------------------------
void Logger::logRequestStart(const std::string &method, const std::string &uri,
                             const std::string &clientIP, std::string serverName, int port)
{
    if (_firstLog) {
        printSeparator();
        _firstLog = false;
    }

    bool isGroupableRequest = (_lastMethod == method && _lastUri == uri &&
                          _lastClientIP == clientIP && _lastServerName == serverName &&
                          _lastServerPort == port);

    if (_pendingRequest && !isGroupableRequest) {
        std::cout << std::endl;
        _minTime = std::numeric_limits<double>::max();
        _maxTime = 0.0;
    }

    if (isGroupableRequest && _pendingRequest) {
        _requestCount++;
        return;
    }

    // New request - initialize
    _requestCount = 1;
    _lastMethod = method;
    _lastUri = uri;
    _lastClientIP = clientIP;
    _lastServerName = serverName;
    _lastServerPort = port;
    _lastRequestStartTime = getCurrentTime();

    _pendingRequest = true;

    flushRequestLine(false);
}

void Logger::logRequestEnd(int statusCode, size_t responseSize, double responseTime)
{
	if (!_pendingRequest)
		return;

	if (responseTime < _minTime)
		_minTime = responseTime;
	if (responseTime > _maxTime)
		_maxTime = responseTime;

	_lastStatus = statusCode;
	_lastSize = responseSize;

	flushRequestLine(true);
}

void Logger::logMessage(const std::string &message)
{
	// Force finalization to ensure log line is complete
	if (_pendingRequest)
		std::cout << std::endl;

	Logger::printSeparator();
	std::cout << message;
	if (message.empty() || message[message.length() - 1] != '\n')
		std::cout << std::endl;
	Logger::printSeparator();

	std::cout.flush();
}

void Logger::printSeparator()
{
	std::cout << GREY << std::string(132, '-') << RESET << std::endl;
}
