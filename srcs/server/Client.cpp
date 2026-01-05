/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/05 15:10:44 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "../cgi/CGI.hpp"
#include "../server/Router.hpp"
#include "../utils/utils.hpp"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

// Constructor: initialize socket and activity timestamp
Client::Client(int socket)
	: _socket(socket)
	, _lastActivity(time(NULL))
	, _requestComplete(false)
	, _responseReady(false)
{}

Client::~Client() {}

int Client::getSocket() const {
	return _socket;
}

bool Client::hasTimedOut(time_t timeout) const {
	return time(NULL) - _lastActivity > timeout;
}

void Client::updateActivity() {
	_lastActivity = time(NULL);
}

const HttpRequest& Client::getRequest() const {
	return _request;
}

bool Client::isRequestComplete() const {
	return _requestComplete;
}

// Read data from socket into buffer and parse request
bool Client::readData() {
	char buffer[4096];
	int bytesRead = recv(_socket, buffer, sizeof(buffer), 0);
	if (bytesRead <= 0)
		return false;
	// Append only the new data to the request
	std::string newData(buffer, bytesRead);
	_request.appendData(newData);
	if (_request.isComplete())
		_requestComplete = true;
	updateActivity();
	return true;
}

void Client::buildErrorResponse(int statusCode) {
	_response.setStatus(statusCode);
	_response.setHeader("Content-Type", "text/html");
	std::string errorPage = "www/error_pages/" + intToString(statusCode) + ".html";
	if (fileExists(errorPage)) {
		try {
			_response.setBody(readFile(errorPage));
		} catch (const std::exception& e) {
			std::cerr << "[Client] buildErrorResponse: " << e.what() << std::endl;
			_response.setBody("<html><body><h1>" + intToString(statusCode) + " Error</h1></body></html>");
		}
	} else {
		_response.setBody("<html><body><h1>" + intToString(statusCode) + " Error</h1></body></html>");
	}
}

void Client::handleSession() {
	std::map<std::string, std::string> cookies = _request.getCookies();
	std::string sessionId;
	if (cookies.find("session_id") != cookies.end()) {
		sessionId = cookies["session_id"];
	} else {
		sessionId = generateSessionId();
		_response.setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly");
	}
}

void Client::buildResponse(const ServerConfig& config, Router& router) {
	// Check body size limit
	if (_request.getBody().size() > config.getMaxBodySize()) {
		buildErrorResponse(413); // 413 Payload Too Large
		_responseReady = true;
		return;
	}

	handleSession();

	RouteMatch match = router.matchRoute(config, _request);

	// Handle redirections
	if (!match.redirectUrl.empty()) {
		_response.setStatus(match.statusCode);
		_response.setHeader("Location", match.redirectUrl);
		_response.setBody("");
	}

	// Handle errors (405 Method Not Allowed, 404 Not Found)
	else if (match.statusCode == 405 || match.statusCode == 404)
		_response.serveError(match.statusCode, "");

	// Execute CGI script
	else if (match.isCGI) {
		CGI cgi;
		CGIResult result = cgi.execute(match, _request);
		if (result.statusCode == 200) {
			_response.setStatus(200);
			_response.setHeader("Content-Type", result.contentType);
			_response.setBody(result.output);
		} else
			_response.serveError(result.statusCode, "");
	}

	// Handle DELETE request
	else if (_request.getMethod() == "DELETE")
		_response.serveDelete(match.filePath);

	// Handle file upload (POST with uploaded files)
	else if (_request.getMethod() == "POST" && !_request.getUploadedFiles().empty())
		_response.handleUpload(_request, match.location->getUploadPath());

	// Serve directory listing if autoindex is enabled
	else if (isDirectory(match.filePath) && match.location->getAutoIndex())
		_response.serveDirectoryListing(match.filePath, _request.getUri());

	// Serve static file
	else
		_response.serveFile(match.filePath);
	_responseReady = true;
}

bool Client::sendResponse() {
	std::string rawResponse = _response.build();
	
	// Send with partial send handling
	ssize_t totalSent = 0;
	ssize_t remaining = rawResponse.size();
	while (remaining > 0) {
		ssize_t sent = send(_socket, rawResponse.data() + totalSent, remaining, 0);
		if (sent < 0) {
			std::cerr << "[Client] sendResponse: send failed on fd " << _socket << std::endl;
			return false;
		}
		if (sent == 0)
			break;  // Connection closed by peer
		totalSent += sent;
		remaining -= sent;
	}
	return (remaining == 0);  // true if everything was sent
}
