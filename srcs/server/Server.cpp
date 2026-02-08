/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:49 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/08 17:10:56 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "Server.hpp"
#include "../cgi/CGI.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../utils/utils.hpp"
#include "../utils/Logger.hpp"
#include <arpa/inet.h> // IP client
#include <cerrno>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <sys/wait.h>

// Self-pipe for signal handling in poll()
int Server::_s_sigpipe[2] = {-1, -1};

// Special member function(s)
Server::Server(const Config &config)
	: _configs(config.getServers()), _running(false), _lastSessionCleanup(0)
{
	installSignals();
	setupListenSockets();
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
		safeClose(_pollFds[i].fd);
	// Close signal pipe explicitly
	if (_s_sigpipe[0] >= 0)
		safeClose(_s_sigpipe[0]);
	if (_s_sigpipe[1] >= 0)
		safeClose(_s_sigpipe[1]);
	Logger::logMessage("Server was closed");
}

// Public method(s)
void Server::run()
{
	_running = true;
	std::cout << "Server running... (Ctrl+C to stop)" << std::endl;

	while (_running)
	{
		// Check for idle client timeouts
		handleClientTimeouts();
		handleCGITimeouts();
		handleSessionTimeouts();

		// Poll for events on all sockets (1 second timeout)
		int ret = poll(&_pollFds[0], _pollFds.size(), 1000);
		if (ret < 0)
			continue;

		// Process events on each socket
		for (size_t i = 0; i < _pollFds.size(); )  // Pas de i++ ici !
		{
			int revents = _pollFds[i].revents;
			if (!revents)
			{
				i++;  // Seulement si pas d'événement
				continue;
			}
			int fd = _pollFds[i].fd;
			SocketType type = _socketTypes[fd];
			
size_t oldSize = _pollFds.size();  // Remember size

			// Handle POLLIN (incoming data to read)
			if (revents & POLLIN)
			{

				// Handle SIGINT or SIGTERM
				if (type == SOCKET_SIGNAL)
				{
					handleSignalPipeReadable();
					break;
				}

				// Handle listen socket
				if (type == SOCKET_LISTEN)
					acceptNewClient(fd);

				// Handle client socket
				else if (type == SOCKET_CLIENT)
					handleClientRead(i);
			}

			// Handle POLLOUT (socket ready to write)
			if (revents & POLLOUT && type == SOCKET_CLIENT)
				handleClientWrite(i);

			if (type == SOCKET_CGI)
				handleCGIPipe(i);
			
// If size changed (element removed), don't increment i
			if (_pollFds.size() < oldSize)
				continue;  // Element removed, i already points to next
			
			i++;  // Otherwise move to next
		}
	}
}

void Server::stop()
{
	_running = false;
}

// Private method(s)
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
		int listenFd = socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP
		if (listenFd < 0)
			throw std::runtime_error("socket() failed");

		// Configure SO_REUSEADDR to allow address reuse and prevent "Address already in use" error
		int opt = 1;
		if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		{
			safeClose(listenFd);
			throw std::runtime_error("setsockopt() failed");
		}

		// Configure the server address structure
		struct sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;		   // IPv4
		addr.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces
		addr.sin_port = htons(port[i]);	   // Convert port to network byte order
		if (bind(listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		{
			safeClose(listenFd);
			throw std::runtime_error("bind() failed");
		}

		// Start listening for incoming connections
		if (listen(listenFd, 128) < 0)
		{
			safeClose(listenFd);
			throw std::runtime_error("listen() failed");
		}

		// Add to poll to monitor incoming connections
		pollfd pfd;
		pfd.fd = listenFd;
		pfd.events = POLLIN; // Watch for incoming data
		pfd.revents = 0;
		_pollFds.push_back(pfd);
		_socketTypes[listenFd] = SOCKET_LISTEN;
		std::cout << "Server " << _configs[i].getServerName() << " listening on port " << port[i] << std::endl;
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
		std::cerr << "[Server] accept failed on fd " << listenSocket << std::endl;
		return; // No connection available right now
	}

	// Get client IP
	char clientIp[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);

	// Set the client socket to non-blocking mode
	try
	{
		setNonBlocking(clientFd);
	}
	catch (const std::exception &e)
	{
		std::cerr << "[Server] " << e.what() << " for fd " << clientFd << std::endl;
		safeClose(clientFd);
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
			++it;
			for (size_t j = 0; j < _pollFds.size(); j++)
			{
				if (_pollFds[j].fd == fd)
				{
					removeClient(fd, j);
					break;
				}
			}
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
		std::cerr << "Error: client not found for fd " << clientFd << std::endl;
		return;
	}
	Client &client = it->second;

	// Read data from socket
	if (!client.readData())
	{
		removeClient(clientFd, clientIndex);
		return;
	}

	// If an early error response is already prepared (e.g., size limit), switch to write-only
	if (client.isResponseReady())
	{
		_pollFds[clientIndex].events = POLLOUT;
		return;
	}

	// Early size guard: if body already exceeds configured limit (location override
	// server default if defined), send 413 and close.
	const ServerConfig *earlyCfg = selectConfig(client.getRequest(), clientFd);
	if (!client.isResponseReady() && earlyCfg)
	{
		// Default to server-level limit
		size_t maxBodySize = earlyCfg->getMaxBodySize();

		// If we can resolve a location, prefer its specific limit when set
		HttpRequest &earlyRequest = const_cast<HttpRequest &>(client.getRequest());
		RouteMatch earlyMatch = _router.matchRoute(*earlyCfg, earlyRequest);
		if (earlyMatch.location && earlyMatch.location->getMaxBodySize() > 0)
			maxBodySize = earlyMatch.location->getMaxBodySize();

		if (client.getRequest().getBody().size() > maxBodySize)
		{
			client.buildErrorResponse(413);
			client.markCloseAfterResponse();
			// Log the oversized request early rejection
			struct timeval end;
			gettimeofday(&end, NULL);
			double responseTime = 0.0; // Early guard happens during read; treat as immediate
			if (earlyCfg)
				Logger::logRequest(client.getRequest().getMethod(), client.getRequest().getUri(), client.getClientIp(), client.getResponseStatus(), client.getResponseBodySize(), responseTime, earlyCfg->getServerName(), earlyCfg->getPort());
			_pollFds[clientIndex].events = POLLOUT;
			return;
		}
	}

	// Check if request is complete
	if (!client.isRequestComplete())
		return;

	// Build response
	if (!client.isResponseReady())
	{
		client.setState(STATE_PROCESSING);
		client.updateActivity();

		const ServerConfig *config = selectConfig(client.getRequest(), clientFd);
		if (!config)
		{
			client.buildErrorResponse(500);
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
			client.buildErrorResponse(413);
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

			CGI cgi;
			CGIProcess *cgiProc = cgi.startAsync(match, request);
			if (cgiProc)
				cgiProc->executionTimeout = cgiExecutionTimeout;
			else
			{
				client.buildErrorResponse(500);
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
		std::cerr << "Error: client not found for fd " << clientFd << std::endl;
		return;
	}
	Client &client = it->second;

	// Update activity timestamp during progressive sending
	client.updateActivity();

	// Send response (may be progressive for large responses)
	bool sendComplete = client.sendResponse();

	if (!sendComplete)
	{
		// Sending not complete (EAGAIN or large response)
		// Keep POLLOUT active and retry later
		return; // Don't close connection yet
	}

	// Response fully sent, close connection and cleanup
	removeClient(clientFd, clientIndex);
}

void Server::removeClient(int fd, size_t pollIndex)
{
	safeClose(fd);
	_clients.erase(fd);
	_socketTypes.erase(fd);
	_pollFds.erase(_pollFds.begin() + pollIndex);
}

const ServerConfig *Server::selectConfig(const HttpRequest &request, int clientFd) const
{
	std::string host = request.getHeader("Host");
	int localPort = getSocketPort(clientFd);

	// Remove port from Host header if present
	size_t colonPos = host.find(':');

	const ServerConfig *defaultForPort = NULL;
	if (colonPos != std::string::npos)
		host = host.substr(0, colonPos);

	// Find config matching server_name
	for (size_t i = 0; i < _configs.size(); i++)
	{
		if (_configs[i].getPort() != localPort)
			continue;
		if (!defaultForPort)
			defaultForPort = &_configs[i];
		if (!host.empty() && _configs[i].getServerName() == host)
			return &_configs[i];
	}
	return defaultForPort;
}

void Server::handleCGITimeouts()
{
	time_t now = time(NULL);

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

			// Remove pipes from poll
			for (size_t i = 0; i < _pollFds.size();)
			{
				if (_pollFds[i].fd == cgi->pipeOut || _pollFds[i].fd == cgi->pipeIn || _pollFds[i].fd == cgi->pipeErr)
				{
					_socketTypes.erase(_pollFds[i].fd);
					_pollFds.erase(_pollFds.begin() + i);
				}
				else
					i++;
			}

			// Build 504 Gateway Timeout response
			client.buildErrorResponse(504);

			// Log the timeout event so it appears in server logs
			const ServerConfig *cfg = selectConfig(client.getRequest(), it->first);
			double responseTime = difftime(now, cgi->startTime) * 1000.0;
			if (cfg)
				Logger::logRequest(client.getRequest().getMethod(), client.getRequest().getUri(), client.getClientIp(), client.getResponseStatus(), client.getResponseBodySize(), responseTime, cfg->getServerName(), cfg->getPort());

			// Clean up CGI
			delete cgi;
			client.setCGIProcess(NULL);
			client.setState(STATE_KEEPALIVE);

			// Enable POLLOUT to send the error response
			for (size_t i = 0; i < _pollFds.size(); ++i)
				if (_pollFds[i].fd == it->first)
				{
					_pollFds[i].events = POLLOUT;
					break;
				}
		}
	}
}

void Server::handleCGIPipe(size_t pipeIndex)
{
	int pipeFd = _pollFds[pipeIndex].fd;

	// Find the client that owns this CGI
	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it)
	{
		Client &client = it->second;
		CGIProcess *cgi = client.getCGIProcess();
		if (!cgi)
			continue;

		// Handle pipeErr (reading CGI stderr)
		if (cgi->pipeErr == pipeFd && (_pollFds[pipeIndex].revents & (POLLIN | POLLHUP | POLLERR)))
		{
			char buffer[4096];
			ssize_t bytes = read(pipeFd, buffer, sizeof(buffer));

			if (bytes > 0)
				cgi->errorOutput.append(buffer, bytes);
			else
			{
				// EOF or error on stderr - remove from poll THEN close
				_pollFds.erase(_pollFds.begin() + pipeIndex);
				_socketTypes.erase(pipeFd);
				safeClose(cgi->pipeErr);
				cgi->pipeErr = -1;
			}
			return;
		}

		// Handle pipeOut (reading CGI output or detecting closure)
		if (cgi->pipeOut == pipeFd && (_pollFds[pipeIndex].revents & (POLLIN | POLLHUP | POLLERR)))
		{
			char buffer[4096];
			ssize_t bytes = read(pipeFd, buffer, sizeof(buffer));

			if (bytes > 0)
				cgi->output.append(buffer, bytes);
			else
			{
				// EOF - CGI finished
				// FIRST: Remove all CGI pipes from poll
				// Remove pipeOut (current pipe)
				_pollFds.erase(_pollFds.begin() + pipeIndex);
				_socketTypes.erase(pipeFd);
				
				// Remove pipeIn from poll if it exists
				if (cgi->pipeIn != -1)
				{
					for (size_t i = 0; i < _pollFds.size(); ++i)
					{
						if (_pollFds[i].fd == cgi->pipeIn)
						{
							_pollFds.erase(_pollFds.begin() + i);
							_socketTypes.erase(cgi->pipeIn);
							break;
						}
					}
				}
				
				// Remove pipeErr from poll if it exists
				if (cgi->pipeErr != -1)
				{
					for (size_t i = 0; i < _pollFds.size(); ++i)
					{
						if (_pollFds[i].fd == cgi->pipeErr)
						{
							_pollFds.erase(_pollFds.begin() + i);
							_socketTypes.erase(cgi->pipeErr);
							break;
						}
					}
				}
				
				// THEN: Close the pipes
				safeClose(cgi->pipeOut);
				cgi->pipeOut = -1;
				
				if (cgi->pipeIn != -1)
				{
					safeClose(cgi->pipeIn);
					cgi->pipeIn = -1;
				}
				
				if (cgi->pipeErr != -1)
				{
					safeClose(cgi->pipeErr);
					cgi->pipeErr = -1;
				}

				int status;
				waitpid(cgi->pid, &status, 0);

				bool cgiError = (WIFEXITED(status) && WEXITSTATUS(status))
								|| WIFSIGNALED(status)
								|| cgi->output.empty()
								|| cgi->output.find("Content-Type:") == std::string::npos;

				if (cgiError)
					client.buildErrorResponse(500);
				else
				{
					CGIResult result;
					result.output = cgi->output;
					CGI cgiParser;
					cgiParser.parseHeaders(cgi->output, result);
					client.buildResponseFromCGI(result);
				}

				if (cgiError)
				{
					const ServerConfig *cfg = selectConfig(client.getRequest(), it->first);
					time_t now = time(NULL);
					double responseTime = difftime(now, cgi->startTime) * 1000.0;
					if (cfg)
					{
						Logger::logRequest(client.getRequest().getMethod(), client.getRequest().getUri(), client.getClientIp(), client.getResponseStatus(), client.getResponseBodySize(), responseTime, cfg->getServerName(), cfg->getPort());
						if (!cgi->errorOutput.empty())
							Logger::logMessage(RED "CGI Error:\n" RESET + cgi->errorOutput);
					}
				}
				
				delete cgi;
				client.setCGIProcess(NULL);
				client.setState(STATE_KEEPALIVE);
				
				for (size_t i = 0; i < _pollFds.size(); ++i)
					if (_pollFds[i].fd == it->first)
					{
						_pollFds[i].events |= POLLOUT;
						break;
					}
				return;
			}
		}

		// Handle pipeIn (writing POST body to CGI)
		if (cgi->pipeIn == pipeFd && (_pollFds[pipeIndex].revents & POLLOUT))
		{
			if (!cgi->inputWritten)
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
							
							// Remove from poll THEN close
							for (size_t i = 0; i < _pollFds.size(); ++i)
							{
								if (_pollFds[i].fd == cgi->pipeIn)
								{
									_pollFds.erase(_pollFds.begin() + i);
									_socketTypes.erase(cgi->pipeIn);
									break;
								}
							}
							safeClose(cgi->pipeIn);
							cgi->pipeIn = -1;
						}
					}
					else if (written < 0 && errno != EAGAIN && errno != EWOULDBLOCK)
					{
						std::cerr << "[CGI] ❌ ASSERT: Write error to stdin: " << strerror(errno) << std::endl;
					}
					else if (written < 0)
					{
						std::cout << "[CGI] Write would block (EAGAIN), will retry..." << std::endl;
					}
				}
			}
		}
		return;
	}
}

void Server::handleSessionTimeouts()
{
	// Check cleanup interval
	if (time(NULL) - _lastSessionCleanup <= SESSION_CLEANUP_INTERVAL)
		return;
	_lastSessionCleanup = time(NULL);

	// Remove expired sessions
	std::map<std::string, SessionData>::iterator it = _sessions.begin();
	while (it != _sessions.end())
	{
		if (time(NULL) - it->second.lastActive > SESSION_TIMEOUT)
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
	while (read(_s_sigpipe[0], buf, sizeof(buf)) > 0)
		;
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
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &Server::signalHandler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGINT, &sa, 0);
	sigaction(SIGTERM, &sa, 0);

	// Ignore SIGPIPE to prevent termination on broken socket writes
	signal(SIGPIPE, SIG_IGN);
}
