/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:15:35 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/20 09:59:28 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "server/Server.hpp"
#include "config/Config.hpp"
#include <cstdlib>
#include <iostream>
// #include <string>			// Parsing test
// #include "utils/utils.hpp"	// Parsing test
// #include <iomanip>			// Parsing test

// Define(s)
#define MAGENTA	"\033[35m"
#define RESET	"\033[0m"

int main(int argc, char **argv)
{
	try {
		// Seed the random number generator for generateSessionId()
		std::srand(static_cast<unsigned int>(std::time(0)));

		// Check if have rigth arguments number or go with default config
		if (argc > 2) {
			std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
			return 1;
		}

		std::string configPath = (argc == 2) ? argv[1] : "config/default.conf";

		std::cout << MAGENTA "[ Welcome to webserv ]" RESET << std::endl;
		Config config;
		if (!config.parse(configPath))
		{
			std::cerr << "Error: Failed to parse configuration file" << std::endl;
			return 1;
		}
		Server server(config);
		server.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "[main] Fatal error: " << e.what() << std::endl;
		return 1;
	}
}
