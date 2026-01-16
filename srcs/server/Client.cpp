/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/16 11:36:32 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "Client.hpp"
#include "Router.hpp"
#include "Server.hpp"
#include "../utils/Logger.hpp"
#include "../cgi/CGI.hpp"
#include "../utils/utils.hpp"
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

// Constructor: initialize socket and activity timestamp
Client::Client(int socket, const std::string &clientIp)
	: _socket(socket)
	, _clientIp(clientIp)
	, _lastActivity(time(NULL))
	, _requestComplete(false)
	, _responseReady(false)
	, _state(STATE_IDLE)
	, _cgiProcess(NULL)
{}

// Private method(s)
void Client::handleSession(std::map<std::string, SessionData>& sessions) {

	std::map<std::string, std::string> cookies = _request.getCookies();
	std::string sessionId;

	if (cookies.find("session_id") != cookies.end()) {
		sessionId = cookies["session_id"];

		// Update existing session or create new if expired
		if (sessions.find(sessionId) != sessions.end()) {
			sessions[sessionId].lastActive = time(NULL);

			// Only count html request (for good count page visit)
			std::string uri = _request.getUri();
			bool isInternalRequest = _request.getHeader("X-Internal-Request") == "true";
			bool isHtmlPage = (uri == "/" ||
							uri.find(".html") != std::string::npos ||
							(uri.find('.') == std::string::npos && uri != "/counter-api"));
			if (isHtmlPage && !isInternalRequest)
					sessions[sessionId].visitCount++;
		} else {
			// Invalid/expired session → create new
			sessionId = generateSessionId();
			sessions[sessionId].lastActive = time(NULL);
			sessions[sessionId].visitCount = 1;
			sessions[sessionId].username = "";
		// _response.setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly"); // Disabled for tester
	}
} else {
	// New session
	sessionId = generateSessionId();
	sessions[sessionId].lastActive = time(NULL);
	sessions[sessionId].visitCount = 1;
	sessions[sessionId].username = "";
	// _response.setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly"); // Disabled for tester
}

_sessionId = sessionId;
}

const std::string &Client::getClientIp() const {
	return _clientIp;
}

bool Client::hasTimedOut(time_t idleTimeout, time_t processingTimeout) const {
	time_t timeout;

	// Use longer timeout during processing to allow CGI scripts to complete
	if (_state == STATE_PROCESSING)
		timeout = processingTimeout;
	else
		timeout = idleTimeout;

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

bool Client::isResponseReady() const {
	return _responseReady;
}

void Client::setState(ClientState state) {
	_state = state;
}

CGIProcess* Client::getCGIProcess() const {
	return _cgiProcess;
}

void Client::setCGIProcess(CGIProcess* cgi) {
	_cgiProcess = cgi;
}

// Public method(s)
bool Client::readData() { // Read data from socket into buffer and parse request
	char buffer[4096];
	int bytesRead = recv(_socket, buffer, sizeof(buffer), 0);
	std::cerr << "[DEBUG] readData: bytesRead=" << bytesRead << std::endl;
	if (bytesRead <= 0)
		return false;
	// Append only the new data to the request
	std::string newData(buffer, bytesRead);
	_request.appendData(newData);
	std::cerr << "[DEBUG] Request complete: " << _request.isComplete() << ", ErrorCode: " << _request.getErrorCode() << std::endl;
	if (_request.isComplete()) {
		_requestComplete = true;
		if (_request.getErrorCode()) {
			std::cerr << "[DEBUG] Request has error code " << _request.getErrorCode() << ", building error response" << std::endl;
			buildErrorResponse(_request.getErrorCode());
			_responseReady = true;
		}
	}
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
	} else
		_response.setBody("<html><body><h1>" + intToString(statusCode) + " Error</h1></body></html>");
}

void Client::buildResponse(const ServerConfig& config, Router& router, std::map<std::string, SessionData>& sessions) {

	std::cerr << "[DEBUG] buildResponse called for " << _request.getMethod() << " " << _request.getUri() << std::endl;

	// Timer
	struct timeval start, end;
	gettimeofday(&start, NULL);

	// Check body size limit
	if (_request.getBody().size() > config.getMaxBodySize()) {
		buildErrorResponse(413);
		// log + return
		gettimeofday(&end, NULL);
		double responseTime = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
		Logger::logRequest(_request.getMethod(), _request.getUri(), _clientIp, _response.getStatus(), _response.getBody().size(), responseTime, config.getServerName(), config.getPort());
		_responseReady = true;
		return;
	}

	handleSession(sessions);

	if (_request.getUri() == "/counter-api") {
		SessionData &session = sessions[_sessionId];

		std::string json = "{\"visitCount\":" + intToString(session.visitCount) + ",\"sessionId\":\"" + _sessionId + "\"}";
		_response.setStatus(200);
		_response.setHeader("Content-Type", "application/json");
		_response.setBody(json);

		// log and return
		gettimeofday(&end, NULL);
		double responseTime = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
		Logger::logRequest(_request.getMethod(), _request.getUri(), _clientIp, _response.getStatus(),
							 _response.getBody().size(), responseTime, config.getServerName(), config.getPort());
		_responseReady = true;
		return ;
	}

	// Continue with normal routing
	RouteMatch match = router.matchRoute(config, _request);

	// Handle redirections
	if (!match.redirectUrl.empty()) {
		_response.setStatus(match.statusCode);
		_response.setHeader("Location", match.redirectUrl);
		_response.setBody("");
	}

	// Handle errors (405 Method Not Allowed, 404 Not Found, 501 Not Implemented)
	else if (match.statusCode == 405 || match.statusCode == 404 || match.statusCode == 501)
		_response.serveError(match.statusCode, "");

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

	// Logging
	gettimeofday(&end, NULL);
	double responseTime = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;

	Logger::logRequest(
		_request.getMethod(),
		_request.getUri(),
		_clientIp,
		_response.getStatus(),
		_response.getBody().size(),
		responseTime,
		config.getServerName(),
		config.getPort()
	);
	_responseReady = true;
}

void Client::buildResponseFromCGI(const CGIResult& result) {
	if (result.statusCode == 200) {
		_response.setStatus(200);
		_response.setHeader("Content-Type", result.contentType);
		_response.setBody(result.output);
	} else
		_response.serveError(result.statusCode, "");
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
		if (!sent)
			break; // Connection closed by peer
		totalSent += sent;
		remaining -= sent;
	}
	return (!remaining); // true if everything was sent
}
