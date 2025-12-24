/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:15:35 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/24 17:03:58 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <string>
#include "server/Server.hpp"
#include "config/Config.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

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
					<< "Path: " << std::setw(14) << loc.getPath() << " "
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

	std::cout << std::endl;
	std::cout << MAGENTA "\n============ TEST Server::init + Server::run ============" RESET << std::endl;
	try {
		Config config;
		if (!config.parse("config/default.conf"))
		{
			std::cerr << "Error: Failed to parse config" << std::endl;
			return 1;
		}
		
		std::cout << "[Server initialization]" << std::endl;
		Server server;
		server.init(config);

		std::cout << "\n[Server starting]" << std::endl;
		std::cout << "[Try 'curl http://localhost:8080']" << std::endl;
		server.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "❌ Error: " << e.what() << std::endl;
		return 1;
	}
}
