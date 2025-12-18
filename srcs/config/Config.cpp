/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:11 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/18 14:48:20 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// =============================================================================
// Includes

#include <stdexcept>
#include <cstdlib>

#include "Config.hpp"
#include "../utils/utils.hpp"


// =============================================================================
// Handlers

// Default constructor
Config::Config()
{
}

// Destructor
Config::~Config()
{
}

// =============================================================================
// Getters

const std::vector<ServerConfig> &Config::getServers() const
{
	return _servers;
}

// =============================================================================
// Methods

std::vector<std::string> Config::extractBlocks(const std::string &content, const std::string &keyword)
{
	std::vector<std::string> blocks;
	size_t pos = 0;
	size_t keywordLen = keyword.length();

	while (pos < content.length())
	{
		// Search for keyword
		size_t keywordPos = content.find(keyword, pos);
		if (keywordPos == std::string::npos)
			break; // no more keyword possible

		// Search for "{"
		size_t openBracePos = content.find("{", keywordPos);
		if (openBracePos == std::string::npos)
			break; // Syntax error

		// Check if only whitspace between keyword and "{"
		// For "location", we accept any path (location /api, location /uploads, etc.)
		// For "server", only whitespace is allowed
		if (keyword != "location")
		{
			std::string between = content.substr(keywordPos + keywordLen, openBracePos - (keywordPos + keywordLen));
			if (between.find_first_not_of(" \t\n\r") != std::string::npos)
			{
				pos = keywordPos + keywordLen;
				continue;
			}
		}

		// Check if keyword have whitespace before it
		if (keywordPos > 0)
		{
			char before = content[keywordPos - 1];
			if (before != ' ' && before != '\t' && before != '\n' && before != '\r')
			{
				pos = keywordPos + 1;
				continue;
			}
		}

		// Count braces to find closing
		int braceCount = 0; // int here because braceCount can be neg
		size_t i = openBracePos;
		size_t closeBrace = std::string::npos;

		while (i < content.length())
		{
			if (content[i] == '{')
				braceCount++;
			else if (content[i] == '}')
			{
				braceCount--;
				if (braceCount < 0)
					throw std::runtime_error("Config syntax error: unmatched '}'");
				if (braceCount == 0)
				{
					closeBrace = i;
					break;
				}
			}
			i++;
		}

		if (closeBrace == std::string::npos)
			break; // Error: no closing brace

		// Extract content without keyword or "{}"
		// For location: keep "location /path" for extract the path
		// For server: just the content
		size_t start = (keyword == "location") ? keywordPos : openBracePos + 1;
		size_t length = (keyword == "location") ? (closeBrace - keywordPos + 1 ) : (closeBrace - openBracePos - 1);
		std::string block = content.substr(start, length);
		blocks.push_back(block);

		// Continue for next block
		pos = closeBrace + 1;
	}

	return blocks;
}

void Config::parseServerBlock(const std::string &block, ServerConfig &server)
{

	// Extract all location blocks first
	std::vector<std::string> locationBlocks = extractBlocks(block, "location");

	// Remove location blocks form content to parse simple directives
	std::string cleanBlock = block;
	for (size_t i = 0; i < locationBlocks.size(); i++)
	{
		size_t pos = cleanBlock.find(locationBlocks[i]);
		if (pos != std::string::npos)
			cleanBlock.erase(pos,locationBlocks[i].length());
	}

	// Parse directives (listen, server_name, etc.)
	std::vector<std::string> lines = split(cleanBlock, '\n');
	for (size_t i = 0; i < lines.size(); i++)
	{
		// trim each line
		std::string line = trim(lines[i]);
		if (line.empty())
			continue;

		// Remove ";" at the end
		if (line[line.length() - 1] == ';')
			line = line.substr(0, line.length() - 1);

		// Extract tokens from the line
		std::vector<std::string> tokens = split(line, ' ');
		if (tokens.empty())
			continue;

		// Parse directive
		if (tokens[0] == "listen")
			server.setPort(atoi(tokens[1].c_str()));
		else if (tokens[0] == "server_name")
			server.setServerName(tokens[1]);
		else if (tokens[0] == "error_page")
			server.addErrorPage(atoi(tokens[1].c_str()), tokens[2]);
		else if (tokens[0] == "client_max_body_size")
			server.setMaxBodySize(parseSize(tokens[1]));
	}

	// Parse each location block
	for (size_t i = 0; i < locationBlocks.size(); i++)
	{
		Location loc;
		parseLocationBlock(locationBlocks[i], loc);
		server.addLocation(loc);
	}
}

void Config::parseLocationBlock(const std::string &block, Location &location)
{
	// Extract path
	size_t openBrace = block.find("{");
	if (openBrace == std::string::npos)
		return;

	std::string header = block.substr(0, openBrace);
	std::vector<std::string> headerTokens = split(trim(header), ' ');
	std::string path = "/";

	if (headerTokens.size() >= 2)
		path = headerTokens[1];
	location.setPath(path);

	// Extract content
	size_t closeBrace = block.rfind('}');
	if (closeBrace == std::string::npos)
		return;
	std::string content = block.substr(openBrace + 1, closeBrace - openBrace - 1);

	// Parse lines
	std::vector<std::string> lines = split(content, '\n');
	for (size_t i = 0; i < lines.size(); i++)
	{
		// trim each line
		std::string line = trim(lines[i]);
		if (line.empty())
			continue;

		// Remove ";" at the end
		if (line[line.length() - 1] == ';')
			line = line.substr(0, line.length() - 1);

		// Extract tokens from the line
		std::vector<std::string> tokens = split(line, ' ');
		if (tokens.empty())
			continue;

		// Parse directive
		if (tokens[0] == "root")
			location.setRoot(tokens[1]);
		else if (tokens[0] == "upload_path")
			location.setUploadPath(tokens[1]);
		else if (tokens[0] == "cgi_extension")
			location.setCgiExtension(tokens[1]);
		else if (tokens[0] == "cgi_path")
			location.setCgiPath(tokens[1]);
		else if (tokens[0] == "autoindex")
			location.setAutoIndex(tokens[1] == "on" || tokens[1] == "true");
		else if (tokens[0] == "return" && tokens.size() >= 2)
			location.setRedirect(tokens[tokens.size() - 1]);
		else if (tokens[0] == "index")
			for (size_t j = 1; j < tokens.size(); j++)
				location.addIndex(tokens[j]);
		else if (tokens[0] == "allowed_methods")
			for (size_t j = 1; j < tokens.size(); j++)
				location.addAllowedMethod(tokens[j]);
	}
}

size_t Config::parseSize(const std::string &sizeStr)
{
	if (sizeStr.empty())
		return 0;

	// Extract all except the last letter "M" or "K"
	size_t len = sizeStr.length();
	char lastChar = sizeStr[len - 1];

	// Check if last char is an unit
	std::string numStr = sizeStr;
	size_t multiplier = 1;

	if (lastChar == 'M' || lastChar == 'm')
	{
		numStr = sizeStr.substr(0, len - 1);
		multiplier = 1024 * 1024;
	}
	else if (lastChar == 'K' || lastChar == 'k')
	{
		numStr = sizeStr.substr(0, len - 1);
		multiplier = 1024;
	}

	size_t num = atoi(numStr.c_str());
	return num * multiplier;
}

bool Config::validate() const
{
	// Check if there are at least one server
	if (_servers.empty())
		throw std::runtime_error("Config error: No server configured");

	// Checking on each server
	for (size_t i = 0; i < _servers.size(); i++)
	{
		const ServerConfig &server = _servers[i];

		// Check Port
		int port = server.getPort();
		if (port < 1 || port > 65535)
			throw std::runtime_error("Config error: Invalid port number");

		// Check for duplicate Port
		for (size_t j = i + 1; j < _servers.size(); j++)
		{
			if (_servers[j].getPort() ==  port)
			{
				// Same port ok if different server name
				if (_servers[j].getServerName() ==  server.getServerName())
					throw std::runtime_error("Config error: Duplicate port with same server name");
			}
		}

		// Check locations
		const std::vector<Location> &locations = server.getLocations();
		for (size_t j = 0; j < locations.size(); j++)
		{
			const Location &loc = locations[j];

			// Check Path start with '/'
			std::string path = loc.getPath();
			if (path.empty() || path[0] != '/')
				throw std::runtime_error("Config error: Invalid location path");

			// Check CGI ext / path
			if (!loc.getCgiExtension().empty() && loc.getCgiPath().empty())
				throw std::runtime_error("Config error: CGI extension without CGI path");

			// Check method HTTP
			const std::vector<std::string> &methods = loc.getAllowedMethods();
			for (size_t k = 0; k < methods.size(); k++)
			{
				const std::string &method = methods[k];
				if (method != "GET" && method != "POST" && method != "DELETE" && method != "PUT" && method != "HEAD" && method != "OPTIONS")
					throw std::runtime_error("Config error: Invalid HTTP method");
			}
		}
	}

	return true;
}

bool Config::parse(const std::string &filePath)
{
	// Read file
	std::string content = readFile(filePath);

	if (content.empty())
		return false;

	// Extract server block
	std::vector<std::string> serverBlocks = extractBlocks(content, "server");

	// Parse each server
	for (size_t i = 0; i < serverBlocks.size(); i++)
	{
		ServerConfig server;
		parseServerBlock(serverBlocks[i], server);
		_servers.push_back(server);
	}

	// return after validate()
	return validate();
}

