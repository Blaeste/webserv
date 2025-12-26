/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:46 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/26 15:47:10 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "../cgi/CGI.hpp"
#include "../server/Router.hpp"
#include "../utils/utils.hpp"
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
	_readBuffer.append(buffer, bytesRead);
	_request.appendData(_readBuffer);
	if (_request.isComplete())
		_requestComplete = true;
	updateActivity();
	return true;
}

void Client::buildErrorResponse(int statusCode) {
	_response.setStatus(statusCode);
	_response.setHeader("Content-Type", "text/html");
	std::string errorPage = "www/error_pages/" + intToString(statusCode) + ".html";
	if (fileExists(errorPage))
		_response.setBody(readFile(errorPage));
	else
		_response.setBody("<html><body><h1>" + intToString(statusCode) + " Error</h1></body></html>");
}

void Client::buildResponse(const ServerConfig& config, Router& router) {
	// Check body size limit
	if (_request.getBody().size() > config.getMaxBodySize()) {
		buildErrorResponse(413); // 413 Payload Too Large
		_responseReady = true;
		return;
	}

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
	int bytesSent = send(_socket, rawResponse.c_str(), rawResponse.length(), 0);
	if (bytesSent < 0)
		return false;
	return true;
}
