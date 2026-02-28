/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:38 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/28 12:38:52 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------
#include "Logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <limits>

// Static variables initialization ---------------------------------------------
std::map<int, RequestData> Logger::_activeRequests;
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
int Logger::_lastDisplayedRequestId = -1;

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

void Logger::flushRequestLine(int requestId, bool includeCompletion, int status, size_t size)
{
	std::stringstream output;

	// Look up request data for this specific request
	std::map<int, RequestData>::iterator it = _activeRequests.find(requestId);
	if (it == _activeRequests.end() && includeCompletion) {
		// Request not found in active requests, use static variables as fallback
		// This happens for grouped requests that don't have individual entries
	}

	// Get request-specific data or fall back to static variables for display
	std::string method = (it != _activeRequests.end()) ? it->second.method : _lastMethod;
	std::string uri = (it != _activeRequests.end()) ? it->second.uri : _lastUri;
	std::string clientIP = (it != _activeRequests.end()) ? it->second.clientIP : _lastClientIP;
	std::string serverName = (it != _activeRequests.end()) ? it->second.serverName : _lastServerName;
	int serverPort = (it != _activeRequests.end()) ? it->second.serverPort : _lastServerPort;
	std::string requestStartTime = (it != _activeRequests.end()) ? it->second.requestStartTime : _lastRequestStartTime;

	std::string statusColor = includeCompletion ? getStatusColor(status) : RESET;
	std::string methodColor = (method == "GET") ? GREEN : 
							  (method == "HEAD") ? CYAN : 
							  (method == "POST") ? YELLOW : 
							  (method == "DELETE") ? RED : GREY;
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
	int ipLen = clientIP.length();
	int ipPadding = (ipFieldWidth - ipLen) / 2;
	std::string centeredIP = std::string(ipPadding, ' ') + clientIP + 
							 std::string(ipFieldWidth - ipLen - ipPadding, ' ');

	// Clean URI: replace non-printable characters with '?'
	std::string displayUri = uri;
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
	portStr << ":" << serverPort;
	std::string portPart = portStr.str();
	int maxServerNameLen = serverFieldWidth - portPart.length();
	std::string displayServerName = serverName;

	if ((int)displayServerName.length() > maxServerNameLen && maxServerNameLen >= 2) {
		displayServerName = displayServerName.substr(0, maxServerNameLen - 2) + "..";
	} else if ((int)displayServerName.length() > maxServerNameLen) {
		displayServerName = displayServerName.substr(0, maxServerNameLen);
	}
	std::string serverPortStr = displayServerName + portPart;

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
		   << "[" << requestStartTime << "] "
		   << GREY << "|" << RESET << " "
		   << centeredIP << " "
		   << GREY << "|" << RESET << " "
		   << methodColor << BOLD << std::setw(7) << std::right << method << RESET << " "
		   << uriField.str() << " "
		   << GREY << "|" << RESET;

	if (includeCompletion) {
		output << " " << formatSize(size)
			   << GREY << " | " << RESET
			   << GREY << "→ " << RESET
			   << statusColor << BOLD << status << RESET
			   << GREY << " | " << RESET
			   << std::left << timingStr.str();
	} else
		output << GREY << std::string(6, ' ') << "|" << std::string(7, ' ') << "|" << RESET;
	
	output << "\033[K";
	std::cout << output.str();
	std::cout.flush();
}

// Public method(s) ------------------------------------------------------------
void Logger::logRequestStart(int requestId, const std::string &method, const std::string &uri,
                             const std::string &clientIP, std::string serverName, int port)
{
    if (_firstLog) {
        printSeparator();
        _firstLog = false;
    }

    // Store request data for this specific request
    RequestData data;
    data.method = method;
    data.uri = uri;
    data.clientIP = clientIP;
    data.serverName = serverName;
    data.serverPort = port;
    data.requestStartTime = getCurrentTime();
    _activeRequests[requestId] = data;

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
    _lastRequestStartTime = data.requestStartTime;

    _pendingRequest = true;
    _lastDisplayedRequestId = requestId; // Track which request displayed this line

    flushRequestLine(requestId, false, 0, 0);
}

void Logger::logRequestEnd(int requestId, int statusCode, size_t responseSize, double responseTime)
{
	if (!_pendingRequest)
		return;

	if (responseTime < _minTime)
		_minTime = responseTime;
	if (responseTime > _maxTime)
		_maxTime = responseTime;

	_lastStatus = statusCode;
	_lastSize = responseSize;

	// Check if this request is part of the currently displayed group
	// We check the data BEFORE looking in the map, because concurrent requests
	// from the same group might have already been erased
	std::map<int, RequestData>::iterator it = _activeRequests.find(requestId);
	bool isGroupedRequest = false;
	if (it != _activeRequests.end()) {
		// Request still in map, compare its data with current group
		isGroupedRequest = (it->second.method == _lastMethod &&
		                    it->second.uri == _lastUri &&
		                    it->second.clientIP == _lastClientIP &&
		                    it->second.serverName == _lastServerName &&
		                    it->second.serverPort == _lastServerPort);
	} else {
		// Request already erased by another concurrent completion
		// This can happen when multiple requests from the same group complete concurrently
		// In this case, we assume it's still part of the current group (don't add newline)
		isGroupedRequest = true;
	}

	// If completing a different group of requests, we need a new line
	if (!isGroupedRequest && _lastDisplayedRequestId != -1) {
		std::cout << std::endl; // Finalize the previous line
	}

	flushRequestLine(requestId, true, statusCode, responseSize);
	_lastDisplayedRequestId = requestId; // Update to current request

	// Clean up the completed request from active requests map
	_activeRequests.erase(requestId);
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
