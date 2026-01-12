/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:51 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/12 12:57:12 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s)
#include "Client.hpp"
#include "Router.hpp"
#include "../config/Config.hpp"
#include <ctime>
#include <poll.h>
#include <vector>
#include <map>
#include <string>
#include <signal.h>

// Forward declaration(s)
class HttpRequest;

// Structure(s)
struct SessionData {

	time_t lastActive;
	std::string username;
	int visitCount;

};

// Class
class Server {

	private:

		// Attribute(s)

			enum {
				SESSION_TIMEOUT = 1800, // 30 minutes
				SESSION_CLEANUP_INTERVAL = 60, // 1 minute
				CLIENT_READ_TIMEOUT = 30, // 30 seconds
				CLIENT_PROCESSING_TIMEOUT = 300 // 5 minutes - timeout for processing/CGI execution
			};
			std::vector<ServerConfig> _configs;
			std::vector<Client> _clients;
			std::vector<pollfd> _pollFds;
			std::vector<int> _listenSockets;
			bool _running;
			Router _router;
			std::map<std::string, SessionData> _sessions;
			static int _s_sigpipe[2];

	public:

		// Special member function(s)

			explicit Server(const Config& config);
			~Server();

		// Public method(s)

			void run();
			void stop();

	private:

		// private method(s)

			void checkTimeouts();
			void setupListenSockets();
			bool isListenSocket(int fd) const;
			void acceptNewClient(int listenSocket);
			void cleanupSessions();

		// Client handling

			const ServerConfig* selectConfig(const HttpRequest& request, int clientFd) const;
			void handleClientRead(size_t clientIndex);
			void handleClientWrite(size_t clientIndex);

		// Signal handling

			static void signalHandler(int sig);
			void installSignals();
			void addSignalPipeToPoll();
			void handleSignalPipeReadable();

};
