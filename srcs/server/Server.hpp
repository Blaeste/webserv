/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:51 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/14 22:06:45 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
# define SERVER_HPP

// Include(s) ------------------------------------------------------------------

# include "Client.hpp"
# include "Router.hpp"
# include "../config/ConfigParser.hpp"
# include <map>						// std::map
# include <string>					// std::string
# include <vector>					// std::vector
# include <ctime>					// time_t
# include <poll.h>					// pollfd

// Structure(s) ----------------------------------------------------------------

struct	SessionData
{
	time_t		lastActive;	// Timestamp of last session activity
	std::string	username;	// Username associated with the session
	int			visitCount;	// Number of page visits for this session
};

// Enum(s) ---------------------------------------------------------------------

enum	SocketType
{
	SOCKET_LISTEN,
	SOCKET_CLIENT,
	SOCKET_SIGNAL,
	SOCKET_CGI
};

// Typedef(s) ------------------------------------------------------------------

typedef	std::map<int, SocketType>			socketTypeMap;
typedef std::map<int, Client>				ClientMap;
typedef	std::vector<pollfd>					PollfdVector;

// Class -----------------------------------------------------------------------

class	Server
{
	private:

		// Constant(s)

		static const int	POLL_TIMEOUT_MS					= 1000;	// 1 second
		static const int	LISTEN_BACKLOG					= 128;	// TCP listen queue size
		static const size_t	SIGNAL_PIPE_BUFFER_SIZE			= 64;	// Drain buffer for self-pipe
		static const time_t	SESSION_TIMEOUT					= 1800;	// 30 minutes
		static const time_t	SESSION_CLEANUP_INTERVAL		= 60;	// 1 minute
		static const time_t	CLIENT_KEEPALIVE_TIMEOUT		= 75;	// 75 seconds - max idle time on keep-alive connections
		static const time_t	CLIENT_PROCESSING_TIMEOUT		= 180;	// 3 minutes - request processing timeout, including CGI execution

		// Attribute(s)

		serverBlockVector	_servers;				// Server configurations
		PollfdVector		_pollFds;				// Poll file descriptors for I/O multiplexing
		ClientMap			_clients;				// Active client connections
		socketTypeMap		_socketTypes;			// Socket type mapping
		bool				_running;				// Server running state
		Router				_router;				// Request router
		SessionMap			_sessions;				// Active user sessions
		static int			_s_sigpipe[2];			// Self-pipe for signal handling
		time_t				_lastSessionCleanup;	// Timestamp of last session cleanup

		// Private method(s)

		void				acceptNewClient(int listenSocket);
		void				handleClientTimeouts();
		ClientMap::iterator	removeClient(ClientMap::iterator it);
		void				handleCgiTimeouts();
		void				closePollFd(int& fd);
		void				killCgiProcess(Client& client);
		void				handleSessionTimeouts();
		static void			signalHandler(int sig);
		void				installSignals();

		/** @brief Creates, binds and adds one listening socket per configured port to the poll set. */
		void				setupListenSockets();

		/** @brief Validates request body size against configured limits; returns true if exceeded. */
		bool				checkBodySize(Client& client, const ServerBlock* server, size_t clientIndex);

		/** @brief Registers CGI process pipes (stdin, stdout, stderr) in the poll set for monitoring. */
		void				registerCgiPipes(CgiProcess* cgi, size_t clientIndex);

		/** @brief Spawns a CGI process, sets up pipes, and registers them for I/O multiplexing. */
		void				startCgi(Client& client, const RouteMatch& match, size_t clientIndex, const ServerBlock* server);

		/** @brief Routes the client request and builds the appropriate HTTP response. */
		bool				buildClientResponse(Client& client, const ServerBlock* server, size_t clientIndex);

		/** @brief Processes POLLIN for the client at _pollFds[clientIndex]. */
		void				handleClientRead(size_t clientIndex);

		/** @brief Processes POLLOUT for the client at _pollFds[clientIndex]. */
		void				handleClientWrite(size_t clientIndex);

		/** @brief Picks the ServerBlock matching the Host header, or falls back to the first config. */
		const ServerBlock*	getServerBlock(const HttpRequest& request, int clientFd) const;

		/** @brief Reads available CGI stdout/stderr and triggers response finalization on EOF. */
		void				handleCgiPipe(size_t pipeIndex);

		/** @brief Collects CGI output, parses its headers, and builds the client HTTP response. */
		void				finalizeCgi(Client& client, CgiProcess* cgi, int clientFd);

		/** @brief Handles POLLERR/POLLHUP on _pollFds[i] (client disconnect, broken pipe, etc.). */
		void				handleSocketError(size_t i);

		/** @brief Updates the events mask (POLLIN/POLLOUT) for the given fd in _pollFds. */
		void				setPollEvents(int fd, short events);

		/** @brief Drains the self-pipe after a signal to prevent poll() from busy-looping. */
		void				handleSignalPipeReadable();

		/** @brief Registers the self-pipe read end in _pollFds for signal-safe interruption. */
		void				addSignalPipeToPoll();

	public:

		// Special member function(s)

		explicit Server(const ConfigParser& config);
		~Server();

		// Setter(s)

		void				stop() { _running = false; }

		// Public method(s)

		/** @brief Enters the main poll() event loop; blocks until stop() is called or a signal is caught. */
		void				run();
};

#endif
