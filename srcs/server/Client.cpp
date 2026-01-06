/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/06 09:43:26 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "Server.hpp"
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

void Client::handleSession(std::map<std::string, SessionData>& sessions) {
    std::map<std::string, std::string> cookies = _request.getCookies();
    std::string sessionId;

    if (cookies.find("session_id") != cookies.end()) {
        sessionId = cookies["session_id"];

        // Update existing session or create new if expired
        if (sessions.find(sessionId) != sessions.end()) {
            sessions[sessionId].lastActive = time(NULL);
            sessions[sessionId].visitCount++;
        } else {
            // Invalid/expired session → create new
            sessionId = generateSessionId();
            sessions[sessionId].lastActive = time(NULL);
            sessions[sessionId].visitCount = 1;
            sessions[sessionId].username = "";
            _response.setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly");
        }
    } else {
        // New session
        sessionId = generateSessionId();
        sessions[sessionId].lastActive = time(NULL);
        sessions[sessionId].visitCount = 1;
        sessions[sessionId].username = "";
        _response.setHeader("Set-Cookie", "session_id=" + sessionId + "; Path=/; HttpOnly");
    }

    _sessionId = sessionId;
}

void Client::serveCounterPage(std::map<std::string, SessionData>& sessions) {
    SessionData& session = sessions[_sessionId];
    std::string html =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "  <title>Visit Counter</title>\n"
        "  <style>\n"
        "    body { font-family: Arial, sans-serif; text-align: center; padding: 50px; background: #f5f5f5; }\n"
        "    .container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); max-width: 500px; margin: 0 auto; }\n"
        "    h1 { color: #333; }\n"
        "    .counter { font-size: 48px; color: #007bff; font-weight: bold; margin: 20px 0; }\n"
        "    .info { color: #666; font-size: 14px; margin-top: 20px; }\n"
        "    a { color: #007bff; text-decoration: none; }\n"
        "    a:hover { text-decoration: underline; }\n"
        "  </style>\n"
        "</head>\n"
        "<body>\n"
        "  <div class=\"container\">\n"
        "    <h1>🎉 Visit Counter</h1>\n"
        "    <div class=\"counter\">" + intToString(session.visitCount) + "</div>\n"
        "    <p>visits to this page</p>\n"
        "    <div class=\"info\">\n"
        "      <p>Session ID: <code>" + _sessionId.substr(0, 16) + "...</code></p>\n"
        "      <p><a href=\"/\">← Back to home</a></p>\n"
        "    </div>\n"
        "  </div>\n"
        "</body>\n"
        "</html>";
    _response.setStatus(200);
    _response.setHeader("Content-Type", "text/html; charset=utf-8");
    _response.setBody(html);
    _responseReady = true;
}

void Client::buildResponse(const ServerConfig& config, Router& router, std::map<std::string, SessionData>& sessions) {
    // Check body size limit
    if (_request.getBody().size() > config.getMaxBodySize()) {
        buildErrorResponse(413);
        _responseReady = true;
        return;
    }

    handleSession(sessions);

    // Special route for visit counter
    if (_request.getUri() == "/counter" || _request.getUri() == "/counter.html") {
        serveCounterPage(sessions);
        return;
    }

    // Continue with normal routing
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
