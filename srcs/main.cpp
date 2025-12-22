/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:15:35 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/22 16:15:57 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <string>
// #include "server/Server.hpp"
#include "config/Config.hpp"

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

	std::cout << MAGENTA "\n=================== TEST Config::parse ==================\n" RESET;
	try {
		Config config;
		if (!config.parse("config/default.conf"))
		{
			std::cerr << "Error: Failed to parse config\n";
			return 1;
		}
		const std::vector<ServerConfig>& servers = config.getServers();
		for (size_t i = 0; i < servers.size(); i++)
		{
			const ServerConfig& srv = servers[i];
			std::cout << "SERVER #" << (i + 1) << ":\n";
			std::cout << "Port:\t\t" << srv.getPort() << "\n";
			std::cout << "Name:\t\t" << srv.getServerName() << "\n";
			std::cout << "Max body:\t" << srv.getMaxBodySize() << " bytes\n";
			const std::vector<Location>& locs = srv.getLocations();
			std::cout << "Locations:\t";
			for (size_t j = 0; j < locs.size(); j++)
			{
				const Location& loc = locs[j];
				std::cout << std::left;
				std::cout << j + 1 << ". Path: " << std::setw(12) << loc.getPath();
				std::cout << "Root: " << std::setw(12) << loc.getRoot();
				const std::vector<std::string>& methods = loc.getAllowedMethods();
				std::cout << "Methods: ";
				for (size_t k = 0; k < methods.size(); k++)
					std::cout << methods[k] << " ";
				std::cout << "\n\t\t";
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "❌ Error: " << e.what() << "\n";
		return 1;
	}

	// std::cout	<< "\n======================= TEST trim =======================\n" RESET;
	// std::cout	<< "[" << trim("     hello      ") << "]\n";
	// std::cout	<< "[" << trim("\t\nhello\n") << "]\n";
	// std::cout	<< "[" << trim("        ") << "]\n";
	// std::cout	<< "[" << trim("") << "]\n";

	// std::cout	<< "\n======================= TEST split ======================\n" RESET;
	// std::vector<std::string> parts = split("GET /index.html HTTP/1.1", ' ');
	// for (size_t i = 0; i < parts.size(); i++)
	// 	std::cout << i << ": [" << parts[i] << "]\n";

	std::cout	<< MAGENTA "\n====================TEST fileExists ====================\n" RESET;
	std::cout	<< "config/default.conf exists? "
				<< (fileExists("config/default.conf") ? "YES" : "NO")
				<< std::endl;
	std::cout	<< "fake.txt exists? "
				<< (fileExists("fake.txt") ? "YES" : "NO")
				<< std::endl;

	// std::cout	<< MAGENTA "\n===================== TEST readFile =====================\n" RESET;
	// std::string content = readFile("config/default.conf");
	// std::cout	<< "File size: " << content.length() << " bytes\n";
	// std::cout	<< "Content:\n" << content << std::endl;

	std::cout	<< MAGENTA "\n==================== TEST isDirectory ===================\n" RESET;
	std::cout	<< "is www/ a directory ? "
				<< (isDirectory("www/") ? "YES" : "NO")
				<< std::endl;
	std::cout	<< "is config/default.conf a directory ? "
				<< (isDirectory("config/default.conf") ? "YES" : "NO")
				<< std::endl;

	std::cout	<< MAGENTA "\n================= TEST getFileExtension =================\n" RESET;
	std::cout	<< "index.html -> [" << getFileExtension("index.html") << "]\n";
	std::cout	<< "README -> [" << getFileExtension("README") << "]\n";
	std::cout	<< "script.php -> [" << getFileExtension("script.php") << "]\n";

	std::cout	<< MAGENTA "\n==================== TEST getFileSize ===================\n" RESET;
	std::cout	<< "Size of index.html: "
				<< getFileSize("www/index.html")
				<< " bytes" << std::endl;

	std::cout	<< MAGENTA "\n===================== TEST urlDecode ====================\n" RESET;
	std::cout	<< "[" << urlDecode("hello%20world") << "]\n";
	std::cout	<< "[" << urlDecode("test%2Fpath") << "]\n";
	std::cout	<< "[" << urlDecode("hello+world") << "]\n";
	std::cout	<< "[" << urlDecode("caf%C3%A9") << "]\n";

	std::cout	<< MAGENTA "\n==================== TEST getHttpDate ===================\n" RESET;
	std::cout	<< getHttpDate() << std::endl;

	return 0;
}
