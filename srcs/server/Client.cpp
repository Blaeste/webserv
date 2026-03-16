/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/16 15:33:54 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------

#include "Client.hpp"
#include "../cgi/Cgi.hpp"
#include "../utils/Logger.hpp"
#include "Router.hpp"
#include "Server.hpp"
#include "../utils/utils.hpp"
#include <stdexcept>			// std::runtime_error
#include <fcntl.h>				// open, O_RDONLY
#include <sys/socket.h>			// recv, send
#include <unistd.h>				// read, close

// Static helper(s) ------------------------------------------------------------

std::string Client::generateSessionId()
{
	const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const size_t idLength = Client::SESSION_ID_LENGTH;
	const size_t charsetSize = sizeof(charset) - 1;

	// Open urandom (regular disk file, exempt from poll() readiness checks)
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		throw std::runtime_error("Failed to open /dev/urandom (generateSessionId)");

	// Read random bytes
	unsigned char randomBytes[idLength];
	if (read(fd, randomBytes, idLength) != (ssize_t)idLength)
	{
		safeClose(fd, "Client");
		throw std::runtime_error("Failed to read from /dev/urandom (generateSessionId)");
	}
	safeClose(fd, "Client");

	// Build session id from random bytes
	std::string id;
	for (size_t i = 0; i < idLength; i++)
		id += charset[randomBytes[i] % charsetSize];
	return id;
}

// Constructor -----------------------------------------------------------------

// Initialize socket and activity timestamp
Client::Client(int socket, const std::string& clientIp)
	: _socket(socket)
	, _clientIp(clientIp)
	, _lastActivity(std::time(NULL))
	, _requestComplete(false)
	, _responseReady(false)
	, _closeAfterResponse(false)
	, _requestLogged(false)
	, _state(STATE_KEEPALIVE)
	, _CgiProcess(NULL)
	, _bytesSent(0)
	, _cgiStartTime(0)
	, _requestStartTime(0)
	, _server(NULL)
{}

// Private method(s) -----------------------------------------------------------

void Client::applyConnectionHeader()
{
	// If already marked for closure, force close header
	if (_closeAfterResponse)
	{
		_response.setHeader("Connection", "close");
		return;
	}

	std::string conn = toLowercase(trim(_request.getHeader("connection")));
	if (_request.getVersion() == "HTTP/1.1")
		_closeAfterResponse = (conn == "close"); // keep-alive by default
	else
		_closeAfterResponse = (conn != "keep-alive"); // HTTP/1.0 => close by default

	_response.setHeader("Connection", _closeAfterResponse ? "close" : "keep-alive");
}

// Handles the /counter-api endpoint: returns session visit count as JSON.
void Client::handleCounterApi(SessionMap& sessions)
{
	SessionData& session = sessions[_sessionId];
	std::string json = "{\"visitCount\":" + intToString(session.visitCount) + ",\"sessionId\":\"" + _sessionId + "\"}";
	_response.setStatus(200);
	_response.setHeader("Content-Type", "application/json");
	_response.setBody(json);
	_responseReady = true;
	applyConnectionHeader();
}

// Dispatches a validated request to the appropriate handler based on method and route.
void Client::dispatchRequest(const ServerBlock& server, const RouteMatch& match)
{
	if (!match.redirectUrl.empty())
	{
		_response.setStatus(match.statusCode);
		_response.setHeader("Location", match.redirectUrl);
		_response.setBody("");
	}
	else if (match.statusCode != 200)
	{
		if (match.statusCode == 405 && match.location)
		{
			buildErrorResponse(match.statusCode, &server);
			std::string allow;
			const stringVector& methods = match.location->getAllowedMethods();
			for (size_t i = 0; i < methods.size(); ++i)
			{
				if (i > 0)
					allow += ", ";
				allow += methods[i];
			}
			_response.setHeader("Allow", allow);
		}
		else
			buildErrorResponse(match.statusCode, &server);
	}
	else if (_request.getMethod() == "OPTIONS")
		_response.serveOptions(match.location->getAllowedMethods());
	else if (_request.getMethod() == "DELETE")
	{
		int status = _response.serveDelete(match.filePath, match.location->getUploadPath());
		if (status >= 400)
			buildErrorResponse(status, &server);
	}
	else if (_request.getMethod() == "POST" && !_request.getUploadedFiles().empty())
	{
		int status = _response.handleUpload(_request, match.location->getUploadPath());
		if (status >= 400)
			buildErrorResponse(status, &server);
	}
	else if (_request.getMethod() == "POST")
	{
		_response.setStatus(200);
		_response.setHeader("Content-Type", "text/plain");
		_response.setBody("OK");
	}
	else if (isDirectory(match.filePath) && match.location->getAutoIndex())
	{
		int status = _response.serveDirectoryListing(match.filePath, _request.getUri());
		if (status >= 400)
			buildErrorResponse(status, &server);
	}
	else
	{
		int status = _response.serveFile(match.filePath, match.location->getRoot());
		if (status >= 400)
			buildErrorResponse(status, &server);
	}
}

void Client::createSession(SessionMap& sessions, const std::string& sessionId)
{
	sessions[sessionId].lastActive = std::time(NULL);
	sessions[sessionId].visitCount = 1;
	sessions[sessionId].username = "";
	_response.setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly");
	_sessionId = sessionId;
}

void Client::handleSession(SessionMap& sessions)
{
	cookieMap cookies = _request.getCookies();

	if (cookies.find("session_id") != cookies.end())
	{
		std::string sessionId = cookies["session_id"];

		if (sessions.find(sessionId) != sessions.end())
		{
			sessions[sessionId].lastActive = std::time(NULL);

			// Only count HTML requests for page visit counter
			std::string uri = _request.getUri();
			bool isInternalRequest = _request.getHeader("X-Internal-Request") == "true";
			bool isHtmlPage = (uri == "/"
				|| uri.find(".html") != std::string::npos
				|| (uri.find('.') == std::string::npos && uri != "/counter-api"));
			if (isHtmlPage && !isInternalRequest)
				sessions[sessionId].visitCount++;
			_sessionId = sessionId;
			return;
		}
	}
	createSession(sessions, generateSessionId()); // New session
}

// Public method(s) ------------------------------------------------------------

bool Client::hasTimedOut(time_t idleTimeout, time_t processingTimeout) const
{
	time_t timeout;
	// Use longer timeout during processing to allow CGI scripts to complete
	if (_state == STATE_PROCESSING)
		timeout = processingTimeout;
	else
		timeout = idleTimeout;
	return std::time(NULL) - _lastActivity > timeout;
}

void Client::markCloseAfterResponse()
{
	_response.setHeader("Connection", "close");
	_closeAfterResponse = true;
}

void Client::setCgiTiming(const ServerBlock& server)
{
	_cgiStartTime = std::time(NULL);
	_server = &server;
}

void Client::logRequestStart(const std::string& serverName, int port)
{
	_logInfo.requestId = _socket;
	_logInfo.method = _request.getMethod();
	_logInfo.uri = _request.getUri();
	_logInfo.clientIP = _clientIp;
	_logInfo.serverName = serverName;
	_logInfo.port = port;

	const bool hasBody = (_logInfo.method == "POST" || _logInfo.method == "PUT" || _logInfo.method == "PATCH");

	if (hasBody && !_request.isChunked())
		_logInfo.declaredSize = _request.getContentLength();
	else
		_logInfo.declaredSize = std::numeric_limits<size_t>::max();

	Logger::requestStart(_logInfo);
	_requestLogged = true;
}

void Client::logRequestEnd()
{
	const bool hasBody = (_logInfo.method == "POST" || _logInfo.method == "PUT" || _logInfo.method == "PATCH");

	_logInfo.statusCode = _response.getStatus();
	_logInfo.requestSize = hasBody ? _request.getBody().size() : std::numeric_limits<size_t>::max();
	_logInfo.responseSize = _response.getBody().size();
	_logInfo.responseTime = std::time(NULL) - _requestStartTime;

	Logger::requestEnd(_logInfo);
}

bool Client::readData(const ServerBlock* server)
{
	// Set start time on very first read (before any parsing)
	if (_requestStartTime == 0)
		_requestStartTime = std::time(NULL);

	// Read data from socket into buffer and parse request
	char buffer[READ_BUFFER_SIZE];
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
			buildErrorResponse(_request.getErrorCode(), server);
			markCloseAfterResponse();
			_responseReady = true;
		}
	}
	updateActivity();
	return true;
}

void Client::buildResponse(const ServerBlock& server, Router& router, SessionMap& sessions)
{
	RouteMatch match = router.matchRoute(server, _request);

	// Check body size limit (use location limit if set, otherwise server limit)
	size_t maxBodySize = server.getMaxBodySize();
	if (match.location && match.location->getMaxBodySize() > 0)
		maxBodySize = match.location->getMaxBodySize();

	if (_request.getBody().size() > maxBodySize)
	{
		buildErrorResponse(413, &server);
		markCloseAfterResponse();
		_responseReady = true;
		applyConnectionHeader();
		return;
	}

	handleSession(sessions);

	if (_request.getUri() == "/counter-api")
	{
		handleCounterApi(sessions);
		return;
	}

	dispatchRequest(server, match);

	_responseReady = true;
	applyConnectionHeader();
}

void Client::buildResponseFromCGI(const CgiResult &result)
{
	if (result.statusCode == 200)
	{
		_response.setStatus(200);
		_response.setHeader("Content-Type", result.contentType);
		_response.setBody(result.output);
	}
	else
		buildErrorResponse(result.statusCode, _server);

	_responseReady = true;
	applyConnectionHeader();
}

void Client::buildErrorResponse(int statusCode, const ServerBlock* server)
{
	_response.setStatus(statusCode);
	_response.setHeader("Content-Type", "text/html");

	std::string errorPage;

	// 1. Check custom error page from server configuration
	if (server)
	{
		std::string customPath = server->getErrorPage(statusCode);
		if (!customPath.empty())
		{
			// Resolve against the root of the first location (typically "/")
			const locationVector& locations = server->getLocations();
			for (size_t i = 0; i < locations.size(); ++i)
			{
				if (locations[i].getPath() == "/")
				{
					std::string candidate = joinPath(locations[i].getRoot(), customPath);
					if (isPathSafe(candidate, locations[i].getRoot()))
						errorPage = candidate;
					else
						Logger::logMessage(RED "[Client] Error: " RESET "buildErrorResponse: error page path traversal blocked: " + candidate);
					break;
				}
			}
			// If no "/" location found, try with customPath as-is (relative)
			if (errorPage.empty() && !customPath.empty() && customPath.find("..") == std::string::npos)
				errorPage = joinPath(".", customPath);
		}
	}

	// 2. Fallback to default hardcoded path
	if (errorPage.empty())
		errorPage = "www/error_pages/" + intToString(statusCode) + ".html";

	if (fileExists(errorPage))
	{
		try
		{
			_response.setBody(readFile(errorPage, "Client"));
		}
		catch (const std::exception& e)
		{
			Logger::logMessage(RED "[Client] Error: " RESET "buildErrorResponse: readFile failed: " + std::string(e.what()));
			_response.setBody("<html><body><h1>" + intToString(statusCode) + " Error</h1></body></html>");
		}
	}
	else
		_response.setBody("<html><body><h1>" + intToString(statusCode) + " Error</h1></body></html>");

	applyConnectionHeader();
}

bool Client::sendResponse()
{
	// Build response only once and cache it
	if (_cachedResponse.empty())
	{
		_cachedResponse = _response.build(_request.getMethod());
		_bytesSent = 0;
	}

	// Send one chunk per call — poll(POLLOUT) will call us again when ready
	size_t remaining = _cachedResponse.size() - _bytesSent;
	if (!remaining)
	{
		_cachedResponse.clear();
		_bytesSent = 0;
		return true;
	}

	ssize_t sent = send(_socket, _cachedResponse.data() + _bytesSent, remaining, 0);
	if (sent <= 0)
	{
		if (sent < 0)
			Logger::logMessage(RED "[Client] Error: " RESET "sendResponse: send failed on fd " + intToString(_socket));
		markCloseAfterResponse();
		return false;
	}

	_bytesSent += sent;
	if (_bytesSent >= _cachedResponse.size())
	{
		_cachedResponse.clear();
		_bytesSent = 0;
		return true;
	}

	return false; // More data to send, wait for next POLLOUT
}

void Client::resetForNextRequest()
{
	// Save connection-level state and leftover before full reset
	const int socket = _socket;
	const std::string clientIp = _clientIp;
	const std::string leftover = _request.getLeftover();

	// Full reset via constructor
	*this = Client(socket, clientIp);

	// Re-inject already received bytes for next request
	if (!leftover.empty())
	{
		_request.appendData(leftover);
		if (_request.isComplete())
			_requestComplete = true;
		_requestStartTime = std::time(NULL);
	}
}
