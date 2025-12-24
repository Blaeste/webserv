/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:04 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/24 19:49:14 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGI.hpp"
#include "../utils/utils.hpp"
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

void CGI::setupEnvironment(const RouteMatch& match, const HttpRequest& request) {
	std::string uri = request.getUri();
	size_t pos = uri.find('?');
	// Basic CGI variables
	_env["REQUEST_METHOD"] = request.getMethod();
	_env["SCRIPT_FILENAME"] = match.filePath;
	// Query string
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

std::string CGI::execute(const RouteMatch& match, const HttpRequest& request) {
	setupEnvironment(match, request);
	int pipeIn[2];  // stdin for CGI
	int pipeOut[2]; // stdout from CGI
	if (pipe(pipeIn) < 0 || pipe(pipeOut) < 0)
		return "";
	pid_t pid = fork();
	if (pid < 0)
		return "";
	if (!pid) {
		// Child process: CGI execution
		dup2(pipeOut[1], 1);  // stdout -> pipeOut[1]
		dup2(pipeIn[0], 0);   // stdin <- pipeIn[0]
		close(pipeOut[0]);
		close(pipeIn[1]);
		char* argv[3];
		argv[0] = (char *)match.location->getCgiPath().c_str();
		argv[1] = (char *)match.filePath.c_str();
		argv[2] = NULL;
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
	// Parent: write POST body to stdin, read output from stdout
	close(pipeOut[1]);
	close(pipeIn[0]);
	if (request.getMethod() == "POST") {
		const std::string& body = request.getBody();
		write(pipeIn[1], body.c_str(), body.length());
	}
	close(pipeIn[1]);  // Close to send EOF to CGI
	std::string output = readFromPipe(pipeOut[0]);
	close(pipeOut[0]);
	waitpid(pid, NULL, 0);
	return output;
}
