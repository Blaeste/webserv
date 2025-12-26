/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:04 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/26 15:49:50 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGI.hpp"
#include "../utils/utils.hpp"
#include <cstdlib>
#include <ctime>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

CGI::CGI() {}

CGI::~CGI() {}

std::string CGI::readFromPipe(int fd) {
	char buffer[4096];
	std::string result;
	ssize_t bytesRead;
	while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
		result.append(buffer, bytesRead);
	return result;   
}

// Setup CGI environment variables
void CGI::setupEnvironment(const RouteMatch& match, const HttpRequest& request) {
	std::string uri = request.getUri();
	size_t pos = uri.find('?');

	// Basic CGI variables
	_env["REQUEST_METHOD"] = request.getMethod();
	_env["SCRIPT_FILENAME"] = match.filePath;

	// Query string (part after '?')
	if (pos != std::string::npos)
		_env["QUERY_STRING"] = uri.substr(pos + 1);
	else
		_env["QUERY_STRING"] = "";

	// Script name (URI path without query string)
	std::string scriptName = uri;
	if (pos != std::string::npos)
		scriptName = uri.substr(0, pos);
	_env["SCRIPT_NAME"] = scriptName;

	// Server info
	_env["SERVER_NAME"] = match.serverName;
	_env["SERVER_PORT"] = intToString(match.serverPort);
	_env["SERVER_PROTOCOL"] = "HTTP/1.1";
	_env["GATEWAY_INTERFACE"] = "CGI/1.1";

	// Required for PHP-CGI security
	_env["REDIRECT_STATUS"] = "200";

	// Content-Type and Content-Length (for POST)
	std::string contentType = request.getHeader("content-type");
	if (!contentType.empty())
		_env["CONTENT_TYPE"] = contentType;
	std::string contentLength = request.getHeader("content-length");
	if (!contentLength.empty())
		_env["CONTENT_LENGTH"] = contentLength;
	else if (request.getMethod() == "POST")
		_env["CONTENT_LENGTH"] = intToString(request.getBody().size());

	// Pass all HTTP headers as HTTP_* variables
	const std::map<std::string, std::string>& headers = request.getHeaders();
	for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it) {
		std::string key = it->first;

		// Skip content-type and content-length (already set above)
		if (key == "content-type" || key == "content-length")
			continue;

		// Transform: user-agent → HTTP_USER_AGENT
		std::string envKey = "HTTP_";
		for (size_t i = 0; i < key.length(); ++i) {
			if (key[i] == '-')
				envKey += '_';
			else
				envKey += toupper(key[i]);
		}
		_env[envKey] = it->second;
	}
}

CGIResult CGI::execute(const RouteMatch& match, const HttpRequest& request) {
	CGIResult result;
	result.statusCode = 200;
	result.output = "";
	setupEnvironment(match, request);

	// Create pipes for stdin/stdout
	int pipeIn[2];
	int pipeOut[2];
	if (pipe(pipeIn) < 0 || pipe(pipeOut) < 0) {
		result.statusCode = 500;
		return result;
	}
	pid_t pid = fork();
	if (pid < 0) {
		result.statusCode = 500;
		return result;
	}
	if (!pid) {
		// Child process: CGI execution
		dup2(pipeOut[1], 1); // Redirect stdout to pipe
		dup2(pipeIn[0], 0); // Redirect stdin from pipe
		close(pipeOut[0]);
		close(pipeIn[1]);

		// Prepare argv
		char* argv[3];
		argv[0] = (char *)match.location->getCgiPath().c_str();
		argv[1] = (char *)match.filePath.c_str();
		argv[2] = NULL;

		// Prepare environment
		std::vector<std::string> envStrings;
		for (std::map<std::string, std::string>::iterator it = _env.begin(); it != _env.end(); ++it)
			envStrings.push_back(it->first + "=" + it->second);
		std::vector<char *> envp;
		for (size_t i = 0; i < envStrings.size(); i++)
			envp.push_back((char *)envStrings[i].c_str());
		envp.push_back(NULL);

		execve(argv[0], argv, &envp[0]);
		_exit(1);
	}

	// Parent: send POST body, read output with timeout
	close(pipeOut[1]);
	close(pipeIn[0]);

	// Send POST body to CGI stdin if present
	if (request.getMethod() == "POST") {
		const std::string& body = request.getBody();
		write(pipeIn[1], body.c_str(), body.length());
	}
	close(pipeIn[1]);

	// Wait for child with timeout (5 seconds)
	time_t startTime = time(NULL);
	int timeout = 5;
	int status;
	while (true) {
		pid_t wpid = waitpid(pid, &status, WNOHANG);
		if (wpid == pid)
			break; // Child finished
		if (time(NULL) - startTime > timeout) {
			// kill child and return 504 Gateway Timeout
			kill(pid, SIGKILL);
			waitpid(pid, NULL, 0);
			close(pipeOut[0]);
			result.statusCode = 504;
			return result;
		}
		usleep(100000); // Sleep 100ms
	}
	result.output = readFromPipe(pipeOut[0]);
	close(pipeOut[0]);

	// Parse CGI headers (Status, etc.)
	parseHeaders(result.output, result);
	return result;
}

void CGI::parseHeaders(const std::string& output, CGIResult& result) {
	size_t headersEnd = output.find("\r\n\r\n");
	if (headersEnd == std::string::npos) {
		// No headers separator, treat all as body
		result.output = output;
		return;
	}
	std::string headersBlock = output.substr(0, headersEnd);
	std::string body = output.substr(headersEnd + 4);
	
	// Parse headers line by line
	size_t pos = 0;
	while (pos < headersBlock.length()) {
		size_t lineEnd = headersBlock.find("\r\n", pos);
		if (lineEnd == std::string::npos)
			lineEnd = headersBlock.length();
		std::string line = headersBlock.substr(pos, lineEnd - pos);

		// Check for Status header
		if (line.find("Status: ") == 0) {
			std::string statusLine = line.substr(8); // Skip "Status: "
			// Extract status code (first 3 digits)
			if (statusLine.length() >= 3) {
				result.statusCode = atoi(statusLine.substr(0, 3).c_str());
			}
		}
		// Other headers (Content-Type, etc.) are ignored for now

		pos = lineEnd + 2;
	}
	result.output = body;
}
