/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:38 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/20 13:31:16 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "Logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>

// Initialisation de la variable statique
timeval Logger::_lastRequestTime = {0, 0};

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

void Logger::logRequest(const std::string &method, const std::string &uri,
								const std::string &clientIP, int statusCode,
								size_t responseSize, double responseTime,
								 std::string serverName, int port)
{
	std::string statusColor = getStatusColor(statusCode);
	std::string methodColor = (method == "GET") ? BLUE : (method == "POST") ? MAGENTA : CYAN;

	// Get time
	timeval now;
	gettimeofday(&now, NULL);

	// if inactiv put separator
	if (_lastRequestTime.tv_sec != 0) {
		long timeDiff = (now.tv_sec - _lastRequestTime.tv_sec) * 1000 +
						(now.tv_usec - _lastRequestTime.tv_usec) / 1000;
		if (timeDiff > 100) {
			std::cout << GRAY << std::string(91, '-') << RESET << std::endl;
		}
	}

	// Truncate URI if too long
	std::string displayUri = uri;
	if (displayUri.length() > 20) {
		displayUri = displayUri.substr(0, 18) + "..";
	}

	// log
	std::stringstream timeStr;
	if (responseTime < 1.0)
		timeStr << std::fixed << std::setprecision(0) << std::setw(4) << (responseTime * 1000) << "µs";
	else
		timeStr << std::fixed << std::setprecision(1) << std::setw(5) << responseTime << "ms";

	std::cout
				<< serverName << ":" << port << " "
				<< "[" << getCurrentTime() << "] "
				<< methodColor << BOLD << std::setw(7) << std::left << method << RESET << " "
				<< std::setw(20) << std::left << displayUri << " "
				<< GRAY << "→" << RESET << " "
				<< statusColor << BOLD << statusCode << RESET << " "
				<< GRAY << "|" << RESET << " "
				<< formatSize(responseSize) << " "
				<< GRAY << "|" << RESET << " "
				<< timeStr.str() << " "
				<< GRAY << "|" << RESET << " "
				<< GRAY << "(" << clientIP << ")" << RESET
				<< std::endl;

	// keep last time stamp
	_lastRequestTime = now;
}

void Logger::logConnection(int fd, const std::string &clientIP)
{
	// Supprimé pour éviter le spam - info visible dans le log de requête
	(void)fd;
	(void)clientIP;
}

void Logger::logError(const std::string &message)
{
	std::cout << RED << "❌ Error: " << RESET << message << std::endl;
}


