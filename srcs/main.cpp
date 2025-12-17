/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:15:35 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/17 12:27:14 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <string>
// #include "server/Server.hpp"
// #include "config/Config.hpp"

#include "utils/utils.hpp"

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
	std::cout << "Webserv starting...\n";



// ========================================================================== //
//                                 TESTS                                      //
// ========================================================================== //

	std::cout	<< "\n\n"; // TEST trim ========================================

	std::cout	<< "TEST trim ==============================================\n";
	std::cout	<< "[" << trim("     hello      ") << "]\n";
	std::cout	<< "[" << trim("\t\nhello\n") << "]\n";
	std::cout	<< "[" << trim("        ") << "]\n";
	std::cout	<< "[" << trim("") << "]\n";

	std::cout	<< "\n\n"; // TEST split =======================================

	std::cout	<< "TEST split =============================================\n";
	std::vector<std::string> parts = split("GET /index.html HTTP/1.1", ' ');
	for (size_t i = 0; i < parts.size(); i++)
		std::cout << i << ": [" << parts[i] << "]\n";


	std::cout	<< "\n\n"; // TEST fileExists ==================================

	std::cout	<< "TEST fileExists ========================================\n";
	std::cout	<< "config/default.conf exists? "
				<< (fileExists("config/default.conf") ? "YES" : "NO")
				<< std::endl;

	std::cout	<< "fake.txt exists? "
				<< (fileExists("fake.txt") ? "YES" : "NO")
				<< std::endl;


	std::cout	<< "\n\n"; // TEST readFile ====================================

	std::cout	<< "TEST readFile ==========================================\n";
	std::string content = readFile("config/default.conf");
	std::cout	<< "File size: " << content.length() << " bytes\n";
	std::cout	<< "Content:\n" << content << std::endl;


	std::cout	<< "\n\n"; // TEST isDirectory =================================

	std::cout	<< "TEST isdirectory =======================================\n";
	std::cout	<< "www/ is dire ? "
				<< (isDirectory("www/") ? "YES" : "NO")
				<< std::endl;

	std::cout	<< "config/default.conf is dire ? "
				<< (isDirectory("config/default.conf") ? "YES" : "NO")
				<< std::endl;


	std::cout	<< "\n\n"; // TEST getFileExtension ============================

	std::cout	<< "TEST getFileExtension ==================================\n";
	std::cout	<< "index.html -> [" << getFileExtension("index.html") << "]\n";
	std::cout	<< "README -> [" << getFileExtension("README") << "]\n";
	std::cout	<< "script.php -> [" << getFileExtension("script.php") << "]\n";

	std::cout	<< "\n\n"; // TEST getFileSize =================================

	std::cout	<< "TEST getFileSize =======================================\n";
	std::cout	<< "Size of index.html: "
				<< getFileSize("www/index.html")
				<< " bytes\n";


	std::cout	<< "\n\n"; // TEST urlDecode ===================================

	std::cout	<< "TEST urlDecode =========================================\n";
	std::cout	<< "[" << urlDecode("hello%20world") << "]\n";
	std::cout	<< "[" << urlDecode("test%2Fpath") << "]\n";
	std::cout	<< "[" << urlDecode("hello+world") << "]\n";
	std::cout	<< "[" << urlDecode("caf%C3%A9") << "]\n";

	std::cout	<< "\n\n"; // TEST getHttpDate =================================

	std::cout	<< "TEST getHttpDate =======================================\n";
	std::cout	<< getHttpDate() << std::endl;

	return 0;
}
