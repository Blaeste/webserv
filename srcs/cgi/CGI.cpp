/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:04 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/24 17:54:27 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "CGI.hpp"
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
	if (pos != std::string::npos)
		_env["QUERY_STRING"] = uri.substr(pos + 1);
	else
		_env["QUERY_STRING"] = "";
	_env["REQUEST_METHOD"] = request.getMethod();
	_env["SCRIPT_FILENAME"] = match.filePath;
}

std::string CGI::execute(const RouteMatch& match, const HttpRequest& request) {
	setupEnvironment(match, request);
	int pipeFd[2];
	if (pipe(pipeFd) < 0)
		return "";
	pid_t pid = fork();
	if (pid < 0)
		return "";
	if (!pid) {
		// Child process: CGI execution
		dup2(pipeFd[1], 1);
		close(pipeFd[0]);
		char* argv[3];
		argv[0] = (char *)match.location->getCgiPath().c_str(); // "/usr/bin/python3"
		argv[1] = (char *)match.filePath.c_str(); // "./cgi-bin/py/test.py"
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
	// Parent: reads the output
	close(pipeFd[1]);
	std::string output = readFromPipe(pipeFd[0]);
	close(pipeFd[0]);
	waitpid(pid, NULL, 0);
	
	return output;
}
