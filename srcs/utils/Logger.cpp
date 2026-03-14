/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:38 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/14 22:44:03 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------

#include "Logger.hpp"
#include <iomanip>		// std::setw, std::left, std::right
#include <iostream>		// std::cout
#include <sstream>		// std::stringstream

// Static variable(s) initialization -------------------------------------------

requestMap		Logger::_activeRequests;
std::string		Logger::_lastMethod;
std::string		Logger::_lastUri;
std::string		Logger::_lastClientIP;
size_t			Logger::_requestCount			= 0;
time_t			Logger::_minTime				= std::numeric_limits<time_t>::max();
time_t			Logger::_maxTime				= 0;
std::string		Logger::_lastServerName;
int				Logger::_lastServerPort			= 0;
size_t			Logger::_lastEndRequestSize		= std::numeric_limits<size_t>::max();
int				Logger::_lastEndStatus			= -1;
size_t			Logger::_groupEndCount			= 0;
bool			Logger::_pendingRequest			= false;
std::string		Logger::_lastRequestStartTime;
int				Logger::_lastDisplayedRequestId	= -1;
int				Logger::_currentLine			= 0;
bool			Logger::_s_logging				= false;
LastLineType	Logger::_lastLineType			= LINE_NONE;

// Private method(s) -----------------------------------------------------------

std::string Logger::getCurrentTime()
{
	std::time_t now = std::time(NULL);
	std::tm* tm_info = std::localtime(&now);
	if (!tm_info)
		return "00:00:00";
	char buffer[TIME_BUFFER_SIZE];
	std::strftime(buffer, sizeof(buffer), "%H:%M:%S", tm_info);
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
	// Look up request data for this specific request
	requestMap::iterator it = _activeRequests.find(requestId);
	std::string method = it->second.method;
	std::string uri = it->second.uri;
	std::string clientIP = it->second.clientIP;
	std::string serverName = it->second.serverName;
	int serverPort = it->second.serverPort;
	std::string requestStartTime = _lastRequestStartTime;
	std::string statusColor = includeCompletion ? getStatusColor(status) : RESET;
	std::string methodColor = (method == "GET") ? GREEN : 
							  (method == "HEAD") ? CYAN : 
							  (method == "POST") ? YELLOW : 
							  (method == "DELETE") ? RED : GREY;

	// Format server:port (right-align)
	std::stringstream portStr;
	portStr << ":" << serverPort;
	std::string portPart = portStr.str();
	size_t maxServerNameLen = 0;
	if (portPart.length() < SERVER_PORT_FIELD_WIDTH)
		maxServerNameLen = SERVER_PORT_FIELD_WIDTH - portPart.length();
	std::string displayServerName = serverName;
	if (displayServerName.length() > maxServerNameLen)
	{
		if (maxServerNameLen < 2)
			displayServerName = displayServerName.substr(0, maxServerNameLen);
		else
			displayServerName = displayServerName.substr(0, maxServerNameLen - 2) + "..";
	}
	std::string serverPortStr = displayServerName + portPart;
	size_t serverPad = 0;
	if (serverPortStr.length() < SERVER_PORT_FIELD_WIDTH)
		serverPad = SERVER_PORT_FIELD_WIDTH - serverPortStr.length();
	std::string rightAlignedServerPort = std::string(serverPad, ' ') + serverPortStr;

	// --- Build URI field (fixed width: URI_FIELD_WIDTH) ---

	// Clean URI: replace non-printable characters with '?'
	std::string displayUri = uri;
	for (size_t i = 0; i < displayUri.length(); i++)
		if (displayUri[i] < 32 || displayUri[i] == 127)
			displayUri[i] = '?';

	// Compute upload size hint shown after URI for POST
	std::string hintStr;
	size_t hintLen = 0;
	{
		size_t sz = it->second.declaredSize;
		if (sz == std::numeric_limits<size_t>::max() && includeCompletion
			&& requestSize != std::numeric_limits<size_t>::max())
			sz = requestSize;
		if (sz != std::numeric_limits<size_t>::max())
		{
			std::string s = formatSize(sz);
			hintStr = std::string(" ") + YELLOW + "(" + s + ")" + RESET;
			hintLen = 3 + s.length();
		}
	}

	// Compute count suffix for grouped requests
	std::string countStr;
	size_t countLen = 0;
	if (_requestCount > 1)
	{
		std::stringstream ss;
		ss << "(x" << _requestCount << ")";
		countLen = ss.str().length();
		countStr = std::string(GREY) + ss.str() + RESET;
	}

	// Truncate URI to fit - reserving space for hint, count, pending "..."
	size_t reserved = hintLen + (countLen > 0 ? 1 + countLen : 0)
					+ (!includeCompletion && countLen == 0 ? 4 : 0);
	size_t maxUriLen = (reserved < URI_FIELD_WIDTH) ? URI_FIELD_WIDTH - reserved : 2;
	if (maxUriLen < 2) maxUriLen = 2;
	if (displayUri.length() > maxUriLen)
		displayUri = (maxUriLen >= 2) ? displayUri.substr(0, maxUriLen - 1) + "…" : "…";

	// Assemble URI field
	std::stringstream uriField;
	size_t usedLen = displayUri.length() + hintLen;
	uriField << displayUri << hintStr;
	if (countLen > 0)
	{
		size_t gap = (usedLen + countLen < URI_FIELD_WIDTH) ? URI_FIELD_WIDTH - usedLen - countLen : 1;
		uriField << std::string(gap, ' ') << countStr;
	} else {
		if (!includeCompletion)
			uriField << GREY << " ..." << RESET;
		size_t totalUsed = usedLen + (!includeCompletion ? 4 : 0);
		if (totalUsed < URI_FIELD_WIDTH)
			uriField << std::string(URI_FIELD_WIDTH - totalUsed, ' ');
	}

	// Format timing (only if completion)
	std::stringstream timingStr;
	if (includeCompletion && _maxTime)
	{
		if (_requestCount > 1 && _minTime != _maxTime)
			timingStr << _minTime << "-";
		timingStr << _maxTime << "s";
	}

	std::stringstream statusStr;
	if (includeCompletion)
		statusStr << GREY << "→ " << statusColor << BOLD << status << RESET;

	// Build output
	std::stringstream output;
	output << "\r" << "[" << requestStartTime << "] " << GREY << "| "
		   << CYAN << std::setw(SERVER_PORT_FIELD_WIDTH) << std::right << serverPortStr << RESET << " >-< "
		   << CYAN << std::setw(IP_FIELD_WIDTH) << std::left << clientIP << GREY << " | " << RESET
		   << methodColor << BOLD << std::setw(METHOD_FIELD_WIDTH) << std::right << method << RESET << " "
		   << uriField.str();
	if (includeCompletion)
		output << GREY << " | " << RESET << std::setw(RESPONSE_SIZE_FIELD_WIDTH) << std::right << formatSize(responseSize)
			   << GREY << " | " << RESET << std::setw(STATUS_FIELD_WIDTH) << statusStr.str() << RESET
			   << GREY << " | " << RESET << std::left << timingStr.str();
	output << CLEARLINE;
	std::cout << output.str();
	std::cout.flush();
}

// Public method(s) ------------------------------------------------------------

void Logger::requestStart(const RequestInfo& info)
{
	if (_lastLineType == LINE_NONE)
	{
		_s_logging = true;
		printSeparator();
		std::cout << std::endl;
		_currentLine++;
	}
	else if (_lastLineType == LINE_SEPARATOR)
	{
		// Start a new line after the closing separator
		std::cout << std::endl;
		_currentLine++;
	}
	_lastLineType = LINE_REQUEST;

	// Store request data for this specific request
	RequestData data;
	data.method = info.method;
	data.uri = info.uri;
	data.clientIP = info.clientIP;
	data.serverName = info.serverName;
	data.serverPort = info.port;
	data.declaredSize = info.declaredSize;
	data.requestStartTime = getCurrentTime();
	data.displayLine = _currentLine;
	_activeRequests[info.requestId] = data;

	bool isGroupableRequest = (_lastMethod == info.method && _lastUri == info.uri
		&& _lastClientIP == info.clientIP && _lastServerName == info.serverName
		&& _lastServerPort == info.port);

	if (_pendingRequest && !isGroupableRequest)
	{
		std::cout << std::endl;
		_currentLine++;
		_minTime = std::numeric_limits<time_t>::max();
		_maxTime = 0;
		_lastEndRequestSize = std::numeric_limits<size_t>::max();
		_lastEndStatus = -1;
		_groupEndCount = 0;
	}

	if (isGroupableRequest && _pendingRequest)
	{
		_requestCount++;
		return;
	}

	// New request - initialize
	_requestCount = 1;
	_lastMethod = info.method;
	_lastUri = info.uri;
	_lastClientIP = info.clientIP;
	_lastServerName = info.serverName;
	_lastServerPort = info.port;
	_lastRequestStartTime = data.requestStartTime;
	_pendingRequest = true;
	_lastDisplayedRequestId = info.requestId;

	// Record line AFTER potential endl above
	_activeRequests[info.requestId].displayLine = _currentLine;

	flushRequestLine(info.requestId, false, 0, 0, 0);
}

void Logger::requestEnd(const RequestInfo& info)
{
	if (!_pendingRequest)
		return;

	// Check if this request is part of the currently displayed group
	requestMap::iterator it = _activeRequests.find(info.requestId);
	if (it == _activeRequests.end())
		return; // already logged or unknown request
	bool isGroupedRequest = (it->second.method == _lastMethod &&
							it->second.uri == _lastUri &&
							it->second.clientIP == _lastClientIP &&
							it->second.serverName == _lastServerName &&
							it->second.serverPort == _lastServerPort);

	// Break the group if request body size or status differs from previous completion
	if (isGroupedRequest && _groupEndCount
		&& (info.requestSize != _lastEndRequestSize || info.statusCode != _lastEndStatus))
	{
		std::cout << std::endl;
		_currentLine++;
		_requestCount = 1;
		_minTime = std::numeric_limits<time_t>::max();
		_maxTime = 0;
		_groupEndCount = 0;
	}

	if (isGroupedRequest)
	{
		// Part of the current group - update timing and overwrite current line
		if (info.responseTime < _minTime) _minTime = info.responseTime;
		if (info.responseTime > _maxTime) _maxTime = info.responseTime;
		_lastEndRequestSize = info.requestSize;
		_lastEndStatus = info.statusCode;
		_groupEndCount++;
		flushRequestLine(info.requestId, true, info.statusCode, info.requestSize, info.responseSize);
		_lastDisplayedRequestId = info.requestId;
	}
	else if (it != _activeRequests.end())
	{
		// Non-grouped request on a previous line - overwrite it in place
		int linesUp = _currentLine - it->second.displayLine;

		// Save current group state
		size_t savedCount = _requestCount;
		time_t savedMin = _minTime;
		time_t savedMax = _maxTime;

		// Set state for this individual request
		_requestCount = 1;
		_minTime = info.responseTime;
		_maxTime = info.responseTime;

		if (linesUp > 0)
			std::cout << "\033[" << linesUp << "A"; // cursor up
		flushRequestLine(info.requestId, true, info.statusCode, info.requestSize, info.responseSize);
		if (linesUp > 0)
			std::cout << "\033[" << linesUp << "B"; // cursor down
		std::cout.flush();

		// Restore group state
		_requestCount = savedCount;
		_minTime = savedMin;
		_maxTime = savedMax;
	}

	_activeRequests.erase(info.requestId);
}

void Logger::logMessage(const std::string& message)
{
    if (_pendingRequest)
    {
        std::cout << std::endl;
        _currentLine++;
        _lastLineType = LINE_REQUEST;
    }

    // Opening separator only if last line wasn't already one
    if (_lastLineType != LINE_SEPARATOR)
    {
        printSeparator();
        std::cout << std::endl;
        _currentLine++;
    }

	// Message content
	std::cout << message;
	for (size_t i = 0; i < message.length(); i++)
		if (message[i] == '\n')
			_currentLine++;
	if (message.empty() || message[message.length() - 1] != '\n')
	{
		std::cout << std::endl;
		_currentLine++;
	}

	// Closing separator (no trailing newline — next logRequestStart will handle it)
    printSeparator();
    _lastLineType = LINE_SEPARATOR;
    std::cout.flush();

	// Break any pending group — next identical request starts fresh
	_pendingRequest = false;
	_lastMethod = "";
	_lastUri = "";
	_requestCount = 0;
	_groupEndCount = 0;
	_minTime = std::numeric_limits<time_t>::max();
	_maxTime = 0;
	_lastEndRequestSize = std::numeric_limits<size_t>::max();
	_lastEndStatus = -1;
}

bool Logger::hasStarted()
{
	return _s_logging;
}

void Logger::printSeparator()
{
	std::cout << GREY << std::string(LOG_SEPARATOR_WIDTH, '-') << RESET;
}
