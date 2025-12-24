/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:46 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/24 20:59:21 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "../cgi/CGI.hpp"
#include "../server/Router.hpp"
#include "../utils/MimeTypes.hpp"
#include "../utils/utils.hpp"
#include <netinet/in.h>
#include <sys/socket.h>

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

bool Client::isRequestComplete() const {
	return _requestComplete;
}

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
	RouteMatch match = router.matchRoute(config, _request);
	if (!match.redirectUrl.empty()) {
		_response.setStatus(match.statusCode);
		_response.setHeader("Location", match.redirectUrl);
		_response.setBody("");
	} else if (match.statusCode == 405)
		buildErrorResponse(405);
	else if (match.statusCode == 404)
		buildErrorResponse(404);
	else if (match.isCGI) {
		CGI cgi;
		CGIResult result = cgi.execute(match, _request);
		if (result.statusCode == 200) {
			_response.setStatus(200);
			_response.setBody(result.output);
		} else {
			buildErrorResponse(result.statusCode);
		}
	} else {
		_response.setStatus(200);
		std::string ext = getFileExtension(match.filePath);
		_response.setHeader("Content-Type", MimeTypes::get(ext));
		_response.setBody(readFile(match.filePath));
	}
	_responseReady = true;
}

bool Client::sendResponse() {
	std::string rawResponse = _response.build();
	int bytesSent = send(_socket, rawResponse.c_str(), rawResponse.length(), 0);
	if (bytesSent < 0)
		return false;
	return true;
}
