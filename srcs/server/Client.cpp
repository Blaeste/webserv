/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:46 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/26 12:54:13 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"
#include "../cgi/CGI.hpp"
#include "../server/Router.hpp"
#include "../utils/MimeTypes.hpp"
#include "../utils/utils.hpp"
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

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
		} else
			buildErrorResponse(result.statusCode);
	} else if (_request.getMethod() == "DELETE") { // DELETE method: remove file
		if (unlink(match.filePath.c_str()) == 0) {
			_response.setStatus(204); // 204 No Content = success
			_response.setBody("");
		} else
			buildErrorResponse(404); // File doesn't exist or permission denied
	} else if (_request.getMethod() == "POST" && !_request.getUploadedFiles().empty()) {
		std::string uploadPath = match.location->getUploadPath();
		const std::vector<UploadedFile>& files = _request.getUploadedFiles();;
		for (size_t i = 0; i < files.size(); i++) {
			const UploadedFile& file = files[i];
			std::string fullPath = match.location->getUploadPath() + "/" + file.filename;
			int fd = open(fullPath.c_str(), O_WRONLY|O_CREAT|O_TRUNC, 0644);
			if (fd < 0)
				continue;
			write(fd, file.content.c_str(), file.content.length());
			close(fd);
		}
		_response.setStatus(201);
		_response.setBody("<html><body><h1>Upload successful</h1><p>" + intToString(files.size()) + " file(s) uploaded</p></body></html>");
	} else { // POST without files or GET
		if (isDirectory(match.filePath) && match.location->getAutoIndex()) { // Generate directory listing
		std::vector<std::string> entries = listDirectory(match.filePath);
		std::string html = "<html><head><title>Index of " + _request.getUri() + "</title></head>";
		html += "<body><h1>Index of " + _request.getUri() + "</h1><hr><ul>";
		for (size_t i = 0; i < entries.size(); i++) {
			html += "<li><a href=\"" + _request.getUri();
			if (_request.getUri()[_request.getUri().length() - 1] != '/')
				html += "/";
			html += entries[i] + "\">" + entries[i] + "</a></li>";
		}
		html += "</ul><hr></body></html>";
		_response.setStatus(200);
		_response.setHeader("Content-Type", "text/html");
		_response.setBody(html);
	} else {
			_response.setStatus(200);
			std::string ext = getFileExtension(match.filePath);
			_response.setHeader("Content-Type", MimeTypes::get(ext));
			_response.setBody(readFile(match.filePath));
		}
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
