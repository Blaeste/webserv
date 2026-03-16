/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:04 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/16 15:28:02 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------
#include "Cgi.hpp"
#include "../config/Location.hpp"
#include "../http/HttpRequest.hpp"
#include "../server/Router.hpp"
#include "../utils/Logger.hpp"
#include "../utils/utils.hpp"
#include <iostream>					// std::cerr
#include <cctype>					// std::toupper
#include <cerrno>					// errno
#include <cstdlib>					// std::exit, std::getenv
#include <cstring>					// std::strcpy, std::strerror
#include <fcntl.h>					// fcntl, F_SETFL, O_NONBLOCK
#include <unistd.h>					// fork, pipe, close, dup2, access, X_OK, R_OK, chdir, execve, read, STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO

// Static helper(s) ------------------------------------------------------------

static std::string toAbsolutePath(const std::string& path)
{
	if (!path.empty() && path[0] == '/')
		return path;
	const char* cwd = std::getenv("PWD");
	if (cwd)
		return std::string(cwd) + "/" + path;
	return path;
}

// Closes any open pipe ends, skipping fds already set to -1.
static void closePipes(int pipeOut[2], int pipeIn[2], int pipeErr[2])
{
	for (int i = 0; i < 2; i++)
	{
		safeClose(pipeOut[i], "Cgi");
		safeClose(pipeIn[i], "Cgi");
		safeClose(pipeErr[i], "Cgi");
	}
}

// Executes the CGI script in the child process after fork().
// Redirects stdin/stdout/stderr to pipes, closes inherited fds, then execve().
// Never returns on success; calls exit(1) on failure.
static void executeCgiChild(const RouteMatch& match, const std::vector<int>& fdsToClose,
	int pipeIn0, int pipeOut1, int pipeErr1, const envMap& env)
{
	// Redirect stdin/stdout/stderr to pipes
	dup2(pipeIn0, STDIN_FILENO);
	dup2(pipeOut1, STDOUT_FILENO);
	dup2(pipeErr1, STDERR_FILENO);

	safeClose(pipeIn0, "Cgi");
	safeClose(pipeOut1, "Cgi");
	safeClose(pipeErr1, "Cgi");

	// Close all inherited fds from the server
	for (size_t i = 0; i < fdsToClose.size(); i++)
	{
		int fd = fdsToClose[i];
		safeClose(fd, "Cgi");
	}

	// Get CGI interpreter path as absolute BEFORE chdir
	std::string interpreter = toAbsolutePath(match.location->getCgiPath());

	// Change to script directory for relative path access
	std::string scriptName = match.filePath;
	size_t lastSlash = match.filePath.find_last_of('/');
	if (lastSlash != std::string::npos)
	{
		std::string scriptDir = match.filePath.substr(0, lastSlash);
		scriptName = match.filePath.substr(lastSlash + 1);
		if (chdir(scriptDir.c_str()) != 0)
			std::cerr << "chdir failed to " << scriptDir << std::endl;
	}

	// Build environment for execve
	std::vector<std::string> envStrings;
	for (envMap::const_iterator it = env.begin(); it != env.end(); ++it)
		envStrings.push_back(it->first + "=" + it->second);

	std::vector<char*> envp;
	for (size_t i = 0; i < envStrings.size(); ++i)
		envp.push_back(const_cast<char*>(envStrings[i].c_str()));
	envp.push_back(NULL);

	// Execute CGI script
	char* argv[] =
	{
		const_cast<char*>(interpreter.c_str()),
		const_cast<char*>(scriptName.c_str()),
		NULL
	};

	execve(interpreter.c_str(), argv, &envp[0]);

	std::cerr << "execve failed for " << interpreter << ": " << strerror(errno) << std::endl;
	std::exit(1);
}

// Private method(s) -----------------------------------------------------------

void Cgi::setupEnvironment(const RouteMatch& match, const HttpRequest& request)
{
	std::string uri = request.getUri();
	size_t pos = uri.find('?');

	// Basic CGI variables
	_env["REQUEST_METHOD"] = request.getMethod();

	// Convert script path to absolute path for PHP-CGI
	_env["SCRIPT_FILENAME"] = toAbsolutePath(match.filePath);

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
	const headerMap& headers = request.getHeaders();
	for (headerMap::const_iterator it = headers.begin(); it != headers.end(); ++it)
	{
		std::string key = it->first;

		// Skip content-type and content-length (already set above)
		if (key == "content-type" || key == "content-length")
			continue;

		// Transform: user-agent → HTTP_USER_AGENT
		std::string envKey = "HTTP_";
		for (size_t i = 0; i < key.length(); ++i)
		{
			if (key[i] == '-')
				envKey += '_';
			else
				envKey += std::toupper(key[i]);
		}
		_env[envKey] = it->second;
	}
}

// Public method(s) ------------------------------------------------------------

void Cgi::parseHeaders(const std::string& output, CgiResult& result)
{
	size_t headersEnd = output.find("\r\n\r\n");
	if (headersEnd == std::string::npos)
	{
		// No headers separator, treat all as body
		result.output = output;
		return;
	}
	std::string headersBlock = output.substr(0, headersEnd);
	std::string body = output.substr(headersEnd + 4);

	// Parse headers line by line
	size_t pos = 0;
	while (pos < headersBlock.length())
	{
		size_t lineEnd = headersBlock.find("\r\n", pos);
		if (lineEnd == std::string::npos)
			lineEnd = headersBlock.length();
		std::string line = headersBlock.substr(pos, lineEnd - pos);

		// Check for Status header
		if (!line.find("Status: "))
		{
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

CgiProcess* Cgi::startAsync(const RouteMatch& match, const HttpRequest& request, const std::vector<int>& fdsToClose)
{
	setupEnvironment(match, request);

	// Validate interpreter is executable and script is readable
	std::string interpreter = toAbsolutePath(match.location->getCgiPath());

	// Check that the CGI interpreter exists and has execute permission
	if (access(interpreter.c_str(), X_OK) != 0)
	{
		Logger::logMessage(RED "[CGI] Error: " RESET "startAsync: Interpreter not executable: " + interpreter);
		return NULL;
	}

	// Check that the CGI script exists and has read permission
	if (access(match.filePath.c_str(), R_OK) != 0)
	{
		Logger::logMessage(RED "[CGI] Error: " RESET "startAsync: Script not readable: " + match.filePath);
		return NULL;
	}

	// Create pipes for CGI communication
	int pipeOut[2] = {-1, -1};
	int pipeIn[2] = {-1, -1};
	int pipeErr[2] = {-1, -1};

	if (pipe(pipeOut) == -1 || pipe(pipeIn) == -1 || pipe(pipeErr) == -1)
	{
		Logger::logMessage(RED "[CGI] Error: " RESET "startAsync: pipe failed");
		closePipes(pipeOut, pipeIn, pipeErr);
		return NULL;
	}

	CgiProcess* cgi = new CgiProcess();
	cgi->startTime = std::time(NULL);
	pid_t pid = fork();

	if (pid == -1)
	{
		Logger::logMessage(RED "[CGI] Error: " RESET "startAsync: Fork failed");
		closePipes(pipeOut, pipeIn, pipeErr);
		delete cgi;
		return NULL;
	}

	if (!pid)
	{
		// Close unused pipe ends before handing off to child
		safeClose(pipeOut[0], "Cgi");
		safeClose(pipeIn[1], "Cgi");
		safeClose(pipeErr[0], "Cgi");

		executeCgiChild(match, fdsToClose, pipeIn[0], pipeOut[1], pipeErr[1], _env);
	}

	// Parent process
	safeClose(pipeOut[1], "Cgi");	// Close write end
	safeClose(pipeIn[0], "Cgi");	// Close read end
	safeClose(pipeErr[1], "Cgi");	// Close write end

	cgi->pid = pid;
	cgi->pipeOut = pipeOut[0];
	cgi->pipeIn = pipeIn[1];
	cgi->pipeErr = pipeErr[0];

	// Set pipes to non-blocking mode
	fcntl(cgi->pipeOut, F_SETFL, O_NONBLOCK);
	fcntl(cgi->pipeIn, F_SETFL, O_NONBLOCK);
	fcntl(cgi->pipeErr, F_SETFL, O_NONBLOCK);

	// If POST request, mark that we need to write body
	if (request.getMethod() == "POST" && !request.getBody().empty())
	{
		cgi->inputWritten = false;
		cgi->bytesWritten = 0;
	} else {
		cgi->inputWritten = true;
		safeClose(cgi->pipeIn, "Cgi");
		cgi->pipeIn = -1;
	}

	return cgi;
}
