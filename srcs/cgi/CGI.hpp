/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   CGI.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:10 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/16 13:38:27 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Fork + exec du script
// Prepare lenvironnement
// Gere les pipes stdin/stdout
// Timeout du CGI

#pragma once

#include <string>
#include <map>
// #include "../http/HttpRequest.hpp"

class CGI
{
private:
	// TODO: std::string _scriptPath;
	// TODO: std::string _cgiPath;
	// TODO: std::map<std::string, std::string> _env;

public:
	CGI(/* args */);
	~CGI();

	// TODO: std::string execute(const HttpRequest &request, const std::string &scriptPath);

private:
	// TODO: void setupEnvironment(const HttpRequest &request);
	// TODO: std::string readFromPipe(int fd);
	// TODO: void writeToPipe(int fd, const std::string &data);
};
