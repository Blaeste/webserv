/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lmarck <lmarck@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:04 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/02 13:58:25 by lmarck           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "CGI.hpp"
#include "../http/HttpRequest.hpp"
#include "../server/Router.hpp"
#include "../utils/utils.hpp"
#include <climits>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <cerrno>

// Private method(s)
std::string CGI::readFromPipe(int fd) {
	char buffer[4096];
	std::string result;
	ssize_t bytesRead;
	while ((bytesRead = read(fd, buffer, sizeof(buffer))) > 0)
		result.append(buffer, bytesRead);
	if (bytesRead < 0)
		std::cerr << "[CGI] readFromPipe: read failed" << std::endl;
	return result;
}

void CGI::setupEnvironment(const RouteMatch& match, const HttpRequest& request) {
	std::string uri = request.getUri();
	size_t pos = uri.find('?');

	// Basic CGI variables
	_env["REQUEST_METHOD"] = request.getMethod();

	// Convert script path to absolute path for PHP-CGI
	char absolutePath[PATH_MAX];
	if (realpath(match.filePath.c_str(), absolutePath))
		_env["SCRIPT_FILENAME"] = std::string(absolutePath);
	else
		_env["SCRIPT_FILENAME"] = match.filePath; // Fallback to relative path

	// Query string (part after '?')
	if (pos != std::string::npos)
		_env["QUERY_STRING"] = uri.substr(pos + 1);
	else
		_env["QUERY_STRING"] = "";

	// Script name (URI path without query string)
	std::string scriptName = uri;
	if (pos != std::string::npos)
		scriptName = uri.substr(0, pos);
	_env["SCRIPT_NAME"] = "";

	// PATH_INFO - For 42 tester: use the full URI path (not just relative path)
	_env["PATH_INFO"] = scriptName;
	_env["PATH_TRANSLATED"] = match.filePath;

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

// Public method(s)
void CGI::parseHeaders(const std::string& output, CGIResult& result) {
	// Truncate display for large outputs
	std::string displayOutput = output;
	if (displayOutput.size() > 500)
		displayOutput = displayOutput.substr(0, 500) + "... [truncated, total " + intToString(output.size()) + " bytes]";

	size_t headersEnd = output.find("\r\n\r\n");
	if (headersEnd == std::string::npos) {
		// No headers separator, treat all as body
		std::cout << "[CGI] No \\r\\n\\r\\n found, treating all as body" << std::endl;
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
		if (!line.find("Status: ")) {
			std::string statusLine = line.substr(8); // Skip "Status: "
			// Extract status code (first 3 digits)
			if (statusLine.length() >= 3)
				result.statusCode = parseIntSafe(statusLine.substr(0, 3).c_str(), "CGI status code");
		}

		// Check for Content-Type header
		else if (!line.find("Content-Type: "))
			result.contentType = line.substr(14); // skip "Content-Type: "

		pos = lineEnd + 2;
	}
	result.output = body;
}

CGIProcess* CGI::startAsync(const RouteMatch& match, const HttpRequest& request) {

	setupEnvironment(match, request);

	// Validate interpreter is executable and script is readable
	std::string interpreter = match.location->getCgiPath();
	char absoluteInterpreter[PATH_MAX];
	if (interpreter[0] != '/' && realpath(interpreter.c_str(), absoluteInterpreter))
		interpreter = std::string(absoluteInterpreter);

	if (access(interpreter.c_str(), X_OK) != 0) {
		std::cerr << "[CGI] Interpreter not executable: " << interpreter << std::endl;
		return NULL;
	}

	if (access(match.filePath.c_str(), R_OK) != 0) {
		std::cerr << "[CGI] Script not readable: " << match.filePath << std::endl;
		return NULL;
	}

	CGIProcess* cgi = new CGIProcess();

	// Create pipes for CGI communication
	int pipeOut[2];  // CGI stdout
	int pipeIn[2];   // CGI stdin

	if (pipe(pipeOut) == -1 || pipe(pipeIn) == -1) {
		std::cerr << "[CGI] ASSERT: Failed to create pipes" << std::endl;
		delete cgi;
		return NULL;
	}

	cgi->startTime = time(NULL);
	pid_t pid = fork();

	if (pid == -1) {
		std::cerr << "[CGI] ASSERT: Failed to fork" << std::endl;
		close(pipeOut[0]); close(pipeOut[1]);
		close(pipeIn[0]); close(pipeIn[1]);
		delete cgi;
		return NULL;
	}

	if (!pid) {
		// Child process - execute CGI script
		close(pipeOut[0]);  // Close read end
		close(pipeIn[1]);   // Close write end

		// Redirect stdin/stdout
		dup2(pipeIn[0], STDIN_FILENO);
		dup2(pipeOut[1], STDOUT_FILENO);

		close(pipeIn[0]);
		close(pipeOut[1]);

		// Get CGI interpreter path and convert to absolute BEFORE chdir
		std::string interpreter = match.location->getCgiPath();
		char absoluteInterpreter[PATH_MAX];
		if (interpreter[0] != '/' && realpath(interpreter.c_str(), absoluteInterpreter))
			interpreter = std::string(absoluteInterpreter);

		// Change to script directory for relative path access
		std::string scriptPath = match.filePath;
		std::string scriptName = scriptPath; // Will hold just the filename after chdir
		size_t lastSlash = scriptPath.find_last_of('/');
		if (lastSlash != std::string::npos) {
			std::string scriptDir = scriptPath.substr(0, lastSlash);
			scriptName = scriptPath.substr(lastSlash + 1); // Extract filename only
			if (chdir(scriptDir.c_str()) != 0) {
				std::cerr << "CGI: chdir failed to " << scriptDir << std::endl;
			}
		}

		// Prepare environment variables
		std::vector<char*> envp;
		for (std::map<std::string, std::string>::const_iterator it = _env.begin();
			 it != _env.end(); ++it) {
			std::string envStr = it->first + "=" + it->second;
			envp.push_back(strdup(envStr.c_str()));
		}
		envp.push_back(NULL);

		// Execute CGI with absolute interpreter path
		// Use scriptName (basename) after chdir
		char* argv[] = {
			const_cast<char*>(interpreter.c_str()),
			const_cast<char*>(scriptName.c_str()),
			NULL
		};

		execve(interpreter.c_str(), argv, &envp[0]);

		// If execve fails
		std::cerr << "CGI: execve failed for: " << interpreter << " (errno: " << errno << " - " << strerror(errno) << ")" << std::endl;
		exit(1);
	}

	// Parent process
	close(pipeOut[1]);  // Close write end
	close(pipeIn[0]);   // Close read end

	cgi->pid = pid;
	cgi->pipeOut = pipeOut[0];
	cgi->pipeIn = pipeIn[1];

	// Set pipes to non-blocking mode
	fcntl(cgi->pipeOut, F_SETFL, O_NONBLOCK);
	fcntl(cgi->pipeIn, F_SETFL, O_NONBLOCK);

	// If POST request, mark that we need to write body
	if (request.getMethod() == "POST" && !request.getBody().empty()) {
		cgi->inputWritten = false;
		cgi->bytesWritten = 0;
	} else {
		cgi->inputWritten = true;
		close(cgi->pipeIn);
		cgi->pipeIn = -1;
	}

	return cgi;
}
