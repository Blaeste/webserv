/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/05 13:11:27 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "Client.hpp"
#include "Router.hpp"
#include "Server.hpp"
#include "../utils/Logger.hpp"
#include "../cgi/CGI.hpp"
#include "../utils/utils.hpp"
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>

// Constructor: initialize socket and activity timestamp
Client::Client(int socket, const std::string &clientIp)
	: _socket(socket), _clientIp(clientIp), _lastActivity(time(NULL)), _requestComplete(false), _responseReady(false), _closeAfterResponse(false), _state(STATE_KEEPALIVE), _cgiProcess(NULL), _bytesSent(0), _cgiStartTime(), _serverConfig(NULL)
{
}

// Private method(s)
void Client::handleSession(std::map<std::string, SessionData> &sessions)
{
	std::map<std::string, std::string> cookies = _request.getCookies();
	std::string sessionId;

	if (cookies.find("session_id") != cookies.end())
	{
		sessionId = cookies["session_id"];

		// Update existing session or create new if expired
		if (sessions.find(sessionId) != sessions.end())
		{
			sessions[sessionId].lastActive = time(NULL);

			// Only count html request (for good count page visit)
			std::string uri = _request.getUri();
			bool isInternalRequest = _request.getHeader("X-Internal-Request") == "true";
			bool isHtmlPage = (uri == "/" ||
							   uri.find(".html") != std::string::npos ||
							   (uri.find('.') == std::string::npos && uri != "/counter-api"));
			if (isHtmlPage && !isInternalRequest)
				sessions[sessionId].visitCount++;
		}
		else
		{
			// Invalid/expired session → create new
			sessionId = generateSessionId();
			sessions[sessionId].lastActive = time(NULL);
			sessions[sessionId].visitCount = 1;
			sessions[sessionId].username = "";
			_response.setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly");
		}
	}
	else
	{
		// New session
		sessionId = generateSessionId();
		sessions[sessionId].lastActive = time(NULL);
		sessions[sessionId].visitCount = 1;
		sessions[sessionId].username = "";
		_response.setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly");
	}
	_sessionId = sessionId;
}

// Accessor(s)
int Client::getSocket() const
{
	return _socket;
}

const std::string &Client::getClientIp() const
{
	return _clientIp;
}

bool Client::hasTimedOut(time_t idleTimeout, time_t processingTimeout) const
{
	time_t timeout;
	// Use longer timeout during processing to allow CGI scripts to complete
	if (_state == STATE_PROCESSING)
		timeout = processingTimeout;
	else
		timeout = idleTimeout;
	return time(NULL) - _lastActivity > timeout;
}

void Client::updateActivity()
{
	_lastActivity = time(NULL);
}

const HttpRequest &Client::getRequest() const
{
	return _request;
}

int Client::getResponseStatus()
{
	return _response.getStatus();
}

size_t Client::getResponseBodySize() const
{
	return _response.getBody().size();
}

bool Client::isRequestComplete() const
{
	return _requestComplete;
}

bool Client::isResponseReady() const
{
	return _responseReady;
}

bool Client::shouldCloseAfterResponse() const
{
	return _closeAfterResponse;
}

void Client::markCloseAfterResponse()
{
	_response.setHeader("Connection", "close");
	_closeAfterResponse = true;
}

void Client::setState(ClientState state)
{
	_state = state;
}

CGIProcess *Client::getCGIProcess() const
{
	return _cgiProcess;
}

void Client::setCGIProcess(CGIProcess *cgi)
{
	_cgiProcess = cgi;
}

void Client::setCGITiming(const ServerConfig &config)
{
	gettimeofday(&_cgiStartTime, NULL);
	_serverConfig = &config;
}

// Public method(s)
bool Client::readData()
{
	// Read data from socket into buffer and parse request
	char buffer[4096];
	int bytesRead = recv(_socket, buffer, sizeof(buffer), 0);
	if (bytesRead <= 0)
		return false;
	// Append only the new data to the request
	std::string newData(buffer, bytesRead);
	_request.appendData(newData);
	if (_request.isComplete())
	{
		_requestComplete = true;
		if (_request.getErrorCode())
		{
			buildErrorResponse(_request.getErrorCode());
			markCloseAfterResponse();
			_responseReady = true;
		}
	}
	updateActivity();
	return true;
}

void Client::buildErrorResponse(int statusCode)
{
	_response.setStatus(statusCode);
	_response.setHeader("Content-Type", "text/html");
	std::string errorPage = "www/error_pages/" + intToString(statusCode) + ".html";
	if (fileExists(errorPage))
	{
		try
		{
			_response.setBody(readFile(errorPage));
		}
		catch (const std::exception &e)
		{
			std::cerr << "[Client] buildErrorResponse: " << e.what() << std::endl;
			_response.setBody("<html><body><h1>" + intToString(statusCode) + " Error</h1></body></html>");
		}
	}
	else
		_response.setBody("<html><body><h1>" + intToString(statusCode) + " Error</h1></body></html>");
}

void Client::buildResponse(const ServerConfig &config, Router &router, std::map<std::string, SessionData> &sessions)
{
	// Timer
	struct timeval start, end;
	gettimeofday(&start, NULL);

	// Match route to get location-specific settings
	RouteMatch match = router.matchRoute(config, _request);

	// Check body size limit (use location limit if set, otherwise server limit)
	size_t maxBodySize = config.getMaxBodySize();
	if (match.location && match.location->getMaxBodySize() > 0)
		maxBodySize = match.location->getMaxBodySize();

	if (_request.getBody().size() > maxBodySize)
	{
		buildErrorResponse(413);
		markCloseAfterResponse();
		// log + return
		gettimeofday(&end, NULL);
		double responseTime = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) / 1000.0;
		Logger::logRequest(_request.getMethod(), _request.getUri(), _clientIp, _response.getStatus(), _response.getBody().size(), responseTime, config.getServerName(), config.getPort());
		_responseReady = true;
		return;
	}

	handleSession(sessions);

	if (_request.getUri() == "/counter-api")
	{
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
		return;
	}

	// Handle redirections (reuse match from above)
	if (!match.redirectUrl.empty())
	{
		_response.setStatus(match.statusCode);
		_response.setHeader("Location", match.redirectUrl);
		_response.setBody("");
	}

	// Handle errors (e.g., 403 traversal attempts) before dereferencing location data
	else if (match.statusCode != 200)
		_response.serveError(match.statusCode, "");

	// Handle DELETE request
	else if (_request.getMethod() == "DELETE")
		_response.serveDelete(match.filePath, match.location->getUploadPath());

	// Handle file upload (POST with uploaded files)
	else if (_request.getMethod() == "POST" && !_request.getUploadedFiles().empty())
		_response.handleUpload(_request, match.location->getUploadPath());

	// Handle simple POST without files (return 200 OK)
	else if (_request.getMethod() == "POST")
	{
		_response.setStatus(200);
		_response.setHeader("Content-Type", "text/plain");
		_response.setBody("OK");
	}

	// Serve directory listing if autoindex is enabled
	else if (isDirectory(match.filePath) && match.location->getAutoIndex())
		_response.serveDirectoryListing(match.filePath, _request.getUri());

	// Serve static file
	else
		_response.serveFile(match.filePath, match.location->getRoot());

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
		config.getPort());
	_responseReady = true;
}

void Client::buildResponseFromCGI(const CGIResult &result)
{
	if (result.statusCode == 200)
	{
		_response.setStatus(200);
		_response.setHeader("Content-Type", result.contentType);
		_response.setBody(result.output);
	}
	else
		_response.serveError(result.statusCode, "");

	// Log CGI requests
	if (_serverConfig)
	{
		struct timeval end;
		gettimeofday(&end, NULL);
		double responseTime = (end.tv_sec - _cgiStartTime.tv_sec) * 1000.0 +
							  (end.tv_usec - _cgiStartTime.tv_usec) / 1000.0;

		Logger::logRequest(
			_request.getMethod(),
			_request.getUri(),
			_clientIp,
			_response.getStatus(),
			_response.getBody().size(),
			responseTime,
			_serverConfig->getServerName(),
			_serverConfig->getPort());
	}

	_responseReady = true;
}

bool Client::sendResponse()
{
	// Build response only once and cache it
	if (_cachedResponse.empty())
	{
		_cachedResponse = _response.build(_request.getMethod());
		_bytesSent = 0;
	}

	// Send remaining data
	size_t remaining = _cachedResponse.size() - _bytesSent;
	while (remaining)
	{
		ssize_t sent = send(_socket, _cachedResponse.data() + _bytesSent, remaining, 0);
		if (sent < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				return false; // Not done yet, will retry on next POLLOUT
			std::cerr << "[Client] sendResponse: send failed on fd " << _socket << " errno=" << errno
					  << " (" << strerror(errno) << ")" << std::endl;
			return false;
		}
		if (!sent)
			break; // Connection closed by peer

		_bytesSent += sent;
		remaining -= sent;
	}

	if (!remaining)
	{
		_cachedResponse.clear(); // Free memory
		_bytesSent = 0;
		return true;
	}

	return false; // Not complete yet
}
