/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:51 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/09 11:02:25 by gdosch           ###   ########.fr       */
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

			std::vector<ServerConfig> _configs;
			std::vector<Client> _clients;
			std::vector<pollfd> _pollFds;
			std::vector<int> _listenSockets;
			bool _running;
			Router _router;
			std::map<std::string, SessionData> _sessions;
			// int _g_sigpipe[2];

	public:

		// Special member function(s)

			Server(const Config& config);
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

			//static volatile sig_atomic_t s_stop;
			static int s_sigpipe[2];

			static void signalHandler(int sig);
			void installSignals();
			void addSignalPipeToPoll();
			void handleSignalPipeReadable();
};
