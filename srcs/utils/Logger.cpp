/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:38 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/14 23:46:43 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------

#include "Logger.hpp"
#include <iomanip>		// std::setw, std::left, std::right
#include <iostream>		// std::cout
#include <sstream>		// std::stringstream

// Static variable(s) initialization -------------------------------------------

LoggerState Logger::_state;

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
	requestMap::iterator it = _state.activeRequests.find(requestId);
	std::string method = it->second.info.method;
	std::string uri = it->second.info.uri;
	std::string clientIP = it->second.info.clientIP;
	std::string serverName = it->second.info.serverName;
	int serverPort = it->second.info.port;
	std::string requestStartTime = _state.lastRequestStartTime;
	std::string statusColor = includeCompletion ? getStatusColor(status) : RESET;
	std::string methodColor = (method == "GET") ? GREEN
		: (method == "HEAD") ? CYAN
		: (method == "POST") ? YELLOW
		: (method == "DELETE") ? RED : GREY;

	std::string serverPortStr = formatServerPort(serverName, serverPort);
	std::string uriFieldStr = formatUriField(uri, it->second.info.declaredSize, requestSize, includeCompletion);

	// Format timing (only if completion)
	std::stringstream timingStr;
	if (includeCompletion && _state.maxTime)
	{
		if (_state.requestCount > 1 && _state.minTime != _state.maxTime)
			timingStr << _state.minTime << "-";
		timingStr << _state.maxTime << "s";
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
		<< uriFieldStr;
	if (includeCompletion)
		output << GREY << " | " << RESET << std::setw(RESPONSE_SIZE_FIELD_WIDTH) << std::right << formatSize(responseSize)
			<< GREY << " | " << RESET << std::setw(STATUS_FIELD_WIDTH) << statusStr.str() << RESET
			<< GREY << " | " << RESET << std::left << timingStr.str();
	output << CLEARLINE;
	std::cout << output.str();
	std::cout.flush();
}

// Resets timing and completion counters for the current request group.
void Logger::resetTimingState()
{
	_state.minTime = std::numeric_limits<time_t>::max();
	_state.maxTime = 0;
	_state.lastEndRequestSize = std::numeric_limits<size_t>::max();
	_state.lastEndStatus = -1;
	_state.groupEndCount = 0;
}

// Resets all grouping state so the next request starts a fresh group.
void Logger::resetGroupState()
{
	resetTimingState();
	_state.pendingRequest = false;
	_state.lastMethod = "";
	_state.lastUri = "";
	_state.requestCount = 0;
}

// Formats "servername:port" right-aligned and truncated to SERVER_PORT_FIELD_WIDTH.
std::string Logger::formatServerPort(const std::string& serverName, int serverPort)
{
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
	return displayServerName + portPart;
}

// Formats the URI column: computes size hint, truncates URI, appends group count or pending "...".
std::string Logger::formatUriField(const std::string& uri, size_t declaredSize, size_t requestSize, bool includeCompletion)
{
	// Clean URI: replace non-printable characters with '?'
	std::string displayUri = uri;
	for (size_t i = 0; i < displayUri.length(); i++)
		if (displayUri[i] < 32 || displayUri[i] == 127)
			displayUri[i] = '?';

	// Compute upload size hint shown after URI for POST
	std::string hintStr;
	size_t hintLen = 0;
	{
		size_t sz = declaredSize;
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
	if (_state.requestCount > 1)
	{
		std::stringstream ss;
		ss << "(x" << _state.requestCount << ")";
		countLen = ss.str().length();
		countStr = std::string(GREY) + ss.str() + RESET;
	}

	// Truncate URI to fit
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
	}
	else
	{
		if (!includeCompletion)
			uriField << GREY << " ..." << RESET;
		size_t totalUsed = usedLen + (!includeCompletion ? 4 : 0);
		if (totalUsed < URI_FIELD_WIDTH)
			uriField << std::string(URI_FIELD_WIDTH - totalUsed, ' ');
	}
	return uriField.str();
}

// Public method(s) ------------------------------------------------------------

void Logger::requestStart(const RequestInfo& info)
{
	if (_state.lastLineType == LINE_NONE)
	{
		_state.logging = true;
		printSeparator();
		std::cout << std::endl;
		_state.currentLine++;
	}
	else if (_state.lastLineType == LINE_SEPARATOR)
	{
		// Start a new line after the closing separator
		std::cout << std::endl;
		_state.currentLine++;
	}
	_state.lastLineType = LINE_REQUEST;

	// Store request data for this specific request
	RequestData data;
	data.info = info;
	data.requestStartTime = getCurrentTime();
	data.displayLine = _state.currentLine;
	_state.activeRequests[info.requestId] = data;

	bool isGroupableRequest = (_state.lastMethod == info.method && _state.lastUri == info.uri
		&& _state.lastClientIP == info.clientIP && _state.lastServerName == info.serverName
		&& _state.lastServerPort == info.port);

	if (_state.pendingRequest && !isGroupableRequest)
	{
		// Snapshot the current group count into all active requests on this line
		// before switching groups, so requestEnd can restore it for the cursor-up flush
		for (requestMap::iterator it = _state.activeRequests.begin(); it != _state.activeRequests.end(); ++it)
			if (it->first != info.requestId && it->second.displayLine == _state.currentLine)
				it->second.finalGroupCount = _state.requestCount;
		std::cout << std::endl;
		_state.currentLine++;
		resetTimingState();
	}

	if (isGroupableRequest && _state.pendingRequest)
	{
		_state.requestCount++;
		return;
	}

	// New request - initialize
	_state.requestCount = 1;
	_state.lastMethod = info.method;
	_state.lastUri = info.uri;
	_state.lastClientIP = info.clientIP;
	_state.lastServerName = info.serverName;
	_state.lastServerPort = info.port;
	_state.lastRequestStartTime = data.requestStartTime;
	_state.pendingRequest = true;
	_state.lastDisplayedRequestId = info.requestId;

	// Record line AFTER potential endl above
	_state.activeRequests[info.requestId].displayLine = _state.currentLine;

	flushRequestLine(info.requestId, false, 0, 0, 0);
}

void Logger::requestEnd(const RequestInfo& info)
{
	if (!_state.pendingRequest)
		return;

	// Check if this request is part of the currently displayed group
	requestMap::iterator it = _state.activeRequests.find(info.requestId);
	if (it == _state.activeRequests.end())
		return; // already logged or unknown request
	bool isGroupedRequest = (it->second.info.method == _state.lastMethod
		&& it->second.info.uri == _state.lastUri
		&& it->second.info.clientIP == _state.lastClientIP
		&& it->second.info.serverName == _state.lastServerName
		&& it->second.info.port == _state.lastServerPort);

	// Break the group if request body size or status differs from previous completion
	if (isGroupedRequest && _state.groupEndCount
		&& (info.requestSize != _state.lastEndRequestSize || info.statusCode != _state.lastEndStatus))
	{
		std::cout << std::endl;
		_state.currentLine++;
		_state.requestCount = 1;
		resetTimingState();
	}

	if (isGroupedRequest)
	{
		// Part of the current group - update timing and overwrite current line
		if (info.responseTime < _state.minTime) _state.minTime = info.responseTime;
		if (info.responseTime > _state.maxTime) _state.maxTime = info.responseTime;
		_state.lastEndRequestSize = info.requestSize;
		_state.lastEndStatus = info.statusCode;
		_state.groupEndCount++;
		flushRequestLine(info.requestId, true, info.statusCode, info.requestSize, info.responseSize);
		_state.lastDisplayedRequestId = info.requestId;
	}
	else if (it != _state.activeRequests.end())
	{
		// Non-grouped request on a previous line - overwrite it in place
		int linesUp = _state.currentLine - it->second.displayLine;

		// Save current group state
		size_t savedCount = _state.requestCount;
		time_t savedMin = _state.minTime;
		time_t savedMax = _state.maxTime;

		// Set state for this individual request
		_state.requestCount = (it->second.finalGroupCount) ? it->second.finalGroupCount : 1;
		_state.minTime = info.responseTime;
		_state.maxTime = info.responseTime;

		if (linesUp > 0)
			std::cout << "\033[" << linesUp << "A"; // cursor up
		flushRequestLine(info.requestId, true, info.statusCode, info.requestSize, info.responseSize);
		if (linesUp > 0)
			std::cout << "\033[" << linesUp << "B"; // cursor down
		std::cout.flush();

		// Restore group state
		_state.requestCount = savedCount;
		_state.minTime = savedMin;
		_state.maxTime = savedMax;
	}

	_state.activeRequests.erase(info.requestId);
}

void Logger::logMessage(const std::string& message)
{
	if (_state.pendingRequest)
	{
		std::cout << std::endl;
		_state.currentLine++;
		_state.lastLineType = LINE_REQUEST;
	}

	// Opening separator only if last line wasn't already one
	if (_state.lastLineType != LINE_SEPARATOR)
	{
		printSeparator();
		std::cout << std::endl;
		_state.currentLine++;
	}

	// Message content
	std::cout << message;
	for (size_t i = 0; i < message.length(); i++)
		if (message[i] == '\n')
			_state.currentLine++;
	if (message.empty() || message[message.length() - 1] != '\n')
	{
		std::cout << std::endl;
		_state.currentLine++;
	}

	// Closing separator (no trailing newline — next logRequestStart will handle it)
	printSeparator();
	_state.lastLineType = LINE_SEPARATOR;
	std::cout.flush();

	// Break any pending group — next identical request starts fresh
	resetGroupState();
}

bool Logger::hasStarted()
{
	return _state.logging;
}

void Logger::printSeparator()
{
	std::cout << GREY << std::string(LOG_SEPARATOR_WIDTH, '-') << RESET;
}
