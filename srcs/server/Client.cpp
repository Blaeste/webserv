/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/08 13:34:34 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------
#include "Client.hpp"
#include "Router.hpp"
#include "Server.hpp"
#include "../utils/Logger.hpp"
#include "../cgi/CGI.hpp"
#include "../utils/utils.hpp"
#include <iostream>				// std::cout
#include <limits>				// std::numeric_limits
#include <sstream>				// std::stringstream
#include <sys/socket.h>			// recv, send

// Constructor -----------------------------------------------------------------
// Initialize socket and activity timestamp
Client::Client(int socket, const std::string &clientIp)
	: _socket(socket), _clientIp(clientIp), _lastActivity(std::time(NULL)), _requestComplete(false), _responseReady(false), _closeAfterResponse(false), _requestLogged(false), _state(STATE_KEEPALIVE), _cgiProcess(NULL), _bytesSent(0), _cgiStartTime(0), _requestStartTime(0), _serverConfig(NULL)
{
}

// Accessor(s) -----------------------------------------------------------------
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

void Client::setCGITiming(const ServerConfig &config)
{
	_cgiStartTime = std::time(NULL);
	_serverConfig = &config;
}

// Public method(s) ------------------------------------------------------------
bool Client::readData(const ServerConfig *config)
{
	// Set start time on very first read (before any parsing)
	if (_requestStartTime == 0)
		_requestStartTime = std::time(NULL);

	// Read data from socket into buffer and parse request
	char buffer[4096];
	int bytesRead = recv(_socket, buffer, sizeof(buffer), 0);
	if (bytesRead <= 0)
		return false;
	// Append only the new data to the request
	std::string newData(buffer, bytesRead);
	_request.appendData(newData);

	// Log request start as soon as headers are parsed
	// Logging moved to Server::handleClientRead once Host header is fully parsed

	if (_request.isComplete())
	{
		_requestComplete = true;
		if (_request.getErrorCode())
		{
			buildErrorResponse(_request.getErrorCode(), config);
			markCloseAfterResponse();
			_responseReady = true;

			// Logging moved to Server::handleClientRead once Host header is fully parsed
		}
	}
	updateActivity();
	return true;
}

void Client::buildResponse(const ServerConfig &config, Router &router, std::map<std::string, SessionData> &sessions)
{
	// Match route to get location-specific settings
	RouteMatch match = router.matchRoute(config, _request);

	// Check body size limit (use location limit if set, otherwise server limit)
	size_t maxBodySize = config.getMaxBodySize();
	if (match.location && match.location->getMaxBodySize() > 0)
		maxBodySize = match.location->getMaxBodySize();

	if (_request.getBody().size() > maxBodySize)
	{
		buildErrorResponse(413, &config);
		markCloseAfterResponse();
		_responseReady = true;
		applyConnectionHeader();
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
		_responseReady = true;
		applyConnectionHeader();
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
	{
		if (match.statusCode == 405 && match.location)
		{
			buildErrorResponse(match.statusCode, &config);
			std::string allow;
			const std::vector<std::string> &methods = match.location->getAllowedMethods();
			for (size_t i = 0; i < methods.size(); ++i)
			{
				if (i > 0)
					allow += ", ";
				allow += methods[i];
			}
			_response.setHeader("Allow", allow);
		}
		else
			buildErrorResponse(match.statusCode, &config);
	}

	// Handle OPTIONS request
	else if (_request.getMethod() == "OPTIONS")
		_response.serveOptions(match.location->getAllowedMethods());

	// Handle DELETE request
	else if (_request.getMethod() == "DELETE")
	{
		int status = _response.serveDelete(match.filePath, match.location->getUploadPath());
		if (status >= 400)
			buildErrorResponse(status, &config);
	}

	// Handle file upload (POST with uploaded files)
	else if (_request.getMethod() == "POST" && !_request.getUploadedFiles().empty())
	{
		int status = _response.handleUpload(_request, match.location->getUploadPath());
		if (status >= 400)
			buildErrorResponse(status, &config);
	}

	// Handle simple POST without files (return 200 OK)
	else if (_request.getMethod() == "POST")
	{
		_response.setStatus(200);
		_response.setHeader("Content-Type", "text/plain");
		_response.setBody("OK");
	}

	// Serve directory listing if autoindex is enabled
	else if (isDirectory(match.filePath) && match.location->getAutoIndex())
	{
		int status = _response.serveDirectoryListing(match.filePath, _request.getUri());
		if (status >= 400)
			buildErrorResponse(status, &config);
	}

	// Serve static file
	else
	{
		int status = _response.serveFile(match.filePath, match.location->getRoot());
		if (status >= 400)
			buildErrorResponse(status, &config);
	}

	_responseReady = true;
	applyConnectionHeader();
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
		buildErrorResponse(result.statusCode, _serverConfig);

	_responseReady = true;
	applyConnectionHeader();
}

void Client::buildErrorResponse(int statusCode, const ServerConfig *config)
{
	_response.setStatus(statusCode);
	_response.setHeader("Content-Type", "text/html");

	std::string errorPage;

	// 1. Check custom error page from server configuration
	if (config)
	{
		std::string customPath = config->getErrorPage(statusCode);
		if (!customPath.empty())
		{
			// Resolve against the root of the first location (typically "/")
			const std::vector<Location> &locations = config->getLocations();
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
			_response.setBody(readFile(errorPage));
		}
		catch (const std::exception &e)
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

void Client::stashLeftoverFromRequest()
{
	_pendingInput = _request.getLeftover();
}

void Client::resetForNextRequest()
{
	_request.reset();
	_response = HttpResponse();
	_requestComplete = false;
	_responseReady = false;
	_closeAfterResponse = false;
	_requestLogged = false;
	_state = STATE_KEEPALIVE;
	_cachedResponse.clear();
	_bytesSent = 0;
	_requestStartTime = 0;

	// Re-inject already received bytes for next request
	std::string tmp = _pendingInput;
	_pendingInput.clear();
	if (!tmp.empty())
	{
		_request.appendData(tmp);
		if (_request.isComplete())
			_requestComplete = true;
		if (_requestStartTime == 0)
			_requestStartTime = std::time(NULL);
	}
}

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

// Private method(s) -----------------------------------------------------------
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
			sessions[sessionId].lastActive = std::time(NULL);

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
			sessions[sessionId].lastActive = std::time(NULL);
			sessions[sessionId].visitCount = 1;
			sessions[sessionId].username = "";
			_response.setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly");
		}
	}
	else
	{
		// New session
		sessionId = generateSessionId();
		sessions[sessionId].lastActive = std::time(NULL);
		sessions[sessionId].visitCount = 1;
		sessions[sessionId].username = "";
		_response.setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly");
	}
	_sessionId = sessionId;
}
