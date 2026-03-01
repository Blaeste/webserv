/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:38 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/01 16:02:34 by gdosch           ###   ########.fr       */
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
size_t Logger::_lastSize = 0;
size_t Logger::_requestCount = 0;
double Logger::_minTime = std::numeric_limits<double>::max();
double Logger::_maxTime = 0.0;
std::string Logger::_lastServerName = "";
int Logger::_lastServerPort = 0;
size_t Logger::_lastEndRequestSize = std::numeric_limits<size_t>::max();
size_t Logger::_groupEndCount = 0;
bool Logger::_firstLog = true;
bool Logger::_pendingRequest = false;
std::string Logger::_lastRequestStartTime = "";
int Logger::_lastDisplayedRequestId = -1;
int Logger::_currentLine = 0;

// Private method(s) -----------------------------------------------------------
std::string Logger::getCurrentTime()
{
	time_t now = std::time(NULL);
	struct tm *tm_info = std::localtime(&now);
	char buffer[9];

	std::strftime(buffer, 9, "%H:%M:%S", tm_info);
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
		ss << bytes << "B";
	else if (bytes < MB)
		ss << (bytes / KB) << "K";
	else if (bytes < GB)
		ss << (bytes / MB) << "M";
	else if (bytes < TB)
		ss << (bytes / GB) << "G";
	else
		ss << (bytes / TB) << "T";
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

void Logger::flushRequestLine(int requestId, bool includeCompletion, int status, size_t requestSize, size_t responseSize)
{
	std::stringstream output;

	// Look up request data for this specific request
	std::map<int, RequestData>::iterator it = _activeRequests.find(requestId);

	// Get request-specific data or fall back to static variables for display
	std::string method = (it != _activeRequests.end()) ? it->second.method : _lastMethod;
	std::string uri = (it != _activeRequests.end()) ? it->second.uri : _lastUri;
	std::string clientIP = (it != _activeRequests.end()) ? it->second.clientIP : _lastClientIP;
	std::string serverName = (it != _activeRequests.end()) ? it->second.serverName : _lastServerName;
	int serverPort = (it != _activeRequests.end()) ? it->second.serverPort : _lastServerPort;
	std::string requestStartTime = _lastRequestStartTime;
	std::string statusColor = includeCompletion ? getStatusColor(status) : RESET;
	std::string methodColor = (method == "GET") ? GREEN : 
							  (method == "HEAD") ? CYAN : 
							  (method == "POST") ? YELLOW : 
							  (method == "DELETE") ? RED : GREY;

	// Format count suffix
	std::stringstream countStr;
	int actualCountLen = 0;
	if (_requestCount > 1) {
		countStr << GREY << "(x" << _requestCount << ")" << RESET;
		std::stringstream plainCount;
		plainCount << "(x" << _requestCount << ")";
		actualCountLen = plainCount.str().length();
	}

	// Calculate available space for URI
	int maxUriLen = URI_FIELD_WIDTH - actualCountLen;
	if (actualCountLen > 0)
		maxUriLen--;
	if (maxUriLen < 2) maxUriLen = 2;

	// Left-align client IP
	int ipFieldWidth = 15;
	int ipLen = clientIP.length();
	std::string leftAlignedIP = clientIP + std::string(ipFieldWidth - ipLen > 0 ? ipFieldWidth - ipLen : 0, ' ');

	// Clean URI: replace non-printable characters with '?'
	std::string displayUri = uri;
	for (size_t i = 0; i < displayUri.length(); i++) {
		if (displayUri[i] < 32 || displayUri[i] == 127)
			displayUri[i] = '?';
	}
	
	// Truncate URI if necessary
	if ((int)displayUri.length() > maxUriLen) {
		if (maxUriLen >= 1)
			displayUri = displayUri.substr(0, maxUriLen - 1) + "…";
		else
			displayUri = "…";
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

	// Right-align serverPortStr within serverFieldWidth
	int serverPad = serverFieldWidth - (int)serverPortStr.length();
	if (serverPad < 0) serverPad = 0;
	std::string rightAlignedServerPort = std::string(serverPad, ' ') + serverPortStr;

	// Format timing (only if completion)
	std::stringstream timingStr;

	if (includeCompletion) {
		if (_requestCount > 1 && _minTime != _maxTime)
			timingStr << std::fixed << std::setprecision(0) << (_minTime / 1000.0) << "-";
		timingStr << std::fixed << std::setprecision(0) << (_maxTime / 1000.0) << "s";
	}

	// Compute upload size hint to show after URI
	std::string uploadHint;
	int uploadHintPlainLen = 0;
	{
		size_t displayUploadSize = std::numeric_limits<size_t>::max();
		if (it != _activeRequests.end() && it->second.declaredSize != std::numeric_limits<size_t>::max())
			displayUploadSize = it->second.declaredSize;
		else if (includeCompletion && requestSize != std::numeric_limits<size_t>::max())
			displayUploadSize = requestSize;
		if (displayUploadSize != std::numeric_limits<size_t>::max()) {
			std::string sizeStr = formatSize(displayUploadSize);
			uploadHint = std::string(YELLOW) + "(" + sizeStr + ")" + RESET;
			uploadHintPlainLen = 2 + sizeStr.length();
		}
	}
	int hintSpace = uploadHintPlainLen > 0 ? 1 + uploadHintPlainLen : 0;

	// Build URI+count field
	std::stringstream uriField;
	uriField << displayUri;
	if (_requestCount > 1) {
		size_t combinedLen = displayUri.length() + hintSpace + 1 + actualCountLen;
		if (combinedLen > URI_FIELD_WIDTH && displayUri.length() > 2) {
			int excess = combinedLen - URI_FIELD_WIDTH;
			int newUriLen = displayUri.length() - excess;
			if (newUriLen >= 2) {
				displayUri = displayUri.substr(0, newUriLen - 2) + "..";
				uriField.str("");
				uriField << displayUri;
			}
		}
		if (uploadHintPlainLen > 0)
			uriField << " " << uploadHint;
		int padding = URI_FIELD_WIDTH - displayUri.length() - hintSpace - actualCountLen;
		if (padding > 0)
			uriField << std::string(padding, ' ');
		else
			uriField << ' ';
		uriField << countStr.str();
	} else {
		if (uploadHintPlainLen > 0)
			uriField << " " << uploadHint;
		if (!includeCompletion)
			uriField << GREY << " ..." << RESET;
		int padding = URI_FIELD_WIDTH - displayUri.length() - hintSpace - (!includeCompletion ? 4 : 0);
		if (padding > 0)
			uriField << std::string(padding, ' ');
	}

	// Build output
	output << "\r";
	output << "[" << requestStartTime << "] " << GREY << "| "
		   << CYAN << rightAlignedServerPort << RESET << " >-< "
		   << CYAN << leftAlignedIP << GREY << " | " << RESET
		   << methodColor << BOLD << std::setw(7) << std::right << method << RESET << " "
		   << uriField.str();
	if (includeCompletion) {
		output << GREY << " | " << RESET
			   << std::setw(4) << std::right << formatSize(responseSize)
			   << GREY << " | " << RESET
			   << GREY << "→ " << RESET
			   << statusColor << BOLD << status << RESET
			   << GREY << " | " << RESET
			   << std::left << timingStr.str();
	}
	output << CLEARLINE;
	std::cout << output.str();
	std::cout.flush();
}

// Public method(s) ------------------------------------------------------------
void Logger::logRequestStart(int requestId, const std::string &method, const std::string &uri,
							 const std::string &clientIP, std::string serverName, int port, size_t declaredSize)
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
	data.declaredSize = declaredSize;
	data.requestStartTime = getCurrentTime();
	data.displayLine = _currentLine;
	_activeRequests[requestId] = data;

	bool isGroupableRequest = (_lastMethod == method && _lastUri == uri &&
						  _lastClientIP == clientIP && _lastServerName == serverName &&
						  _lastServerPort == port);

	if (_pendingRequest && !isGroupableRequest) {
		std::cout << std::endl;
		_currentLine++;
		_minTime = std::numeric_limits<double>::max();
		_maxTime = 0.0;
		_lastEndRequestSize = std::numeric_limits<size_t>::max();
		_groupEndCount = 0;
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
	_lastDisplayedRequestId = requestId;

	// Record line AFTER potential endl above
	_activeRequests[requestId].displayLine = _currentLine;

	flushRequestLine(requestId, false, 0, 0, 0);
}

void Logger::logRequestEnd(int requestId, int statusCode, size_t requestSize, size_t responseSize, double responseTime)
{
	if (!_pendingRequest)
		return;

	_lastSize = responseSize;

	// Check if this request is part of the currently displayed group
	std::map<int, RequestData>::iterator it = _activeRequests.find(requestId);
	bool isGroupedRequest = false;
	if (it != _activeRequests.end()) {
		isGroupedRequest = (it->second.method == _lastMethod &&
							it->second.uri == _lastUri &&
							it->second.clientIP == _lastClientIP &&
							it->second.serverName == _lastServerName &&
							it->second.serverPort == _lastServerPort);
	} else {
		isGroupedRequest = true;
	}

	// Break the group if actual request body size differs from previous completion
	if (isGroupedRequest && _groupEndCount > 0 && requestSize != _lastEndRequestSize) {
		std::cout << std::endl;
		_currentLine++;
		_requestCount = 1;
		_minTime = std::numeric_limits<double>::max();
		_maxTime = 0.0;
		_groupEndCount = 0;
	}

	if (isGroupedRequest) {
		// Part of the current group - update timing and overwrite current line
		if (responseTime < _minTime) _minTime = responseTime;
		if (responseTime > _maxTime) _maxTime = responseTime;
		_lastEndRequestSize = requestSize;
		_groupEndCount++;
		flushRequestLine(requestId, true, statusCode, requestSize, responseSize);
		_lastDisplayedRequestId = requestId;
	} else if (it != _activeRequests.end()) {
		// Non-grouped request on a previous line - overwrite it in place
		int linesUp = _currentLine - it->second.displayLine;

		// Save current group state
		size_t savedCount = _requestCount;
		double savedMin = _minTime;
		double savedMax = _maxTime;

		// Set state for this individual request
		_requestCount = 1;
		_minTime = responseTime;
		_maxTime = responseTime;

		if (linesUp > 0)
			std::cout << "\033[" << linesUp << "A"; // cursor up
		flushRequestLine(requestId, true, statusCode, requestSize, responseSize);
		if (linesUp > 0)
			std::cout << "\033[" << linesUp << "B"; // cursor down
		std::cout.flush();

		// Restore group state
		_requestCount = savedCount;
		_minTime = savedMin;
		_maxTime = savedMax;
	}

	_activeRequests.erase(requestId);
}

void Logger::logMessage(const std::string &message)
{
	// Force finalization to ensure log line is complete
	if (_pendingRequest) {
		std::cout << std::endl;
		_currentLine++;
	}

	Logger::printSeparator();
	std::cout << message;
	for (size_t i = 0; i < message.length(); i++) {
		if (message[i] == '\n')
			_currentLine++;
	}
	if (message.empty() || message[message.length() - 1] != '\n') {
		std::cout << std::endl;
		_currentLine++;
	}
	Logger::printSeparator();

	std::cout.flush();
}

void Logger::printSeparator()
{
	std::cout << GREY << std::string(145, '-') << RESET << std::endl;
	_currentLine++;
}
