/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:49 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/05 11:57:30 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------
#include "Server.hpp"
#include "../cgi/CGI.hpp"
#include "../utils/utils.hpp"
#include "../utils/Logger.hpp"
#include <algorithm>			// std::find
#include <cerrno>				// errno
#include <csignal>				// signal, SIGINT, SIGTERM, SIGPIPE, SIG_IGN, kill
#include <cstring>				// std::memset, std::strerror
#include <iostream>				// std::cout, std::cerr
#include <netinet/in.h>			// sockaddr_in, htons, INADDR_ANY
#include <limits>				// std::numeric_limits
#include <sstream>				// std::stringstream
#include <stdexcept>			// std::runtime_error
#include <unistd.h>				// read, write, close, pipe
#include <sys/wait.h>			// waitpid, WNOHANG, WIFEXITED, WEXITSTATUS, WIFSIGNALED

// Define(s) -------------------------------------------------------------------
# define CURSOR_HIDE "\033[?25l"
# define CURSOR_SHOW "\033[?25h"

// Static variable initialization ----------------------------------------------
// Self-pipe for signal handling in poll()
int Server::_s_sigpipe[2] = {-1, -1};

// Special member function(s) --------------------------------------------------
Server::Server(const Config &config)
	: _configs(config.getServers()), _running(false), _lastSessionCleanup(0)
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
		CGIProcess *cgi = it->second.getCGIProcess();
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

// Public method(s) ------------------------------------------------------------
void Server::run()
{
	_running = true;
	std::cout << "Press Ctrl+C to stop" << std::endl;

	while (_running)
	{
		// Check for idle client timeouts
		handleClientTimeouts();
		handleCGITimeouts();
		handleSessionTimeouts();

		// Poll for events on all sockets (1 second timeout)
		if (poll(&_pollFds[0], _pollFds.size(), 1000) < 0)
			continue;

		// Process events on each socket
		for (size_t i = 0; i < _pollFds.size(); i++)
		{
			// Skip soft-deleted entries
			if (_pollFds[i].fd == -1)
				continue;

			int revents = _pollFds[i].revents;
			if (!revents)
				continue;

			int fd = _pollFds[i].fd;
			std::map<int, SocketType>::iterator typeIt = _socketTypes.find(fd);
			if (typeIt == _socketTypes.end())
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

			// handleClientWrite may have soft-deleted this fd
			if (_pollFds[i].fd == -1)
				continue;

			// Handle CGI pipes
			if (type == SOCKET_CGI)
				handleCGIPipe(i);
		}

		// --- GARBAGE COLLECTOR ---
		// Clean up all soft-deleted entries in one pass
		for (std::vector<pollfd>::iterator it = _pollFds.begin(); it != _pollFds.end(); )
		{
			if (it->fd == -1)
				it = _pollFds.erase(it);
			else
				++it;
		}
	}
}

void Server::stop()
{
	_running = false;
}

// Private method(s) -----------------------------------------------------------
void Server::setupListenSockets()
{
	// Create one listening socket per configuration (one per port)
	std::vector<int> port;
	for (size_t i = 0; i < _configs.size(); i++)
	{
		if (std::find(port.begin(), port.end(), _configs[i].getPort()) != port.end())
		{
			std::cout << "Server " << _configs[i].getServerName() << " listening on port " << _configs[i].getPort() << std::endl;
			continue; // Skip duplicate ports
		}
		port.push_back(_configs[i].getPort());

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
        struct sockaddr_in addr;
        std::memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;                        // IPv4 address family
        addr.sin_addr.s_addr = INADDR_ANY;                // Accept connections on all local network interfaces (0.0.0.0)
        addr.sin_port = htons(_configs[i].getPort());     // Port from config, converted to network byte order (big-endian)

        // Bind the socket fd to the address structure above
        // After this call, listenFd is associated with port _configs[i].getPort() on all interfaces
        if (bind(listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
        {
            safeClose(listenFd, "Server");
            throw std::runtime_error("bind() failed");
        }

		// Start listening for incoming connections
		if (listen(listenFd, 128) < 0)
		{
			safeClose(listenFd, "Server");
			throw std::runtime_error("listen() failed");
		}

		// Add to poll to monitor incoming connections
		pollfd pfd;
		pfd.fd = listenFd;
		pfd.events = POLLIN; // Watch for incoming data
		pfd.revents = 0;
		_pollFds.push_back(pfd);
		_socketTypes[listenFd] = SOCKET_LISTEN;
		std::cout << "Server " << _configs[i].getServerName() << " listening on port " << _configs[i].getPort() << std::endl;
	}
}

void Server::acceptNewClient(int listenSocket)
{
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);

	// Accept a new incoming connection (non-blocking)
	int clientFd = accept(listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
	if (clientFd < 0)
	{
		Logger::logMessage(RED "[Server] Error: " RESET "acceptNewClient: accept failed on fd " + intToString(listenSocket));
		return; // No connection available right now
	}

	// Get client IP
	unsigned char *ip = reinterpret_cast<unsigned char *>(&clientAddr.sin_addr);
	std::stringstream ipStream;
	ipStream << static_cast<int>(ip[0]) << "." << static_cast<int>(ip[1]) << "."
			 << static_cast<int>(ip[2]) << "." << static_cast<int>(ip[3]);
	std::string clientIp = ipStream.str();

	// Set the client socket to non-blocking mode
	try
	{
		setNonBlocking(clientFd);
	}
	catch (const std::exception &e)
	{
		Logger::logMessage(RED "[Server] Error: " RESET "acceptNewClient: " + std::string(e.what()) + " for fd " + intToString(clientFd) + RESET);
		safeClose(clientFd, "Server");
		return;
	}
	// Add a Client to the map
	Client newClient(clientFd, std::string(clientIp));
	_clients.insert(std::make_pair(clientFd, newClient));

	// Add the new client socket to poll() to monitor incoming data
	pollfd pfd;
	pfd.fd = clientFd;
	pfd.events = POLLIN; // Triggered when client sends data
	pfd.revents = 0;
	_pollFds.push_back(pfd);
	_socketTypes[clientFd] = SOCKET_CLIENT;
}

void Server::handleClientTimeouts()
{
	std::map<int, Client>::iterator it = _clients.begin();
	while (it != _clients.end())
	{
		if (it->second.hasTimedOut(CLIENT_KEEPALIVE_TIMEOUT, CLIENT_PROCESSING_TIMEOUT))
		{
			int fd = it->first;

			Client &client = it->second;
			Server::logClientResponse(client);

			++it;
			removeClient(fd);
		}
		else
			++it;
	}
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
	Client &client = it->second;

	// Get config early for logging
	const ServerConfig *config = selectConfig(client.getRequest(), clientFd);

	// Read data from socket
	if (!client.readData(config))
	{
		Server::logClientResponse(client);
		removeClient(clientFd);
		return;
	}

	// Log request start after headers are parsed so Host-based vhost is accurate
	const ServerConfig *effectiveCfg = selectConfig(client.getRequest(), clientFd);
	if (!client.isRequestLogged() && (client.getRequest().headersParsed() || client.isRequestComplete()) && effectiveCfg)
	{
		std::string method = client.getRequest().getMethod();
		std::string uri = client.getRequest().getUri();
		if (!client.getRequest().headersParsed())
		{
			if (method.empty())
				method = "UNKNOWN";
			if (uri.empty())
				uri = "/";
		}
		size_t declSize = std::numeric_limits<size_t>::max();
		if ((method == "POST" || method == "PUT" || method == "PATCH") && !client.getRequest().isChunked())
			declSize = client.getRequest().getContentLength();
		Logger::logRequestStart(client.getSocket(), method, uri, client.getClientIp(),
						effectiveCfg->getServerName(), effectiveCfg->getPort(), declSize);
		client.markRequestLogged();
	}

	// If an early error response is already prepared (e.g., size limit), switch to write-only
	if (client.isResponseReady())
	{
		_pollFds[clientIndex].events = POLLOUT;
		return;
	}

	// Early size guard: if body already exceeds configured limit (location override
	// server default if defined), send 413 and close.
	const ServerConfig *earlyCfg = effectiveCfg;
	if (!client.isResponseReady() && earlyCfg)
	{
		// Default to server-level limit
		size_t maxBodySize = earlyCfg->getMaxBodySize();

		// If we can resolve a location, prefer its specific limit when set
		HttpRequest &earlyRequest = const_cast<HttpRequest &>(client.getRequest());
		RouteMatch earlyMatch = _router.matchRoute(*earlyCfg, earlyRequest);
		if (earlyMatch.location && earlyMatch.location->getMaxBodySize() > 0)
			maxBodySize = earlyMatch.location->getMaxBodySize();

		if (client.getRequest().getBody().size() > maxBodySize
			|| (!client.getRequest().isChunked() && client.getRequest().getContentLength() > maxBodySize))
		{
			client.buildErrorResponse(413, earlyCfg);
			client.markCloseAfterResponse();
			// Log the oversized request early rejection
			if (earlyCfg)
				Server::logClientResponse(client);
			_pollFds[clientIndex].events = POLLOUT;
			return;
		}
	}

	// Check if request is complete
	if (!client.isRequestComplete())
		return;

	// Keep any additional requests already present in the buffer
	client.stashLeftoverFromRequest();

	// Build response
	if (!client.isResponseReady())
	{
		client.setState(STATE_PROCESSING);
		client.updateActivity();

		const ServerConfig *config = selectConfig(client.getRequest(), clientFd);
		if (!config)
		{
			client.buildErrorResponse(500, NULL);
			client.setState(STATE_KEEPALIVE);
			_pollFds[clientIndex].events = client.shouldCloseAfterResponse() ? POLLOUT : (_pollFds[clientIndex].events | POLLOUT);
			return;
		}

		// Match route to determine effective body size limit (location overrides server)
		HttpRequest &request = const_cast<HttpRequest &>(client.getRequest());
		RouteMatch match = _router.matchRoute(*config, request);
		size_t maxBodySize = config->getMaxBodySize();
		if (match.location && match.location->getMaxBodySize() > 0)
			maxBodySize = match.location->getMaxBodySize();

		// Enforce configured body size limit before routing/CGI
		if (request.getBody().size() > maxBodySize)
		{
			client.buildErrorResponse(413, config);
			client.markCloseAfterResponse();
			client.setState(STATE_KEEPALIVE);
			_pollFds[clientIndex].events = POLLOUT;
			return;
		}

		// Check if this is a CGI request

		if (match.statusCode == 200 && match.isCGI)
		{
			// Start CGI asynchronously
			const ServerConfig *config = selectConfig(client.getRequest(), clientFd);
			size_t cgiExecutionTimeout = config ? config->getCgiTimeout() : static_cast<size_t>(DEFAULT_CGI_EXECUTION_TIMEOUT);

			// Store timing info for CGI logging
			if (config)
				client.setCGITiming(*config);

			std::vector<int> fdsToClose;
			for (size_t i = 0; i < _pollFds.size(); i++)
				fdsToClose.push_back(_pollFds[i].fd);

			CGI cgi;
			CGIProcess *cgiProc = cgi.startAsync(match, request, fdsToClose);
			if (cgiProc)
				cgiProc->executionTimeout = cgiExecutionTimeout;
			else
			{
				client.buildErrorResponse(500, config);
				client.setState(STATE_KEEPALIVE);
				_pollFds[clientIndex].events = client.shouldCloseAfterResponse() ? POLLOUT : (_pollFds[clientIndex].events | POLLOUT);
				return;
			}

			client.setCGIProcess(cgiProc);

			// Disable POLLIN on client socket while CGI is running
			_pollFds[clientIndex].events = 0;
			// Add CGI pipe to poll
			pollfd pfd;
			pfd.fd = cgiProc->pipeOut;
			pfd.events = POLLIN;
			pfd.revents = 0;
			_pollFds.push_back(pfd);
			_socketTypes[cgiProc->pipeOut] = SOCKET_CGI;
			// Add CGI stderr pipe to poll
			if (cgiProc->pipeErr != -1)
			{
				pollfd pfdErr;
				pfdErr.fd = cgiProc->pipeErr;
				pfdErr.events = POLLIN;
				pfdErr.revents = 0;
				_pollFds.push_back(pfdErr);
				_socketTypes[cgiProc->pipeErr] = SOCKET_CGI;
			}
			// If POST with body, also monitor pipeIn for writing
			if (cgiProc->pipeIn != -1)
			{
				pollfd pfdIn;
				pfdIn.fd = cgiProc->pipeIn;
				pfdIn.events = POLLOUT;
				pfdIn.revents = 0;
				_pollFds.push_back(pfdIn);
				_socketTypes[cgiProc->pipeIn] = SOCKET_CGI;
			}
		}
		else
		{
			// Regular non-CGI request
			client.buildResponse(*config, _router, _sessions);
			client.setState(STATE_KEEPALIVE);
			_pollFds[clientIndex].events = client.shouldCloseAfterResponse() ? POLLOUT : (_pollFds[clientIndex].events | POLLOUT);
		}
	}
	else
	{
		_pollFds[clientIndex].events = client.shouldCloseAfterResponse() ? POLLOUT : (_pollFds[clientIndex].events | POLLOUT);
	}
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
	Client &client = it->second;

	// Update activity timestamp during progressive sending
	client.updateActivity();

	// Send response (may be progressive for large responses)
	bool sendComplete = client.sendResponse();

	if (!sendComplete)
	{
		// Send error: clean up the client immediately
		if (client.shouldCloseAfterResponse())
		{
			Server::logClientResponse(client);
			removeClient(clientFd);
		}
		// Otherwise: large response, retry on next POLLOUT
		return;
	}

	// Response fully sent
	Server::logClientResponse(client);

	if (client.shouldCloseAfterResponse())
	{
		removeClient(clientFd);
		return;
	}

	// Keep-alive: prepare the next request on the same connection
	client.resetForNextRequest();
	_pollFds[clientIndex].events = POLLIN;

	// If the next request is already complete (pipeline), chain it
	if (client.isRequestComplete())
	{
		const ServerConfig *cfg = selectConfig(client.getRequest(), clientFd);
		if (!cfg)
		{
			client.buildErrorResponse(500, NULL);
			client.markCloseAfterResponse();
			_pollFds[clientIndex].events = POLLIN | POLLOUT;
			return;
		}
		// Log start for pipelined leftover (headers already parsed, not logged via readData)
		size_t declSize = std::numeric_limits<size_t>::max();
		{
			const std::string &m = client.getRequest().getMethod();
			if ((m == "POST" || m == "PUT" || m == "PATCH") && !client.getRequest().isChunked())
				declSize = client.getRequest().getContentLength();
		}
		Logger::logRequestStart(client.getSocket(), client.getRequest().getMethod(), client.getRequest().getUri(),
							client.getClientIp(), cfg->getServerName(), cfg->getPort(), declSize);
		client.markRequestLogged();
		client.buildResponse(*cfg, _router, _sessions);
		client.stashLeftoverFromRequest();
		_pollFds[clientIndex].events = POLLIN | POLLOUT;
	}
}

void Server::removePollFd(int fd)
{
	for (size_t i = 0; i < _pollFds.size(); ++i)
	{
		if (_pollFds[i].fd == fd)
		{
			_pollFds[i].fd = -1;
			_socketTypes.erase(fd);
			return;
		}
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

void Server::removeClient(int fd)
{
	safeClose(fd, "Server");
	_clients.erase(fd);
	removePollFd(fd);
}

void Server::handleSocketError(size_t i)
{
	int fd = _pollFds[i].fd;
	std::map<int, SocketType>::iterator typeIt = _socketTypes.find(fd);
	if (typeIt == _socketTypes.end())
	{
		_pollFds[i].fd = -1;
		safeClose(fd, "Server");
		return;
	}
	SocketType type = typeIt->second;

	// Client socket: log and remove
	if (type == SOCKET_CLIENT)
	{
		std::map<int, Client>::iterator it = _clients.find(fd);
		if (it != _clients.end())
			Server::logClientResponse(it->second);
		removeClient(fd);
		return;
	}

	// CGI pipe: find owning client and handle the broken pipe
	if (type == SOCKET_CGI)
	{
		for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
		{
			Client &client = it->second;
			CGIProcess *cgi = client.getCGIProcess();
			if (!cgi)
				continue;

			// Broken stderr pipe
			if (cgi->pipeErr == fd)
			{
				_pollFds[i].fd = -1;
				_socketTypes.erase(fd);
				safeClose(cgi->pipeErr, "Server");
				cgi->pipeErr = -1;
				return;
			}

			// Broken stdout pipe: finalize CGI (build response, cleanup)
			if (cgi->pipeOut == fd)
			{
				finalizeCGI(client, cgi, it->first);
				return;
			}

			// Broken stdin pipe
			if (cgi->pipeIn == fd)
			{
				cgi->inputWritten = true;
				_pollFds[i].fd = -1;
				_socketTypes.erase(fd);
				safeClose(cgi->pipeIn, "Server");
				cgi->pipeIn = -1;
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
	_pollFds[i].fd = -1;
	_socketTypes.erase(fd);
	safeClose(fd, "Server");
}

void Server::finalizeCGI(Client &client, CGIProcess *cgi, int clientFd)
{
	// Remove all CGI pipes from poll
	if (cgi->pipeOut != -1)
		removePollFd(cgi->pipeOut);
	if (cgi->pipeIn != -1)
		removePollFd(cgi->pipeIn);
	if (cgi->pipeErr != -1)
		removePollFd(cgi->pipeErr);

	// Close all pipes
	if (cgi->pipeOut != -1)
	{
		safeClose(cgi->pipeOut, "Server");
		cgi->pipeOut = -1;
	}
	if (cgi->pipeIn != -1)
	{
		safeClose(cgi->pipeIn, "Server");
		cgi->pipeIn = -1;
	}
	if (cgi->pipeErr != -1)
	{
		safeClose(cgi->pipeErr, "Server");
		cgi->pipeErr = -1;
	}

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
		const ServerConfig *cfg = selectConfig(client.getRequest(), clientFd);
		client.buildErrorResponse(500, cfg);
	}
	else
	{
		CGIResult result;
		result.output = cgi->output;
		CGI cgiParser;
		cgiParser.parseHeaders(cgi->output, result);
		client.buildResponseFromCGI(result);
	}

	// Log CGI errors
	if (cgiError)
	{
		Server::logClientResponse(client);
		if (!cgi->errorOutput.empty())
			Logger::logMessage(RED "[CGI] Error:\n" RESET + cgi->errorOutput);
	}

	// Clean up CGI process
	delete cgi;
	client.setCGIProcess(NULL);
	client.setState(STATE_KEEPALIVE);

	// Enable POLLOUT on client socket to send the response
	setPollEvents(clientFd, POLLOUT);
}

const ServerConfig *Server::selectConfig(const HttpRequest &request, int clientFd) const
{
	std::string host = request.getHeader("Host");
	int localPort = getSocketPort(clientFd);

	// Remove port from Host header if present
	size_t colonPos = host.find(':');
	if (colonPos != std::string::npos)
		host = host.substr(0, colonPos);

	// Virtual host lookup: match server_name against Host header
	const ServerConfig *firstConfig = NULL;
	for (size_t i = 0; i < _configs.size(); i++)
	{
		if (_configs[i].getPort() != localPort)
			continue;
		if (!firstConfig)
			firstConfig = &_configs[i];
		if (!host.empty() && _configs[i].getServerName() == host)
			return &_configs[i];
	}
	return firstConfig;
}

void Server::handleCGITimeouts()
{
	time_t now = std::time(NULL);

	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		Client &client = it->second;
		CGIProcess *cgi = client.getCGIProcess();
		if (!cgi)
			continue;
		if (now - cgi->startTime > cgi->executionTimeout)
		{
			// Kill the CGI process
			kill(cgi->pid, SIGKILL);
			waitpid(cgi->pid, NULL, 0);

			// Close pipes
			if (cgi->pipeOut != -1)
				close(cgi->pipeOut);
			if (cgi->pipeIn != -1)
				close(cgi->pipeIn);
			if (cgi->pipeErr != -1)
				close(cgi->pipeErr);

			// Remove pipes from poll (soft delete)
			for (size_t i = 0; i < _pollFds.size(); i++)
			{
				if (_pollFds[i].fd == cgi->pipeOut || _pollFds[i].fd == cgi->pipeIn || _pollFds[i].fd == cgi->pipeErr)
				{
					_socketTypes.erase(_pollFds[i].fd);
					_pollFds[i].fd = -1;
				}
			}

			// Build 504 Gateway Timeout response
			const ServerConfig *cfg = selectConfig(client.getRequest(), it->first);
			client.buildErrorResponse(504, cfg);

			// Log the timeout event so it appears in server logs
			if (cfg)
				Server::logClientResponse(client);

			// Clean up CGI
			delete cgi;
			client.setCGIProcess(NULL);
			client.setState(STATE_KEEPALIVE);

			// Enable POLLOUT to send the error response
			setPollEvents(it->first, POLLOUT);
		}
	}
}

void Server::handleCGIPipe(size_t pipeIndex)
{
	int pipeFd = _pollFds[pipeIndex].fd;

	// Find the client that owns this CGI pipe
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		Client &client = it->second;
		CGIProcess *cgi = client.getCGIProcess();
		if (!cgi)
			continue;

		// Handle pipeErr (reading CGI stderr)
		if (cgi->pipeErr == pipeFd && (_pollFds[pipeIndex].revents & POLLIN))
		{
			char buffer[4096];
			ssize_t bytes = read(pipeFd, buffer, sizeof(buffer));

			if (bytes > 0)
				cgi->errorOutput.append(buffer, bytes);
			else
			{
				// EOF or error on stderr
				_pollFds[pipeIndex].fd = -1;
				_socketTypes.erase(pipeFd);
				safeClose(cgi->pipeErr, "Server");
				cgi->pipeErr = -1;
			}
			return;
		}

		// Handle pipeOut (reading CGI stdout)
		if (cgi->pipeOut == pipeFd && (_pollFds[pipeIndex].revents & POLLIN))
		{
			char buffer[4096];
			ssize_t bytes = read(pipeFd, buffer, sizeof(buffer));

			if (bytes > 0)
				cgi->output.append(buffer, bytes);
			else
				finalizeCGI(client, cgi, it->first);
			return;
		}

		// Handle pipeIn (writing POST body to CGI stdin)
		if (cgi->pipeIn == pipeFd)
		{
			if ((_pollFds[pipeIndex].revents & POLLOUT) && !cgi->inputWritten)
			{
				const std::string &body = client.getRequest().getBody();
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
							removePollFd(cgi->pipeIn);
							safeClose(cgi->pipeIn, "Server");
							cgi->pipeIn = -1;
						}
					}
					else if (written < 0)
					{
						Logger::logMessage(RED "[CGI] Error: " RESET "handleCGIPipe: write to stdin pipe failed on fd " + intToString(pipeFd));
						cgi->inputWritten = true;
						removePollFd(cgi->pipeIn);
						safeClose(cgi->pipeIn, "Server");
						cgi->pipeIn = -1;
					}
				}
			}
			return;
		}
	}
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

// Drain pipe so it doesn't remain readable and stop server
void Server::handleSignalPipeReadable()
{
	char buf[64];
	ssize_t bytes = read(_s_sigpipe[0], buf, sizeof(buf));
	(void)bytes;
	_running = false;
}

void Server::addSignalPipeToPoll()
{
	pollfd pfd;
	pfd.fd = _s_sigpipe[0];
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pollFds.push_back(pfd);
	_socketTypes[_s_sigpipe[0]] = SOCKET_SIGNAL;
}

// Called by OS when SIGINT/SIGTERM received (async-signal-safe)
void Server::signalHandler(int)
{
	if (_s_sigpipe[1] != -1)
	{
		ssize_t ret = write(_s_sigpipe[1], "1", 1);
		(void)ret;
	}
}

void Server::installSignals()
{
	// Create self-pipe for safe signal handling in poll()
	if (pipe(_s_sigpipe) == -1)
		throw std::runtime_error(std::string("pipe() failed: ") + std::strerror(errno));
	setNonBlocking(_s_sigpipe[0]);
	setNonBlocking(_s_sigpipe[1]);
	addSignalPipeToPoll();

	// Register signal handlers for graceful shutdown
	signal(SIGINT, Server::signalHandler);
	signal(SIGQUIT, Server::signalHandler);
	signal(SIGTERM, Server::signalHandler);

	// Ignore SIGPIPE to prevent termination on broken socket writes
	signal(SIGPIPE, SIG_IGN);
}

void Server::logClientResponse(Client &client)
{
	time_t end = std::time(NULL);
	time_t responseTime = end - client.getRequestStartTime();

	const std::string &method = client.getRequest().getMethod();
	bool hasBody = (method == "POST" || method == "PUT" || method == "PATCH");
	size_t reqSize = hasBody ? client.getRequestBodySize() : std::numeric_limits<size_t>::max();

	Logger::logRequestEnd(client.getSocket(), client.getResponseStatus(), reqSize, client.getResponseBodySize(), responseTime);
}
