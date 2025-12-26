/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:15:35 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/26 16:08:39 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <string>
#include "server/Server.hpp"
#include "config/Config.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"
// #include "utils/utils.hpp" // Parsing test
// #include <iomanip> // Parsing test

#define MAGENTA	"\033[35m"
#define RESET	"\033[0m"

int main(int argc, char **argv)
{
	// TODO: Parse arguments (config file path or default)
	// TODO: Load and parse configuration file
	// TODO: Create server instance with config
	// TODO: Setup signal handlers (SIGINT, SIGTERM)
	// TODO: Run server loop
	// TODO: Cleanup on exit

	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
		return 1;
	}

	// std::cout << MAGENTA "[ Parsing test ]" RESET << std::endl;;
	// try {
	// 	Config config;
	// 	if (!config.parse(argv[1]))
	// 	{
	// 		std::cerr << "Error: Failed to parse config" << std::endl;
	// 		return 1;
	// 	}
	// 	const std::vector<ServerConfig>& servers = config.getServers();
	// 	for (size_t i = 0; i < servers.size(); i++)
	// 	{
	// 		const ServerConfig& srv = servers[i];
	// 		std::cout
	// 			<< std::left << "SERVER #" << (i + 1) << std::endl
	// 			<< "Port:\t\t" << srv.getPort() << std::endl
	// 			<< "Name:\t\t" << srv.getServerName() << std::endl
	// 			<< "Max body:\t" << srv.getMaxBodySize() << " bytes" << std::endl;
	// 		const std::vector<Location>& locs = srv.getLocations();
	// 		for (size_t j = 0; j < locs.size(); j++)
	// 		{
	// 			const Location& loc = locs[j];
	// 			std::cout
	// 				<< "Location #" << j + 1 << ":\t"
	// 				<< "Path: " << std::setw(14) << loc.getPath() << " "
	// 				<< "Root: " << std::setw(12) << loc.getRoot() << " ";
	// 			const std::vector<std::string>& methods = loc.getAllowedMethods();
	// 			std::cout << "Methods:";
	// 			for (size_t k = 0; k < methods.size(); k++)
	// 				std::cout << " " << methods[k];
	// 			std::cout << std::endl;
	// 		}
	// 		std::cout << std::endl;
	// 	}
	// }
	// catch (const std::exception& e)
	// {
	// 	std::cerr << "❌ Error: " << e.what() << std::endl;
	// 	return 1;
	// }

	std::cout << MAGENTA "[ Welcome to webserv ]" RESET << std::endl;
	try {
        Config config;
        if (!config.parse(argv[1]))
        {
            std::cerr << "Error: Failed to parse configuration file" << std::endl;
            return 1;
        }
        Server server;
        server.init(config);
        server.run();
    }
	catch (const std::exception& e)
	{
		std::cerr << "❌ Error: " << e.what() << std::endl;
		return 1;
	}
}
