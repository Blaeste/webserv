/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:15:35 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/10 12:53:21 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------

#include "config/ConfigParser.hpp"
#include "server/Server.hpp"
#include <iostream>				// std::cout, std::cerr

// Define(s) -------------------------------------------------------------------

#define MAGENTA	"\033[35m"
#define RESET	"\033[0m"

int main(int argc, char** argv)
{
	try {
		// Check if have rigth arguments number or go with default config
		if (argc > 2)
		{
			std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
			return 1;
		}

		std::string configPath = (argc == 2) ? argv[1] : "config/default.conf";

		std::cout << MAGENTA "[ Welcome to webserv ]" RESET << std::endl;
		ConfigParser configParser;
		if (!configParser.parse(configPath))
		{
			std::cerr << "Error: Failed to parse configuration file" << std::endl;
			return 1;
		}
		Server server(configParser);
		server.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "[main] Fatal error: " << e.what() << std::endl;
		return 1;
	}
}
