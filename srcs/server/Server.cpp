/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:49 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/05 15:08:40 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include "Client.hpp"
#include "Router.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../utils/utils.hpp"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

Server::Server() : _running(false) {}

Server::~Server() {
	for (size_t i = 0; i < _listenSockets.size(); i++)
		safeClose(_listenSockets[i]);
}

// Public method(s)
void Server::init(const Config &config) {
	_configs = config.getServers();
	setupListenSockets();
}

void Server::checkTimeouts() {
	for (size_t i = 0; i < _clients.size(); ) {

		// 30 seconds timeout for idle clients
		if (_clients[i].hasTimedOut(30)) {
			std::cout << "Client timeout (fd " << _clients[i].getSocket() << ")" << std::endl;

			// Find and remove corresponding pollfd
			for (size_t j = 0; j < _pollFds.size(); j++) {
				if (_pollFds[j].fd == _clients[i].getSocket()) {
					safeClose(_pollFds[j].fd);
					_pollFds.erase(_pollFds.begin() + j);
					break;
				}
			}
			_clients.erase(_clients.begin() + i);

			// Don't increment i, element removed
		} else
			i++;
	}
}

void Server::run() {
    _running = true;
    std::cout << "Server running... (Ctrl+C to stop)" << std::endl;
	static time_t lastCleanup = 0;

    while (_running) {
        // Check for idle client timeouts
        checkTimeouts();

		// Cleanup expired sessions every 60 seconds
        if (time(NULL) - lastCleanup > 60) {
            cleanupSessions();
            lastCleanup = time(NULL);
        }

        // Poll for events on all sockets (1 second timeout)
        int ret = poll(&_pollFds[0], _pollFds.size(), 1000);
        if (ret < 0)
            continue;

        // Process events on each socket
        for (size_t i = 0; i < _pollFds.size(); i++) {
            // Handle POLLIN (incoming data to read)
            if (_pollFds[i].revents & POLLIN) {
                if (isListenSocket(_pollFds[i].fd))
                    acceptNewClient(_pollFds[i].fd);
                else
                    handleClientRead(i);
            }
            
            // Handle POLLOUT (socket ready to write)
            if (_pollFds[i].revents & POLLOUT) {
                handleClientWrite(i);
            }
        }
    }
}

void Server::stop() {
	_running = false;
}

void Server::setupListenSockets() {
	// Create one listening socket per configuration (one per port)
	for (size_t i = 0; i < _configs.size(); i++) {
		int port = _configs[i].getPort();
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
		addr.sin_family = AF_INET; // IPv4
		addr.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces
		addr.sin_port = htons(port); // Convert port to network byte order
		if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			safeClose(listenFd);
			throw std::runtime_error("bind() failed");
		}

		// Start listening for incoming connections
		if (listen(listenFd, 128) < 0) {
			safeClose(listenFd);
			throw std::runtime_error("listen() failed");
		}
		_listenSockets.push_back(listenFd);

		// Add to poll to monitor incoming connections
		struct pollfd pfd;
		pfd.fd = listenFd;
		pfd.events = POLLIN; // Watch for incoming data
		pfd.revents = 0;
		_pollFds.push_back(pfd);
		std::cout << "Server listening on port " << port << std::endl;
	}
}

// Check if given fd is a listening socket
bool Server::isListenSocket(int fd) const {
	for (size_t i = 0; i < _listenSockets.size(); i++)
		if (_listenSockets[i] == fd)
			return true;
	return false;
}

void Server::acceptNewClient(int listenSocket) {
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);

	// Accept a new incoming connection (non-blocking)
	int clientFd = accept(listenSocket, (struct sockaddr*)&clientAddr, &addrLen);
	if (clientFd < 0) {
		std::cerr << "[Server] accept failed on fd " << listenSocket << std::endl;
		return; // No connection available right now
	}	

	// Set the client socket to non-blocking mode
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0) {
		std::cerr << "[Server] fcntl failed for fd " << clientFd << std::endl;
		safeClose(clientFd);
		return;
	}
	// Add a Client to the vector
	Client newClient(clientFd);
	_clients.push_back(newClient);

	// Add the new client socket to poll() to monitor incoming data
	struct pollfd pfd;
	pfd.fd = clientFd;
	pfd.events = POLLIN; // Triggered when client sends data
	pfd.revents = 0;
	_pollFds.push_back(pfd);
	std::cout << "New client connected (fd " << clientFd << ")" << std::endl;
}

const ServerConfig* Server::selectConfig(const HttpRequest& request) const {
	const ServerConfig* config = &_configs[0];
	std::string host = request.getHeader("Host");

	// Remove port from Host header if present
	size_t colonPos = host.find(':');
	if (colonPos != std::string::npos)
		host = host.substr(0, colonPos);

	// Find config matching server_name
	for (size_t i = 0; i < _configs.size(); i++) {
		if (_configs[i].getServerName() == host) {
			config = &_configs[i];
			break;
		}
	}
	return config;
}

void Server::handleClientRead(size_t clientIndex) {
    int clientFd = _pollFds[clientIndex].fd;

    // Find the client with this fd
    size_t i;
    for (i = 0; i < _clients.size(); i++)
        if (_clients[i].getSocket() == clientFd)
            break;
    if (i == _clients.size()) {
        std::cerr << "Error: client not found for fd " << clientFd << std::endl;
        return;
    }
    Client& client = _clients[i];

    // Read data from socket
    if (!client.readData()) {
        // Error or disconnection
        std::cout << "Client disconnected (fd " << client.getSocket() << ")" << std::endl;
        safeClose(client.getSocket());
        _clients.erase(_clients.begin() + i);
        _pollFds.erase(_pollFds.begin() + clientIndex);
        return;
    }

    // Check if request is complete
    if (!client.isRequestComplete())
        return;
    
    std::cout << "📨 Request received" << std::endl;

    // Build response
    const ServerConfig* config = selectConfig(client.getRequest());
    client.buildResponse(*config, _router, _sessions);

    // Enable POLLOUT to send response when socket is ready for writing
    _pollFds[clientIndex].events |= POLLOUT;
}

void Server::handleClientWrite(size_t clientIndex) {
    int clientFd = _pollFds[clientIndex].fd;

    // Find the client with this fd
    size_t i;
    for (i = 0; i < _clients.size(); i++)
        if (_clients[i].getSocket() == clientFd)
            break;
    if (i == _clients.size()) {
        std::cerr << "Error: client not found for fd " << clientFd << std::endl;
        return;
    }
    Client& client = _clients[i];

    // Send response
    if (!client.sendResponse()) {
        std::cerr << "Error sending response to fd " << clientFd << std::endl;
    }

    // Close connection and cleanup after sending
    safeClose(client.getSocket());
    _clients.erase(_clients.begin() + i);
    _pollFds.erase(_pollFds.begin() + clientIndex);
}

void Server::cleanupSessions() {
	time_t now = time(NULL);
	std::map<std::string, SessionData>::iterator it = _sessions.begin();
	while (it != _sessions.end()) {
		if (now - it->second.lastActive > 1800) { // 30 minutes
			std::map<std::string, SessionData>::iterator toErase = it;
			it++;
			_sessions.erase(toErase);
		} else
			it++;
	}
}
