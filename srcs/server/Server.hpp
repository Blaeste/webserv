/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:51 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/02 14:04:11 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s) ******************************************************************
#include "Client.hpp"
#include "Router.hpp"
#include "../config/Config.hpp"
#include <ctime>
#include <map>
#include <poll.h>
#include <string>
#include <vector>

// Structure(s) ****************************************************************
struct SessionData {
	time_t lastActive; ///< Timestamp of last session activity
	std::string username; ///< Username associated with the session
	int visitCount; ///< Number of page visits for this session
};

// Enum(s) *********************************************************************
enum SocketType {
	SOCKET_LISTEN,
	SOCKET_CLIENT,
	SOCKET_SIGNAL,
	SOCKET_CGI
};

// Class ***********************************************************************
class Server {
	private:
		// Attribute(s) --------------------------------------------------------
		enum {
			SESSION_TIMEOUT = 1800, // 30 minutes
			SESSION_CLEANUP_INTERVAL = 60, // 1 minute
			CLIENT_KEEPALIVE_TIMEOUT = 75, // 75 seconds - prevents zombie connections and would serve as keep-alive timeout if implemented
			CLIENT_PROCESSING_TIMEOUT = 180, // 3 minutes - request processing timeout, including CGI execution
			DEFAULT_CGI_EXECUTION_TIMEOUT = 90 // 90 seconds - single CGI execution timeout (prevents hanging scripts)
		};

		std::vector<ServerConfig> _configs; ///< Server configurations
		std::vector<pollfd> _pollFds; ///< Poll file descriptors for I/O multiplexing
		std::map<int, Client> _clients; ///< Active client connections
		std::map<int, SocketType> _socketTypes; ///< Socket type mapping
		bool _running; ///< Server running state
		Router _router; ///< Request router
		std::map<std::string, SessionData> _sessions; ///< Active user sessions
		static int _s_sigpipe[2]; ///< Self-pipe for signal handling
		time_t _lastSessionCleanup; ///< Timestamp of last session cleanup

	public:
		// Special member function(s) ------------------------------------------
		explicit Server(const Config& config);
		~Server();

		// Public method(s) ----------------------------------------------------
		void run();
		void stop();

	private:
		// Private method(s) ---------------------------------------------------
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

		// Logging
		void logClientResponse(Client &client);
};
