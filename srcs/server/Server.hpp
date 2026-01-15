/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:51 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/15 13:00:51 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s)
#include "Client.hpp"
#include "Router.hpp"
#include "../config/Config.hpp"
#include <ctime>
#include <map>
#include <poll.h>
#include <signal.h>
#include <string>
#include <vector>
#include <algorithm>

// Forward declaration(s)
class HttpRequest;

// Structure(s)
struct SessionData {

	time_t lastActive;
	std::string username;
	int visitCount;

};

// Enum(s)
enum SocketType {
	SOCKET_LISTEN,
	SOCKET_CLIENT,
	SOCKET_SIGNAL,
	SOCKET_CGI
};

// Class
class Server {

	private:

		// Attribute(s)

			enum {
				SESSION_TIMEOUT = 1800, // 30 minutes
				SESSION_CLEANUP_INTERVAL = 60, // 1 minute
				CLIENT_IDLE_TIMEOUT = 30, // 30 seconds
				CLIENT_PROCESSING_TIMEOUT = 300 // 5 minutes - timeout for processing/CGI execution
			};
			std::vector<ServerConfig> _configs;
			std::vector<pollfd> _pollFds;
			std::map<int, Client> _clients;
			std::map<int, SocketType> _socketTypes;
			bool _running;
			Router _router;
			std::map<std::string, SessionData> _sessions;
			static int _s_sigpipe[2];
			time_t _lastSessionCleanup;

	public:

		// Special member function(s)

			explicit Server(const Config& config);
			~Server();

		// Public method(s)

			void run();
			void stop();

	private:

		// Private method(s)

			// Socket management
			void setupListenSockets();
			void acceptNewClient(int listenSocket);

			// Client lifecycle
			void handleClientTimeouts();
			void handleClientRead(size_t clientIndex);
			void handleClientWrite(size_t clientIndex);
			void removeClient(int fd, size_t pollIndex);
			const ServerConfig* selectConfig(const HttpRequest& request, int clientFd) const;

			// CGI handling
			void handleCGITimeouts();
			void handleCGIPipe(size_t pipeIndex);

			// Session management
			void handleSessionTimeouts();

			// Signal handling (self-pipe trick for async-signal-safety)
			void handleSignalPipeReadable();
			void addSignalPipeToPoll();
			static void signalHandler(int sig);
			void installSignals();

};
