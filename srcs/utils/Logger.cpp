/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:38 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/12 13:10:22 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------
#include "Logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

// Static variables initialization ---------------------------------------------
// timeval Logger::_lastRequestTime = {0, 0};
std::string Logger::_lastMethod = "";
std::string Logger::_lastUri = "";
std::string Logger::_lastClientIP = "";
int Logger::_lastStatus = 0;
size_t Logger::_lastSize = 0;
size_t Logger::_requestCount = 0;
double Logger::_totalTime = 0.0;
double Logger::_minTime = 0.0;
double Logger::_maxTime = 0.0;
std::string Logger::_lastServerName = "";
int Logger::_lastServerPort = 0;
bool Logger::_firstLog = true;
bool Logger::_pendingRequest = false;
bool Logger::_currentIsSameAsPrevious = false;

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

void Logger::flushGroupedRequests()
{
	if (_requestCount == 0)
		return;

	std::string statusColor = getStatusColor(_lastStatus);
	std::string methodColor = (_lastMethod == "GET") ? GREEN : (_lastMethod == "HEAD") ? CYAN : (_lastMethod == "POST") ? YELLOW : (_lastMethod == "DELETE") ? RED : GREY;

	int uriFieldWidth = 42;

	// Format count suffix and calculate its actual length
	std::stringstream countStr;
	int actualCountLen = 0;
	if (_requestCount > 1) {
		countStr << GREY << "(" << _requestCount << ")" << RESET;
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

	// Center-align client IP
	int ipFieldWidth = 15;
	int ipLen = _lastClientIP.length();
	int ipPadding = (ipFieldWidth - ipLen) / 2;
	std::string centeredIP = std::string(ipPadding, ' ') + _lastClientIP + std::string(ipFieldWidth - ipLen - ipPadding, ' ');

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

	// Format server:port with fixed width (20 chars max)
	int serverFieldWidth = 20;
	std::stringstream portStr;
	portStr << ":" << _lastServerPort;
	std::string portPart = portStr.str();

	// Calculate max space for server name
	int maxServerNameLen = serverFieldWidth - portPart.length();
	std::string serverName = _lastServerName;

	// Truncate server name if needed
	if ((int)serverName.length() > maxServerNameLen && maxServerNameLen >= 2) {
		serverName = serverName.substr(0, maxServerNameLen - 2) + "..";
	} else if ((int)serverName.length() > maxServerNameLen) {
		serverName = serverName.substr(0, maxServerNameLen);
	}

	std::string serverPortStr = serverName + portPart;

	// Format timing
	std::stringstream timingStr;
	if (_requestCount > 1) {
		// Multiple requests: determine unit for min and max
		enum Unit { US, MS, S };
		Unit minUnit = (_minTime < 1.0) ? US : (_minTime < 1000.0) ? MS : S;
		Unit maxUnit = (_maxTime < 1.0) ? US : (_maxTime < 1000.0) ? MS : S;
		// Format based on unit combinations
		if (minUnit == maxUnit) {
			// Same unit: format together
			if (minUnit == US)
				timingStr << std::fixed << std::setprecision(0) << (_minTime * 1000);
			else if (minUnit == MS)
				timingStr << std::fixed << std::setprecision(1) << _minTime;
			else // S
				timingStr << std::fixed << std::setprecision(2) << (_minTime / 1000.0);
		} else {
			// Different units: format separately
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
		timingStr << std::fixed << std::setprecision(2) << (_maxTime / 1000.0) << "s";

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
				<< std::setw(20) << std::left << serverPortStr << " "
				<< GREY << "|" << RESET << " "
				<< "[" << getCurrentTime() << "] "
				<< GREY << "|" << RESET << " "
				<< centeredIP << " "
				<< GREY << "|" << RESET << " "
				<< methodColor << BOLD << std::setw(7) << std::right << _lastMethod << RESET << " "
				<< uriField.str() << " "
				<< GREY << "|" << RESET << " "
			<< formatSize(_lastSize) << " "
				<< GREY << "|" << RESET << " "
				<< GREY << "→" << RESET << " "
				<< statusColor << BOLD << _lastStatus << RESET << " "
				<< GREY << "|" << RESET << " "
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

// Public method(s) ------------------------------------------------------------
void Logger::logRequestStart(const std::string &method, const std::string &uri,
								const std::string &clientIP, std::string serverName, int port)
{
	// Display separator before first log
	if (_firstLog)
	{
		printSeparator();
		_firstLog = false;
	}

	// Check if this request is identical to the previous one
	bool isSameRequest = (_lastMethod == method && _lastUri == uri && 
						  _lastClientIP == clientIP && _lastServerName == serverName && 
						  _lastServerPort == port);

	// If there's a pending request and it's not the same, finalize it first
	if (_pendingRequest && !isSameRequest)
	{
		std::cout << std::endl;
		_requestCount = 0;
		_minTime = 0.0;
		_maxTime = 0.0;
		_totalTime = 0.0;
	}

	// If same request, increment counter and use \r to update same line
	if (isSameRequest && _pendingRequest)
	{
		_requestCount++;
		// Don't display anything here - logRequestEnd will update the display
		return;
	}
	else
	{
		_requestCount = 1;
		_lastMethod = method;
		_lastUri = uri;
		_lastClientIP = clientIP;
		_lastServerName = serverName;
		_lastServerPort = port;
	}

	_currentIsSameAsPrevious = isSameRequest && _pendingRequest;
	_pendingRequest = true;

	// Format the display using the same layout as logRequestEnd
	std::string methodColor = (_lastMethod == "GET") ? GREEN : (_lastMethod == "HEAD") ? CYAN : (_lastMethod == "POST") ? YELLOW : (_lastMethod == "DELETE") ? RED : GREY;

	int uriFieldWidth = 42;

	// Format count suffix and calculate its actual length
	std::stringstream countStr;
	int actualCountLen = 0;
	if (_requestCount > 1) {
		countStr << GREY << "(" << _requestCount << ")" << RESET;
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

	// Center-align client IP
	int ipFieldWidth = 15;
	int ipLen = _lastClientIP.length();
	int ipPadding = (ipFieldWidth - ipLen) / 2;
	std::string centeredIP = std::string(ipPadding, ' ') + _lastClientIP + std::string(ipFieldWidth - ipLen - ipPadding, ' ');

	// Truncate URI if necessary
	std::string displayUri = _lastUri;
	if ((int)displayUri.length() > maxUriLen) {
		if (maxUriLen >= 2)
			displayUri = displayUri.substr(0, maxUriLen - 2) + "..";
		else
			displayUri = ".."; // Fallback if really too small
	}

	// Format server:port with fixed width (20 chars max)
	int serverFieldWidth = 20;
	std::stringstream portStr;
	portStr << ":" << _lastServerPort;
	std::string portPart = portStr.str();

	// Calculate max space for server name
	int maxServerNameLen = serverFieldWidth - portPart.length();
	std::string sName = _lastServerName;

	// Truncate server name if needed
	if ((int)sName.length() > maxServerNameLen && maxServerNameLen >= 2) {
		sName = sName.substr(0, maxServerNameLen - 2) + "..";
	} else if ((int)sName.length() > maxServerNameLen) {
		sName = sName.substr(0, maxServerNameLen);
	}

	std::string serverPortStr = sName + portPart;

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

	// Display the request info (first part) - same format as logRequestEnd
	std::cout << "\r"
				<< std::setw(20) << std::left << serverPortStr << " "
				<< GREY << "|" << RESET << " "
				<< "[" << getCurrentTime() << "] "
				<< GREY << "|" << RESET << " "
				<< centeredIP << " "
				<< GREY << "|" << RESET << " "
				<< methodColor << BOLD << std::setw(7) << std::right << _lastMethod << RESET << " "
				<< uriField.str() << " "
				<< GREY << "|" << RESET
				<< "\033[K"; // Clear to end of line
	std::cout << std::flush;
}

void Logger::logRequestEnd(int statusCode, size_t responseSize, double responseTime)
{
	if (!_pendingRequest)
		return;

	// Use the responseTime calculated in buildResponse/buildResponseFromCGI
	// (calculated correctly from start to end of each individual request)
	
	// Update timing stats
	if (_requestCount == 1 || _minTime == 0.0)
	{
		_minTime = responseTime;
		_maxTime = responseTime;
	}
	else
	{
		if (responseTime < _minTime) _minTime = responseTime;
		if (responseTime > _maxTime) _maxTime = responseTime;
	}
	_totalTime += responseTime;

	// Store status and size for next comparison
	_lastStatus = statusCode;
	_lastSize = responseSize;

	// Now format and display using the same logic as flushGroupedRequests
	std::string statusColor = getStatusColor(_lastStatus);
	std::string methodColor = (_lastMethod == "GET") ? GREEN : (_lastMethod == "HEAD") ? CYAN : (_lastMethod == "POST") ? YELLOW : (_lastMethod == "DELETE") ? RED : GREY;

	int uriFieldWidth = 42;

	// Format count suffix and calculate its actual length
	std::stringstream countStr;
	int actualCountLen = 0;
	if (_requestCount > 1) {
		countStr << GREY << "(" << _requestCount << ")" << RESET;
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

	// Center-align client IP
	int ipFieldWidth = 15;
	int ipLen = _lastClientIP.length();
	int ipPadding = (ipFieldWidth - ipLen) / 2;
	std::string centeredIP = std::string(ipPadding, ' ') + _lastClientIP + std::string(ipFieldWidth - ipLen - ipPadding, ' ');

	// Truncate URI if necessary
	std::string displayUri = _lastUri;
	if ((int)displayUri.length() > maxUriLen) {
		if (maxUriLen >= 2)
			displayUri = displayUri.substr(0, maxUriLen - 2) + "..";
		else
			displayUri = ".."; // Fallback if really too small
	}

	// Format server:port with fixed width (20 chars max)
	int serverFieldWidth = 20;
	std::stringstream portStr;
	portStr << ":" << _lastServerPort;
	std::string portPart = portStr.str();

	// Calculate max space for server name
	int maxServerNameLen = serverFieldWidth - portPart.length();
	std::string serverName = _lastServerName;

	// Truncate server name if needed
	if ((int)serverName.length() > maxServerNameLen && maxServerNameLen >= 2) {
		serverName = serverName.substr(0, maxServerNameLen - 2) + "..";
	} else if ((int)serverName.length() > maxServerNameLen) {
		serverName = serverName.substr(0, maxServerNameLen);
	}

	std::string serverPortStr = serverName + portPart;

	// Format timing
	std::stringstream timingStr;
	if (_requestCount > 1) {
		// Multiple requests: determine unit for min and max
		enum Unit { US, MS, S };
		Unit minUnit = (_minTime < 1.0) ? US : (_minTime < 1000.0) ? MS : S;
		Unit maxUnit = (_maxTime < 1.0) ? US : (_maxTime < 1000.0) ? MS : S;
		// Format based on unit combinations
		if (minUnit == maxUnit) {
			// Same unit: format together
			if (minUnit == US)
				timingStr << std::fixed << std::setprecision(0) << (_minTime * 1000);
			else if (minUnit == MS)
				timingStr << std::fixed << std::setprecision(1) << _minTime;
			else // S
				timingStr << std::fixed << std::setprecision(2) << (_minTime / 1000.0);
		} else {
			// Different units: format separately
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
		timingStr << std::fixed << std::setprecision(2) << (_maxTime / 1000.0) << "s";

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
				<< std::setw(20) << std::left << serverPortStr << " "
				<< GREY << "|" << RESET << " "
				<< "[" << getCurrentTime() << "] "
				<< GREY << "|" << RESET << " "
				<< centeredIP << " "
				<< GREY << "|" << RESET << " "
				<< methodColor << BOLD << std::setw(7) << std::right << _lastMethod << RESET << " "
				<< uriField.str() << " "
				<< GREY << "|" << RESET << " "
			<< formatSize(_lastSize) << " "
				<< GREY << "|" << RESET << " "
				<< GREY << "→" << RESET << " "
				<< statusColor << BOLD << _lastStatus << RESET << " "
				<< GREY << "|" << RESET << " "
				<< std::setw(6) << std::right << timingStr.str()
				<< "\033[K"; // ANSI escape: clear to end of line
	std::cout.flush();
}

void Logger::logRequest(const std::string &method, const std::string &uri,
								const std::string &clientIP, int statusCode,
								size_t responseSize, double responseTime,
								 std::string serverName, int port)
{
	// Display separator before first log
	if (_firstLog)
	{
		printSeparator();
		_firstLog = false;
	}

	// Get time
	// timeval now;
	// gettimeofday(&now, NULL);

	// Check if this request is identical to the previous one
	bool isSameRequest = (_lastMethod == method && _lastUri == uri &&
						  _lastStatus == statusCode && _lastSize == responseSize);

	// Check for inactivity (separator between bursts)
	// bool isInactive = false;
	// if (_lastRequestTime.tv_sec != 0) {
	// 	long timeDiff = (now.tv_sec - _lastRequestTime.tv_sec) * 1000 +
	// 					(now.tv_usec - _lastRequestTime.tv_usec) / 1000;
	// 	isInactive = (timeDiff > 100);
	// }

	// If different request or inactive period, finalize and start new group
	if (!isSameRequest/* || isInactive*/) {
		finalizeGroupedRequests();

		// if (isInactive && _lastRequestTime.tv_sec)
		// 	Logger::printSeparator();

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

	// _lastRequestTime = now;
}

void Logger::logMessage(const std::string &message)
{
	// Force finalization to ensure log line is complete
	if (_requestCount > 0)
	{
		std::cout << std::endl;
		_requestCount = 0;
		_totalTime = 0.0;
		_minTime = 0.0;
		_maxTime = 0.0;
	}

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
