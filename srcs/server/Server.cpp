/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:49 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/12 18:40:37 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "Server.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../utils/utils.hpp"
#include "../utils/Logger.hpp"
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <unistd.h>
#include <cerrno>
#include <arpa/inet.h> // IP client

// Self-pipe for signal handling in poll()
int Server::_s_sigpipe[2] = {-1, -1};

Server::Server(const Config& config)
	: _configs(config.getServers())
	, _running(false)
	, _lastSessionCleanup(0)
{
	installSignals();
	setupListenSockets();
}

// Destructor
Server::~Server() {
	// Close all sockets (skip signal pipe at index 0)
	for (size_t i = 1; i < _pollFds.size(); i++)
		safeClose(_pollFds[i].fd);
	// Close signal pipe explicitly
	if (_s_sigpipe[0] >= 0)
		safeClose(_s_sigpipe[0]);
	if (_s_sigpipe[1] >= 0)
		safeClose(_s_sigpipe[1]);
	std::cout << "\nServer was closed" << std::endl;
}

// Public method(s)
void Server::run() {
	_running = true;
	std::cout << "Server running... (Ctrl+C to stop)" << std::endl;

	while (_running) {
		// Check for idle client timeouts
		handleClientTimeouts();
		handleSessionTimeouts();

		// Poll for events on all sockets (1 second timeout)
		int ret = poll(&_pollFds[0], _pollFds.size(), 1000);
		if (ret < 0)
			continue;

		// Process events on each socket
		for (size_t i = 0; i < _pollFds.size(); i++) {

			// Handle POLLIN (incoming data to read)
			if (_pollFds[i].revents & POLLIN) {

				// Handle SIGINT or SIGTERM
				if (_socketTypes[_pollFds[i].fd] == SOCKET_SIGNAL) {
					handleSignalPipeReadable();
					break;
				}

				// Handle listen or client socket
				if (_socketTypes[_pollFds[i].fd] == SOCKET_LISTEN)
					acceptNewClient(_pollFds[i].fd);
				else
					handleClientRead(i);
			}

			// Handle POLLOUT (socket ready to write)
			if (_pollFds[i].revents & POLLOUT)
				handleClientWrite(i);
		}
	}
}

void Server::stop() {
	_running = false;
}

// Private method(s)

void Server::setupListenSockets() {
	// Create one listening socket per configuration (one per port)
	std::vector<int> port;
	for (size_t i = 0; i < _configs.size(); i++) {
		if(std::find(port.begin(), port.end(), _configs[i].getPort()) != port.end())
		{
			std::cout << "Server "<<_configs[i].getServerName()<<" listening on port " << _configs[i].getPort() << std::endl;
			continue; // Skip duplicate ports
		}
		port.push_back(_configs[i].getPort());
		int listenFd = socket(AF_INET, SOCK_STREAM, 0); // IPv4, TCP
		if (listenFd < 0)
			throw std::runtime_error("socket() failed");

		// Configure SO_REUSEADDR to allow address reuse and prevent "Address already in use" error
		int opt = 1;
		if (setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
			safeClose(listenFd);
			throw std::runtime_error("setsockopt() failed");
		}

		// Configure the server address structure
		struct sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;		   // IPv4
		addr.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces
		addr.sin_port = htons(port[i]);	   // Convert port to network byte order
		if (bind(listenFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
			safeClose(listenFd);
			throw std::runtime_error("bind() failed");
		}

		// Start listening for incoming connections
		if (listen(listenFd, 128) < 0) {
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
		std::cout << "Server "<<_configs[i].getServerName()<<" listening on port " << port[i] << std::endl;
	}
}

void Server::acceptNewClient(int listenSocket) {
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);

	// Accept a new incoming connection (non-blocking)
	int clientFd = accept(listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
	if (clientFd < 0) {
		std::cerr << "[Server] accept failed on fd " << listenSocket << std::endl;
		return; // No connection available right now
	}

	// Get client IP
	char clientIp[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, INET_ADDRSTRLEN);

	// Set the client socket to non-blocking mode
	try {
		setNonBlocking(clientFd);
	} catch (const std::exception& e) {
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
	Logger::logConnection(clientFd, std::string(clientIp));
}

void Server::handleClientTimeouts() {
	std::map<int, Client>::iterator it = _clients.begin();
	while (it != _clients.end()) {
		if (it->second.hasTimedOut(CLIENT_IDLE_TIMEOUT, CLIENT_PROCESSING_TIMEOUT)) {
			int fd = it->first;
			std::cout << "Client timeout (fd " << fd << ")" << std::endl;
			++it;
			for (size_t j = 0; j < _pollFds.size(); j++) {
				if (_pollFds[j].fd == fd) {
					removeClient(fd, j);
					break;
				}
			}
		}
		else
			++it;
	}
}

void Server::handleClientRead(size_t clientIndex) {
	int clientFd = _pollFds[clientIndex].fd;

	// Find the client with this fd
	std::map<int, Client>::iterator it = _clients.find(clientFd);
	if (it == _clients.end()) {
		std::cerr << "Error: client not found for fd " << clientFd << std::endl;
		return;
	}
	Client &client = it->second;

	// Read data from socket
	if (!client.readData()) {
		removeClient(clientFd, clientIndex);
		return;
	}

	// Check if request is complete
	if (!client.isRequestComplete())
		return;	

	// Build response
	if (!client.isResponseReady())
	{
		client.setState(STATE_PROCESSING); // Switch to processing state (allows longer timeout for CGI)
		const ServerConfig *config = selectConfig(client.getRequest(), clientFd);
		client.buildResponse(*config, _router, _sessions);
		client.setState(STATE_IDLE); // Back to idle state (ready to write)
	}

	// Enable POLLOUT to send response when socket is ready for writing
	_pollFds[clientIndex].events |= POLLOUT;
}

void Server::handleClientWrite(size_t clientIndex) {
	int clientFd = _pollFds[clientIndex].fd;

	// Find the client with this fd
	std::map<int, Client>::iterator it = _clients.find(clientFd);
	if (it == _clients.end()) {
		std::cerr << "Error: client not found for fd " << clientFd << std::endl;
		return;
	}
	Client &client = it->second;

	// Send response
	if (!client.sendResponse())
		std::cerr << "Error sending response to fd " << clientFd << std::endl;

	// Close connection and cleanup after sending
	removeClient(clientFd, clientIndex);
}

void Server::removeClient(int fd, size_t pollIndex) {
	safeClose(fd);
	_clients.erase(fd);
	_socketTypes.erase(fd);
	_pollFds.erase(_pollFds.begin() + pollIndex);
}

const ServerConfig *Server::selectConfig(const HttpRequest &request, int clientFd) const {
	std::string host = request.getHeader("Host");
	int localPort = getSocketPort(clientFd);

	// Remove port from Host header if present
	size_t colonPos = host.find(':');

	const ServerConfig* defaultForPort = NULL;
	if (colonPos != std::string::npos)
		host = host.substr(0, colonPos);

	// Find config matching server_name
	for (size_t i = 0; i < _configs.size(); i++) {
		if (_configs[i].getPort() != localPort)
			continue;
		if (!defaultForPort)
			defaultForPort = &_configs[i];
		if (!host.empty() && _configs[i].getServerName() == host)
			return &_configs[i];
	}
	return defaultForPort;
}

void Server::handleSessionTimeouts() {
	// Check cleanup interval
	if (time(NULL) - _lastSessionCleanup <= SESSION_CLEANUP_INTERVAL)
		return;
	_lastSessionCleanup = time(NULL);
	
	// Remove expired sessions
	std::map<std::string, SessionData>::iterator it = _sessions.begin();
	while (it != _sessions.end()) {
		if (time(NULL) - it->second.lastActive > SESSION_TIMEOUT) {
			std::map<std::string, SessionData>::iterator toErase = it;
			it++;
			_sessions.erase(toErase);
		}
		else
			it++;
	}
}

// Drain pipe so it doesn't remain readable and stop server
void Server::handleSignalPipeReadable() {
	char buf[64];
	while (read(_s_sigpipe[0], buf, sizeof(buf)) > 0);
	_running = false;
}

void Server::addSignalPipeToPoll() {
	pollfd pfd;
	pfd.fd = _s_sigpipe[0];
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pollFds.push_back(pfd);
	_socketTypes[_s_sigpipe[0]] = SOCKET_SIGNAL;
}

// Called by OS when SIGINT/SIGTERM received (async-signal-safe)
void Server::signalHandler(int) {
	if (_s_sigpipe[1] != -1)
		write(_s_sigpipe[1], "1", 1);
}

void Server::installSignals() {
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
