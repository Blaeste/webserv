/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:49 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/23 16:37:42 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Router.hpp"
#include "Server.hpp"
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
		close(_listenSockets[i]);
}

void Server::init(const Config &config) {
	_configs = config.getServers();
	setupListenSockets();
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
			close(listenFd);
			throw std::runtime_error("setsockopt() failed");
		}

		// Configure the server address structure
		struct sockaddr_in addr;
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET; // IPv4
		addr.sin_addr.s_addr = INADDR_ANY; // Listen on all network interfaces
		addr.sin_port = htons(port); // Convert port to network byte order
		if (bind(listenFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			close(listenFd);
			throw std::runtime_error("bind() failed");
		}
		
		// Start listening for incoming connections
		if (listen(listenFd, 128) < 0) {
			close(listenFd);
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

void Server::stop() {
	_running = false;
}

bool Server::isListenSocket(int fd) const {
	for (size_t i = 0; i < _listenSockets.size(); i++)
		if (_listenSockets[i] == fd)
			return true;
	return false;
}

void Server::run() {
	_running = true;
	std::cout << "Server running... (Ctrl+C to stop)" << std::endl;
	while (_running) {
		// Wait for events on file descriptors = monitored sockets
		int ret = poll(&_pollFds[0], _pollFds.size(), 1000); // 1 second timeout
		if (ret < 0)
			continue; // Error, but keep running

			// Iterate through all sockets to check for events
		for (size_t i = 0; i < _pollFds.size(); i++) {
			// There is data to read or a new connection
			if (_pollFds[i].revents & POLLIN) {
				if (isListenSocket(_pollFds[i].fd))
					acceptNewClient(_pollFds[i].fd); // Accept a new client connection
				else
					handleClientRead(i); // Handle incoming data from a client
			}
		}
	}
	std::cout << "Server stopped.";
}

void Server::acceptNewClient(int listenSocket) {
	struct sockaddr_in clientAddr;
	socklen_t addrLen = sizeof(clientAddr);
	
	// Accept a new incoming connection (non-blocking)
	int clientFd = accept(listenSocket, (struct sockaddr*)&clientAddr, &addrLen);
	if (clientFd < 0)
		return; // No connection available right now

	// Set the client socket to non-blocking mode
	fcntl(clientFd, F_SETFL, O_NONBLOCK);

	// Add the new client socket to poll() to monitor incoming data
	struct pollfd pfd;
	pfd.fd = clientFd;
	pfd.events = POLLIN; // Triggered when client sends data
	pfd.revents = 0;
	_pollFds.push_back(pfd);

	std::cout << "New client connected (fd " << clientFd << ")" << std::endl;
}

void Server::handleClientRead(size_t clientIndex) {
	char buffer[4096];
	int clientFd = _pollFds[clientIndex].fd;

	// Read incoming data from the client socket
	int bytesRead = recv(clientFd, buffer, sizeof(buffer), 0);
	if (bytesRead <= 0) {
		std::cout << "Client disconnected (fd " << clientFd << ")" << std::endl;
		close(clientFd);
		_pollFds.erase(_pollFds.begin() + clientIndex);
		return;
	}

	// Append received data to the HTTP request parser
	HttpRequest request;
	std::string data(buffer, bytesRead);
	request.appendData(data);
	
	// Wait until the full HTTP request has been received
	if (!request.isComplete()) {
		std::cout << "Incomplete request, waiting for more data..." << std::endl;
		return;
	}

	// Log basic request information
	std::cout << "📨 " << request.getMethod() << " " << request.getUri() << std::endl;

	// Select the correct ServerConfig based on the Host header
	// Fallback to the first configuration if no match is found
	const ServerConfig* config = &_configs[0];
	std::string host = request.getHeader("Host");

	// Remove port number from Host header if present (e.g. "localhost:8080")
	size_t colonPos = host.find(':');
	if (colonPos != std::string::npos)
		host = host.substr(0, colonPos);
	for (size_t i = 0; i < _configs.size(); i++) {
		if (_configs[i].getServerName() == host) {
			config = &_configs[i];
			break;
		}
	}
	// Use the router to match the request to a route and determine the action
	RouteMatch match = _router.matchRoute(*config, request);

	// Build the HTTP response based on the routing result
	HttpResponse response;
	if (!match.redirectUrl.empty()) {
		// Handle HTTP redirection
		response.setStatus(match.statusCode);
		response.setHeader("Location", match.redirectUrl);
		response.setBody("");
	} else if (match.statusCode == 405) {
		// Method not allowed for this route
		response.setStatus(405);
		response.setHeader("Content-Type", "text/html");
		std::string errorPage = "www/error_pages/405.html";
		if (fileExists(errorPage))
			response.setBody(readFile(errorPage));
		else
			response.setBody("<html><body><h1>405 Method Not Allowed</h1></body></html>");
	} else if (match.statusCode == 404) {
		// Requested resource not found
		response.setStatus(404);
		response.setHeader("Content-Type", "text/html");
		std::string errorPage = "www/error_pages/404.html";
		if (fileExists(errorPage))
			response.setBody(readFile(errorPage));
		else
			response.setBody("<html><body><h1>404 Not Found</h1></body></html>");
	} else {
		// Resource found: serve the requested file
		response.setStatus(200);
		std::string ext = getFileExtension(match.filePath);
		response.setHeader("Content-Type", "text/html"); // TODO: determine MIME type from extension
		response.setBody(readFile(match.filePath));
	}

	// Send the response to the client
	std::string rawResponse = response.build();
	send(clientFd, rawResponse.c_str(), rawResponse.length(), 0);

	// Close the connection and remove the client from poll monitoring
	close(clientFd);
	_pollFds.erase(_pollFds.begin() + clientIndex);
}
