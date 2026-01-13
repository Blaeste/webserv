/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/09 11:33:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/09 13:31:37 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <sys/time.h>

// Colors
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define GRAY    "\033[90m"
#define BOLD    "\033[1m"

class Logger {

	private:
		static timeval _lastRequestTime;

	public:
		static void logRequest(const std::string &method, const std::string &uri,
								const std::string &clientIP, int statusCode,
								size_t responseSize, double responseTime,
								 std::string serverName, int port);

		static void logConnection(int fd, const std::string &clientIP);

		static void logError(const std::string &message);

		static std::string getCurrentTime();

		static std::string formatSize(size_t bytes);

		static std::string getStatusColor(int statusCode);
};
