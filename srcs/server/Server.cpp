/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:49 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/16 12:07:19 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------

#include "Server.hpp"
#include "../cgi/Cgi.hpp"
#include "../utils/Logger.hpp"
#include "../utils/utils.hpp"
#include <algorithm>			// std::find
#include <iostream>				// std::cout, std::endl
#include <limits>				// std::numeric_limits
#include <sstream>				// std::stringstream
#include <stdexcept>			// std::runtime_error
#include <cerrno>				// errno
#include <csignal>				// signal, kill, SIG_IGN, SIGINT, SIGKILL, SIGPIPE, SIGQUIT, SIGTERM
#include <cstring>				// std::memset, std::strerror
#include <netinet/in.h>			// sockaddr_in, htons, ntohs, INADDR_ANY
#include <sys/socket.h>			// socket, bind, listen, accept, setsockopt, getsockname
#include <sys/wait.h>			// waitpid, WNOHANG, WIFEXITED, WEXITSTATUS, WIFSIGNALED
#include <unistd.h>				// read, write, close, pipe

// Define(s) -------------------------------------------------------------------

#define CURSOR_HIDE "\033[?25l"
#define CURSOR_SHOW "\033[?25h"

// Static helper(s) ------------------------------------------------------------

static int getSocketPort(int fd)
{
	sockaddr_in addr;
	socklen_t len = sizeof(addr);
	if (getsockname(fd, (sockaddr*)&addr, &len) == 0)
		return ntohs(addr.sin_port);
	return -1;
}

// Static variable initialization ----------------------------------------------

int Server::_s_sigpipe[2] = {-1, -1}; // Self-pipe for signal handling in poll()

// Special member function(s) --------------------------------------------------

Server::Server(const ConfigParser& config)
	: _servers(config.getServers())
	, _running(false)
	, _lastSessionCleanup(0)
{
	installSignals();
	setupListenSockets();
	std::cout << CURSOR_HIDE;
}

Server::~Server()
{
	// Kill all active CGI processes and clean up
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		CgiProcess* cgi = it->second.getCgiProcess();
		if (cgi)
		{
			kill(cgi->pid, SIGKILL);
			waitpid(cgi->pid, NULL, 0);
			delete cgi;
		}
	}
	// Close all active Client and CGI sockets (skip signal pipe at index 0)
	for (size_t i = 1; i < _pollFds.size(); i++)
	{
		if (_pollFds[i].fd != -1)
			safeClose(_pollFds[i].fd, "Server");
	}
	// Close signal pipe explicitly
	if (_s_sigpipe[0] >= 0)
		safeClose(_s_sigpipe[0], "Server");
	if (_s_sigpipe[1] >= 0)
		safeClose(_s_sigpipe[1], "Server");
	Logger::logMessage("Server was closed");
	std::cout << CURSOR_SHOW;
}

// Private method(s) -----------------------------------------------------------

// Called by OS when SIGINT/SIGTERM received (async-signal-safe)
void Server::signalHandler(int)
{
	if (_s_sigpipe[1] != -1)
	{
		ssize_t ret = write(_s_sigpipe[1], "1", 1);
		(void)ret;
	}
}

// Self-pipe trick: signals write to the pipe's write end; poll() reads the read end like any I/O event.
void Server::installSignals()
{
	if (pipe(_s_sigpipe) == -1)
		throw std::runtime_error(std::string("pipe() failed: ") + std::strerror(errno));
	setNonBlocking(_s_sigpipe[0]);
	setNonBlocking(_s_sigpipe[1]);

	pollfd pfd = { _s_sigpipe[0], POLLIN, 0 }; // Watch read end for incoming signal bytes
	_pollFds.push_back(pfd);
	_socketTypes[_s_sigpipe[0]] = SOCKET_SIGNAL;

	signal(SIGINT, Server::signalHandler); // Ctrl+C
	signal(SIGQUIT, Server::signalHandler); // Ctrl+backslash
	signal(SIGTERM, Server::signalHandler); // kill
	signal(SIGPIPE, SIG_IGN); // Ignore broken pipe on socket writes
}

void Server::setupListenSockets()
{
	// Create one listening socket per configuration (one per port)
	std::vector<int> port;
	for (size_t i = 0; i < _servers.size(); i++)
	{
		if (std::find(port.begin(), port.end(), _servers[i].getPort()) != port.end())
		{
			std::cout << "Server " << _servers[i].getServerName() << " listening on port " << _servers[i].getPort() << std::endl;
			continue; // Skip duplicate ports
		}
		port.push_back(_servers[i].getPort());

		// Create a TCP socket (file descriptor = entry point for network communication)
		int listenFd = socket(AF_INET, SOCK_STREAM, 0); // AF_INET = IPv4, SOCK_STREAM = TCP
		if (listenFd < 0)
			throw std::runtime_error("socket() failed");

		// Allow reuse of the port immediately after server restart
		// Without this, bind() would fail with "Address already in use" for ~60s (TIME_WAIT state)
		int opt = 1;
		if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		{
			safeClose(listenFd, "Server");
			throw std::runtime_error("setsockopt() failed");
		}

		// Define the local address the socket will be bound to (IP + port)
		struct sockaddr_in addr = {};
		addr.sin_family = AF_INET;						// IPv4 address family
		addr.sin_port = htons(_servers[i].getPort());	// Port from config, converted to network byte order (big-endian)
		addr.sin_addr.s_addr = INADDR_ANY;				// Accept connections on all local network interfaces (0.0.0.0)

		// Bind the socket fd to the address structure above
		// After this call, listenFd is associated with port _servers[i].getPort() on all interfaces
		if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
		{
			safeClose(listenFd, "Server");
			throw std::runtime_error("bind() failed");
		}

		// Start listening for incoming connections
		if (listen(listenFd, LISTEN_BACKLOG) < 0)
		{
			safeClose(listenFd, "Server");
			throw std::runtime_error("listen() failed");
		}

		setNonBlocking(listenFd);

		// Add to poll to monitor incoming connections
		pollfd pfd = { listenFd, POLLIN, 0 };
		_pollFds.push_back(pfd);
		_socketTypes[listenFd] = SOCKET_LISTEN;
		std::cout << "Server " << _servers[i].getServerName() << " listening on port " << _servers[i].getPort() << std::endl;
	}
}

// Marks the pollfd slot as -1, removes fd from the socket type map, then closes fd and sets it to -1.
void Server::closePollFd(int& fd)
{
	for (size_t i = 0; i < _pollFds.size(); ++i)
		if (_pollFds[i].fd == fd)
		{
			_pollFds[i].fd = -1;
			break;
		}
	_socketTypes.erase(fd);
	safeClose(fd, "Server");
}

// Kills a CGI process, closes and soft-deletes its pipes, frees memory.
void Server::killCgiProcess(Client& client)
{
	CgiProcess* cgi = client.getCgiProcess();
	if (!cgi)
		return;
	kill(cgi->pid, SIGKILL);
	waitpid(cgi->pid, NULL, 0);
	if (cgi->pipeOut != -1) closePollFd(cgi->pipeOut);
	if (cgi->pipeIn != -1) closePollFd(cgi->pipeIn);
	if (cgi->pipeErr != -1) closePollFd(cgi->pipeErr);
	delete cgi;
	client.setCgiProcess(NULL);
}

ClientMap::iterator Server::removeClient(ClientMap::iterator it)
{
	killCgiProcess(it->second);
	int fd = it->first;
	closePollFd(fd);
	ClientMap::iterator next = it;
	++next;
	_clients.erase(it);
	return next;
}

void Server::handleClientTimeouts()
{
	ClientMap::iterator it = _clients.begin();
	while (it != _clients.end())
	{
		if (it->second.hasTimedOut(CLIENT_KEEPALIVE_TIMEOUT, CLIENT_PROCESSING_TIMEOUT))
		{
			it->second.logRequestEnd();
			it = removeClient(it);
		}
		else
			++it;
	}
}

void Server::setPollEvents(int fd, short events)
{
	for (size_t i = 0; i < _pollFds.size(); ++i)
	{
		if (_pollFds[i].fd == fd)
		{
			_pollFds[i].events |= events;
			return;
		}
	}
}

const ServerBlock* Server::getServerBlock(const HttpRequest& request, int clientFd) const
{
	// Remove port from Host header if present
	std::string host = request.getHeader("Host");
	size_t colonPos = host.find(':');
	if (colonPos != std::string::npos)
		host.erase(colonPos);

	// Virtual host lookup: match server_name against Host header
	const int localPort = getSocketPort(clientFd);
	const ServerBlock* firstServer = NULL;
	for (size_t i = 0; i < _servers.size(); i++)
	{
		if (_servers[i].getPort() != localPort)
			continue;
		if (!firstServer)
			firstServer = &_servers[i];
		if (!host.empty() && _servers[i].getServerName() == host)
			return &_servers[i];
	}
	return firstServer;
}

void Server::handleCgiTimeouts()
{
	time_t now = std::time(NULL);

	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		Client& client = it->second;
		CgiProcess* cgi = client.getCgiProcess();
		if (!cgi)
			continue;
		if (now - cgi->startTime > cgi->executionTimeout)
		{
			killCgiProcess(client);

			// Build 504 Gateway Timeout response
			const ServerBlock* server = getServerBlock(client.getRequest(), it->first);
			client.buildErrorResponse(504, server);

			// Log the timeout event so it appears in server logs
			if (server)
				client.logRequestEnd();

			client.setState(STATE_KEEPALIVE);

			// Enable POLLOUT to send the error response
			setPollEvents(it->first, POLLOUT);
		}
	}
}


void Server::finalizeCgi(Client& client, CgiProcess* cgi, int clientFd)
{
	// Close and unregister all CGI pipes
	if (cgi->pipeOut != -1) closePollFd(cgi->pipeOut);
	if (cgi->pipeIn != -1) closePollFd(cgi->pipeIn);
	if (cgi->pipeErr != -1) closePollFd(cgi->pipeErr);

	// Reap CGI process (kill if still running)
	int status = 0;
	int waitRet = waitpid(cgi->pid, &status, WNOHANG);
	if (waitRet == 0)
	{
		// CGI closed its stdout but is still running (e.g., infinite sleep)
		// Kill it immediately to prevent zombie processes
		kill(cgi->pid, SIGKILL);
		// Blocking waitpid, instant since we just killed it
		waitpid(cgi->pid, &status, 0);
	}

	// Build response based on CGI result
	bool hasContentType = (cgi->output.find("Content-Type:") != std::string::npos)
		|| (cgi->output.find("Content-type:") != std::string::npos);
	bool cgiError = (WIFEXITED(status) && WEXITSTATUS(status))
		|| WIFSIGNALED(status) || cgi->output.empty() || !hasContentType;

	if (cgiError)
	{
		const ServerBlock* server = getServerBlock(client.getRequest(), clientFd);
		client.buildErrorResponse(500, server);
		client.logRequestEnd();
		if (!cgi->errorOutput.empty())
			Logger::logMessage(RED "[CGI] Error:\n" RESET + cgi->errorOutput);
	}
	else
	{
		CgiResult result;
		result.output = cgi->output;
		Cgi cgiParser;
		cgiParser.parseHeaders(cgi->output, result);
		client.buildResponseFromCGI(result);
	}

	// Clean up CGI process
	delete cgi;
	client.setCgiProcess(NULL);
	client.setState(STATE_KEEPALIVE);

	// Enable POLLOUT on client socket to send the response
	setPollEvents(clientFd, POLLOUT);
}

void Server::handleSessionTimeouts()
{
	// Check cleanup interval
	if (std::time(NULL) - _lastSessionCleanup <= SESSION_CLEANUP_INTERVAL)
		return;
	_lastSessionCleanup = std::time(NULL);

	// Remove expired sessions
	std::map<std::string, SessionData>::iterator it = _sessions.begin();
	while (it != _sessions.end())
	{
		if (std::time(NULL) - it->second.lastActive > SESSION_TIMEOUT)
		{
			std::map<std::string, SessionData>::iterator toErase = it;
			it++;
			_sessions.erase(toErase);
		}
		else
			it++;
	}
}

void Server::handleSocketError(size_t i)
{
	int fd = _pollFds[i].fd;
	socketTypeMap::iterator typeIt = _socketTypes.find(fd);
	if (typeIt == _socketTypes.end())
	{
		closePollFd(fd);
		return;
	}
	SocketType type = typeIt->second;

	// Client socket: log and remove
	if (type == SOCKET_CLIENT)
	{
		std::map<int, Client>::iterator it = _clients.find(fd);
		if (it != _clients.end())
		{
			it->second.logRequestEnd();
			removeClient(it);
		}
		return;
	}

	// CGI pipe: find owning client and handle the broken pipe
	if (type == SOCKET_CGI)
	{
		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			Client& client = it->second;
			CgiProcess* cgi = client.getCgiProcess();
			if (!cgi)
				continue;

			// Broken stderr pipe
			if (cgi->pipeErr == fd)
			{
				closePollFd(cgi->pipeErr);
				return;
			}

			// Broken stdout pipe: finalize CGI (build response, cleanup)
			if (cgi->pipeOut == fd)
			{
				finalizeCgi(client, cgi, it->first);
				return;
			}

			// Broken stdin pipe
			if (cgi->pipeIn == fd)
			{
				cgi->inputWritten = true;
				closePollFd(cgi->pipeIn);
				return;
			}
		}
	}

	// Fallback: unhandled socket type or orphan CGI pipe
	// Remove and close to prevent busy-loop
	{
		std::stringstream ss;
		ss << RED "[Server] Error: " RESET "handleSocketError: Unhandled socket error on fd " << fd
			<< " (type=" << type << "), removing to prevent busy-loop";
		Logger::logMessage(ss.str());
	}
	closePollFd(fd);
}

// Drain pipe so it doesn't remain readable and stop server
void Server::handleSignalPipeReadable()
{
	char buf[SIGNAL_PIPE_BUFFER_SIZE];
	ssize_t bytes = read(_s_sigpipe[0], buf, sizeof(buf));
	(void)bytes;
	_running = false;
}

void Server::acceptNewClient(int listenSocket)
{
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);

	// Accept a new incoming connection (non-blocking)
	int clientFd = accept(listenSocket, (struct sockaddr*)&clientAddr, &addrLen);
	if (clientFd < 0)
	{
		Logger::logMessage(RED "[Server] Error: " RESET "acceptNewClient: accept failed on fd " + intToString(listenSocket));
		return; // No connection available right now
	}

	// Get client IP
	unsigned char* ip = reinterpret_cast<unsigned char*>(&clientAddr.sin_addr);
	std::stringstream ipStream;
	ipStream << static_cast<int>(ip[0]) << "." << static_cast<int>(ip[1]) << "."
			 << static_cast<int>(ip[2]) << "." << static_cast<int>(ip[3]);
	std::string clientIp = ipStream.str();

	// Set the client socket to non-blocking mode
	try
	{
		setNonBlocking(clientFd);
	}
	catch (const std::exception& e)
	{
		Logger::logMessage(RED "[Server] Error: " RESET "acceptNewClient: " + std::string(e.what()) + " for fd " + intToString(clientFd) + RESET);
		safeClose(clientFd, "Server");
		return;
	}

	// Add a Client to the map
	Client newClient(clientFd, std::string(clientIp));
	_clients.insert(std::make_pair(clientFd, newClient));

	// Add the new client socket to poll() to monitor incoming data
	pollfd pfd = { clientFd, POLLIN, 0};
	_pollFds.push_back(pfd);
	_socketTypes[clientFd] = SOCKET_CLIENT;
}

// Checks if the request body exceeds the configured size limit (location overrides server default).
// Builds a 413 error response and enables POLLOUT if the limit is exceeded.
// Returns true if the limit was exceeded (caller must return immediately).
bool Server::checkBodySize(Client& client, const ServerBlock* server, size_t clientIndex)
{
	// Default to server-level limit
	size_t maxBodySize = server->getMaxBodySize();

	// Prefer location-specific limit when defined
	HttpRequest& requestRef = const_cast<HttpRequest&>(client.getRequest());
	RouteMatch match = _router.matchRoute(*server, requestRef);
	if (match.location && match.location->getMaxBodySize() > 0)
		maxBodySize = match.location->getMaxBodySize();

	if (client.getRequest().getBody().size() > maxBodySize
		|| (!client.getRequest().isChunked() && client.getRequest().getContentLength() > maxBodySize))
	{
		client.buildErrorResponse(413, server);
		client.markCloseAfterResponse();
		client.logRequestEnd();
		_pollFds[clientIndex].events = POLLOUT;
		return true;
	}
	return false;
}

// Registers CGI process pipes (stdout, stderr, stdin) in the poll set.
// Disables POLLIN on the client socket while CGI is running.
void Server::registerCgiPipes(CgiProcess* cgi, size_t clientIndex)
{
	// Suspend client socket monitoring while CGI handles the request
	_pollFds[clientIndex].events = 0;

	// Monitor CGI stdout for response output
	pollfd pfd = { cgi->pipeOut, POLLIN, 0 };
	_pollFds.push_back(pfd);
	_socketTypes[cgi->pipeOut] = SOCKET_CGI;

	// Monitor CGI stderr for error output (if available)
	if (cgi->pipeErr != -1)
	{
		pollfd pfdErr = { cgi->pipeErr, POLLIN, 0 };
		_pollFds.push_back(pfdErr);
		_socketTypes[cgi->pipeErr] = SOCKET_CGI;
	}

	// Monitor CGI stdin for writing POST body (if available)
	if (cgi->pipeIn != -1)
	{
		pollfd pfdIn = { cgi->pipeIn, POLLOUT, 0 };
		_pollFds.push_back(pfdIn);
		_socketTypes[cgi->pipeIn] = SOCKET_CGI;
	}
}

// Spawns a CGI process asynchronously, sets its execution timeout,
// and registers its pipes in the poll set for I/O multiplexing.
void Server::startCgi(Client& client, const RouteMatch& match, size_t clientIndex, const ServerBlock* server)
{
	// Store server info needed for CGI response logging
	client.setCgiTiming(*server);

	// Collect all open fds so the child process can close them after fork
	std::vector<int> fdsToClose;
	for (size_t i = 0; i < _pollFds.size(); i++)
		fdsToClose.push_back(_pollFds[i].fd);

	HttpRequest& requestRef = const_cast<HttpRequest&>(client.getRequest());
	Cgi cgi;
	CgiProcess* cgiProc = cgi.startAsync(match, requestRef, fdsToClose);
	if (!cgiProc)
	{
		client.buildErrorResponse(500, server);
		client.setState(STATE_KEEPALIVE);
		_pollFds[clientIndex].events |= POLLOUT;
		return;
	}
	cgiProc->executionTimeout = server->getCgiTimeout();

	client.setCgiProcess(cgiProc);

	// Register CGI pipes in poll set and suspend client socket monitoring
	registerCgiPipes(cgiProc, clientIndex);
}

// Routes the completed request to either a CGI process or a regular response builder.
// Returns true if a CGI was started (caller must not touch _pollFds[clientIndex] afterwards).
bool Server::buildClientResponse(Client& client, const ServerBlock* server, size_t clientIndex)
{
	client.setState(STATE_PROCESSING);
	client.updateActivity();

	if (!server)
	{
		client.buildErrorResponse(500, NULL);
		client.setState(STATE_KEEPALIVE);
		return false;
	}

	HttpRequest& requestRef = const_cast<HttpRequest&>(client.getRequest());
	RouteMatch match = _router.matchRoute(*server, requestRef);

	if (match.statusCode == 200 && match.isCGI)
	{
		// Start CGI process asynchronously and register its pipes
		startCgi(client, match, clientIndex, server);
		return true;
	}

	// Regular non-CGI request
	client.buildResponse(*server, _router, _sessions);
	client.setState(STATE_KEEPALIVE);
	return false;
}

void Server::handleClientRead(size_t clientIndex)
{
	int clientFd = _pollFds[clientIndex].fd;

	// Find the client with this fd
	std::map<int, Client>::iterator it = _clients.find(clientFd);
	if (it == _clients.end())
	{
		Logger::logMessage(RED "[Server] Error: " RESET "handleClientRead: client not found for fd " + intToString(clientFd));
		return;
	}
	Client& client = it->second;

	// Get config early for logging
	const ServerBlock* server = getServerBlock(client.getRequest(), clientFd);

	// Read data from socket
	if (!client.readData(server))
	{
		client.logRequestEnd();
		removeClient(it);
		return;
	}

	// Log request start after headers are parsed so Host-based vhost is accurate
	server = getServerBlock(client.getRequest(), clientFd);

	if (!client.isRequestLogged() && (client.getRequest().headersParsed() || client.isRequestComplete()) && server)
		client.logRequestStart(server->getServerName(), server->getPort());

	// If an early error response is already prepared (e.g., size limit), switch to write-only
	if (client.isResponseReady())
	{
		_pollFds[clientIndex].events = POLLOUT;
		return;
	}

	// Reject oversized request bodies early before full parsing
	if (server && checkBodySize(client, server, clientIndex))
		return;

	// Check if request is complete
	if (!client.isRequestComplete())
		return;

	// Build and dispatch the response (CGI or static)
	if (!client.isResponseReady())
	{
		bool cgiStarted = buildClientResponse(client, server, clientIndex);
		if (cgiStarted)
			return;
	}

	_pollFds[clientIndex].events = client.shouldCloseAfterResponse()
		? POLLOUT : (_pollFds[clientIndex].events | POLLOUT);
}

void Server::handleClientWrite(size_t clientIndex)
{
	int clientFd = _pollFds[clientIndex].fd;

	// Find the client with this fd
	std::map<int, Client>::iterator it = _clients.find(clientFd);
	if (it == _clients.end())
	{
		Logger::logMessage(RED "[Server] Error: " RESET "handleClientWrite: client not found for fd " + intToString(clientFd));
		return;
	}
	Client& client = it->second;

	// Update activity timestamp during progressive sending
	client.updateActivity();

	// Send response (may be progressive for large responses)
	bool sendComplete = client.sendResponse();

	if (!sendComplete)
	{
		// Send error: clean up the client immediately
		if (client.shouldCloseAfterResponse())
		{
			client.logRequestEnd();
			removeClient(it);
		}
		// Otherwise: large response, retry on next POLLOUT
		return;
	}

	// Response fully sent
	client.logRequestEnd();

	if (client.shouldCloseAfterResponse())
	{
		removeClient(it);
		return;
	}

	// Keep-alive: prepare the next request on the same connection
	client.resetForNextRequest();
	_pollFds[clientIndex].events = POLLIN;

	// If the next request is already complete (pipeline), chain it
	if (client.isRequestComplete())
	{
		const ServerBlock* server = getServerBlock(client.getRequest(), clientFd);
		if (!server)
		{
			client.buildErrorResponse(500, NULL);
			client.markCloseAfterResponse();
			_pollFds[clientIndex].events = POLLIN | POLLOUT;
			return;
		}

		// Log start for pipelined leftover (headers already parsed, not logged via readData)
		client.logRequestStart(server->getServerName(), server->getPort());
		bool cgiStarted = buildClientResponse(client, server, clientIndex);
		if (!cgiStarted)
			_pollFds[clientIndex].events = POLLIN | POLLOUT;
	}
}

void Server::handleCgiPipe(size_t pipeIndex)
{
	int pipeFd = _pollFds[pipeIndex].fd;

	// Find the client that owns this CGI pipe
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		Client& client = it->second;
		CgiProcess* cgi = client.getCgiProcess();
		if (!cgi)
			continue;

		// Handle pipeErr (reading CGI stderr)
		if (cgi->pipeErr == pipeFd && (_pollFds[pipeIndex].revents & POLLIN))
		{
			char buffer[READ_BUFFER_SIZE];
			ssize_t bytes = read(pipeFd, buffer, sizeof(buffer));

			if (bytes > 0)
				cgi->errorOutput.append(buffer, bytes);
			else if (bytes == 0)
			{
				// EOF or error on stderr
				closePollFd(cgi->pipeErr);
			}
			return;
		}

		// Handle pipeOut (reading CGI stdout)
		if (cgi->pipeOut == pipeFd && (_pollFds[pipeIndex].revents & POLLIN))
		{
			char buffer[READ_BUFFER_SIZE];
			ssize_t bytes = read(pipeFd, buffer, sizeof(buffer));

			if (bytes > 0)
				cgi->output.append(buffer, bytes);
			else if (bytes == 0)
				finalizeCgi(client, cgi, it->first);
			return;
		}

		// Handle pipeIn (writing POST body to CGI stdin)
		if (cgi->pipeIn == pipeFd)
		{
			if ((_pollFds[pipeIndex].revents & POLLOUT) && !cgi->inputWritten)
			{
				const std::string& body = client.getRequest().getBody();
				size_t remaining = body.size() - cgi->bytesWritten;

				if (remaining > 0)
				{
					ssize_t written = write(pipeFd, body.c_str() + cgi->bytesWritten, remaining);
					if (written > 0)
					{
						cgi->bytesWritten += written;

						if (cgi->bytesWritten >= body.size())
						{
							cgi->inputWritten = true;
							closePollFd(cgi->pipeIn);
						}
					}
					else if (written < 0)
					{
						Logger::logMessage(RED "[CGI] Error: " RESET "handleCGIPipe: write to stdin pipe failed on fd " + intToString(pipeFd));
						cgi->inputWritten = true;
						closePollFd(cgi->pipeIn);
					}
				}
			}
			return;
		}
	}
}

// Public method(s) ------------------------------------------------------------

void Server::run()
{
	_running = true;
	std::cout << "Press Ctrl+C to stop" << std::endl;

	while (_running)
	{
		// Check for idle client timeouts
		handleClientTimeouts();
		handleCgiTimeouts();
		handleSessionTimeouts();

		// Poll for events on all sockets
		if (poll(&_pollFds[0], _pollFds.size(), POLL_TIMEOUT_MS) < 0)
			continue;

		// Process events on each socket
		for (size_t i = 0; i < _pollFds.size(); i++)
		{			
			int fd = _pollFds[i].fd;
			int revents = _pollFds[i].revents;
			socketTypeMap::iterator typeIt = _socketTypes.find(fd);

			// Skip non-relevant fds
			if (fd == -1 || !revents || typeIt == _socketTypes.end())
				continue;
			
			SocketType type = typeIt->second;

			// Handle unexpected disconnection / socket errors early
			// POLLERR/POLLHUP/POLLNVAL without POLLIN could cause a busy-loop
			if ((revents & (POLLERR | POLLHUP | POLLNVAL)) && !(revents & POLLIN))
			{
				handleSocketError(i);
				continue;
			}

			// Handle POLLIN (incoming data to read)
			if (revents & POLLIN)
			{
				if (type == SOCKET_SIGNAL)
				{
					handleSignalPipeReadable();
					break;
				}
				else if (type == SOCKET_LISTEN)
					acceptNewClient(fd);
				else if (type == SOCKET_CLIENT)
					handleClientRead(i);
			}

			// handleClientRead may have soft-deleted this fd
			if (_pollFds[i].fd == -1)
				continue;

			// Handle POLLOUT (socket ready to write)
			if (revents & POLLOUT && type == SOCKET_CLIENT)
				handleClientWrite(i);

			// Handle CGI pipes
			if (type == SOCKET_CGI)
				handleCgiPipe(i);
		}

		// --- GARBAGE COLLECTOR ---
		// Clean up all soft-deleted entries in one pass
		for (PollfdVector::iterator it = _pollFds.begin(); it != _pollFds.end();)
		{
			if (it->fd == -1)
				it = _pollFds.erase(it);
			else
				++it;
		}
	}
}
