/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:15:35 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/22 17:52:33 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <string>
// #include "server/Server.hpp"
#include "config/Config.hpp"
#include "http/HttpRequest.hpp"

#define MAGENTA	"\033[35m"
#define RESET	"\033[0m"

#include "utils/utils.hpp"
#include <iomanip>

int main(int ac, char **av)
{
	// TODO: Parse arguments (config file path or default)
	// TODO: Load and parse configuration file
	// TODO: Create server instance with config
	// TODO: Setup signal handlers (SIGINT, SIGTERM)
	// TODO: Run server loop
	// TODO: Cleanup on exit

	(void)ac;
	(void)av;
	std::cout << "Webserv starting..." << std::endl;

// ========================================================================== //
//                                 TESTS                                      //
// ========================================================================== //

	std::cout << MAGENTA "\n=================== TEST Config::parse ==================" RESET << std::endl;;
	try {
		Config config;
		if (!config.parse("config/default.conf"))
		{
			std::cerr << "Error: Failed to parse config" << std::endl;
			return 1;
		}
		const std::vector<ServerConfig>& servers = config.getServers();
		for (size_t i = 0; i < servers.size(); i++)
		{
			if (i) std::cout << std::endl;
			const ServerConfig& srv = servers[i];
			std::cout
				<< std::left << "SERVER #" << (i + 1) << std::endl
				<< "Port:\t\t" << srv.getPort() << std::endl
				<< "Name:\t\t" << srv.getServerName() << std::endl
				<< "Max body:\t" << srv.getMaxBodySize() << " bytes" << std::endl;
			const std::vector<Location>& locs = srv.getLocations();
			for (size_t j = 0; j < locs.size(); j++)
			{
				const Location& loc = locs[j];
				std::cout
					<< "Location #" << j + 1 << ":\t"
					<< "Path: " << std::setw(12) << loc.getPath() << " "
					<< "Root: " << std::setw(12) << loc.getRoot() << " ";
				const std::vector<std::string>& methods = loc.getAllowedMethods();
				std::cout << "Methods:";
				for (size_t k = 0; k < methods.size(); k++)
					std::cout << " " << methods[k];
				std::cout << std::endl;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "❌ Error: " << e.what() << std::endl;
		return 1;
	}

	// std::cout
	// 	<< "\n======================= TEST trim =======================\n" RESET << std::endl;
	// 	<< "[" << trim("     hello      ") << "]\n"
	// 	<< "[" << trim("\t\nhello\n") << "]\n"
	// 	<< "[" << trim("        ") << "]\n"
	// 	<< "[" << trim("") << "]\n";

	// std::cout
	// 	<< "\n======================= TEST split ======================\n" RESET << std::endl;;
	// std::vector<std::string> parts = split("GET /index.html HTTP/1.1", ' ');
	// for (size_t i = 0; i < parts.size(); i++)
	// 	std::cout << i << ": [" << parts[i] << "]\n";

	std::cout << MAGENTA "\n==================== TEST fileExists ====================" RESET << std::endl;;
	std::cout << "config/default.conf exists? " << (fileExists("config/default.conf") ? "YES" : "NO") << std::endl;
	std::cout << "fake.txt exists? " << (fileExists("fake.txt") ? "YES" : "NO") << std::endl;

	// std::cout << MAGENTA "\n===================== TEST readFile =====================" RESET << std::endl;;
	// std::string content = readFile("config/default.conf");
	// std::cout
	// 	<< "File size: " << content.length() << " bytes" << std::endl
	// 	<< "Content:\n" << content << std::endl;

	std::cout << MAGENTA "\n==================== TEST isDirectory ===================" RESET << std::endl;;
	std::cout << "is www/ a directory ? " << (isDirectory("www/") ? "YES" : "NO") << std::endl;
	std::cout << "is config/default.conf a directory ? " << (isDirectory("config/default.conf") ? "YES" : "NO") << std::endl;

	std::cout << MAGENTA "\n================= TEST getFileExtension =================" RESET << std::endl;
	std::cout << "index.html -> [" << getFileExtension("index.html") << "]" << std::endl;
	std::cout << "README -> [" << getFileExtension("README") << "]" << std::endl;
	std::cout << "script.php -> [" << getFileExtension("script.php") << "]" << std::endl;

	std::cout << MAGENTA "\n==================== TEST getFileSize ===================" RESET << std::endl;;
	std::cout << "Size of index.html: " << getFileSize("www/index.html") << " bytes" << std::endl;

	std::cout << MAGENTA "\n===================== TEST urlDecode ====================" RESET << std::endl;;
	std::cout << "[" << urlDecode("hello%20world") << "]" << std::endl;
	std::cout << "[" << urlDecode("test%2Fpath") << "]" << std::endl;
	std::cout << "[" << urlDecode("hello+world") << "]" << std::endl;
	std::cout << "[" << urlDecode("caf%C3%A9") << "]" << std::endl;

	std::cout << MAGENTA "\n==================== TEST getHttpDate ===================" RESET << std::endl;;
	std::cout << getHttpDate() << std::endl;

	std::cout << MAGENTA "\n=============== TEST basic GET HttpRequest ==============" RESET << std::endl;;
	std::string rawRequest = 
		"GET /index.html HTTP/1.1\r\n"
		"Host: localhost:8080\r\n"
		"User-Agent: Mozilla/5.0\r\n"
		"\r\n";
	HttpRequest req;
	if (!req.appendData(rawRequest))
	{
		std::cerr << "Error: Failed to parse request" << std::endl;
		return 1;
	}
	if (req.isComplete())
	{
		std::cout << "Method:\t\t" << req.getMethod() << std::endl;
		std::cout << "URI:\t\t" << req.getUri() << std::endl;
		std::cout << "Version:\t" << req.getVersion() << std::endl;
		const std::map<std::string, std::string> &headers = req.getHeaders();
		for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
			std::cout << "Header #" << std::distance(headers.begin(), it) + 1 << ":\t" << it->first << ": " << it->second << std::endl;
	}
}
