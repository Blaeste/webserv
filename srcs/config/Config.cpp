/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:11 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/15 13:57:03 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "Config.hpp"
#include "../utils/utils.hpp"
#include <iostream>
#include <stdexcept>
#include <cstdlib>

// Private method(s)

std::string Config::removeComments(const std::string &content) {

	std::string result;
	result.reserve(content.size());
	std::vector<std::string> lines = splitTokens(content, '\n');

	for (size_t i = 0; i < lines.size(); i++) {

		std::string line = lines[i];

		// find # pos
		size_t commentPos = line.find('#');

		if (commentPos != std::string::npos) {

			// Keep only part befor #
			line = line.substr(0, commentPos);
		}

		// Add line to result
		result += line + "\n";
	}

	return result;
}

std::vector<BlockInfo> Config::extractBlocks(const std::string &content, const std::string &keyword) {

	std::vector<BlockInfo> blocks;
	blocks.reserve(8); // classical prealloc
	size_t pos = 0;
	size_t keywordLen = keyword.length();

	while (pos < content.length()) {
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
		if (keyword != "location") {
			std::string between = content.substr(keywordPos + keywordLen, openBracePos - (keywordPos + keywordLen));
			if (between.find_first_not_of(" \t\n\r") != std::string::npos) {
				pos = keywordPos + keywordLen;
				continue;
			}
		}

		// Check if keyword have whitespace before it
		if (keywordPos > 0) {
			char before = content[keywordPos - 1];
			if (before != ' ' && before != '\t' && before != '\n' && before != '\r') {
				pos = keywordPos + 1;
				continue;
			}
		}
		// If keywordPos == 0, start of file, valid

		// Count braces to find closing
		int braceCount = 0; // int here because braceCount can be neg
		size_t i = openBracePos;
		size_t closeBrace = std::string::npos;
		while (i < content.length()) {
			if (content[i] == '{')
				braceCount++;
			else if (content[i] == '}') {
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
			throw std::runtime_error("Config: syntax error unclosed '{' for " + keyword + " block");

		// Extract content without keyword or "{}"
		// For location: keep "location /path" for extract the path
		// For server: just the content
		size_t start = (keyword == "location") ? keywordPos : openBracePos + 1;
		size_t length = (keyword == "location") ? (closeBrace - keywordPos + 1 ) : (closeBrace - openBracePos - 1);
		std::string block = content.substr(start, length);

		// Fill structure
		BlockInfo info;
		info.content = block;
		info.startPos = start;
		info.endPos = closeBrace + 1;
		blocks.push_back(info);

		// Continue for next block
		pos = closeBrace + 1;
	}
	return blocks;
}

void Config::parseServerBlock(const std::string &block, ServerConfig &server, size_t serverIndex) {

	// Extract all location blocks first
	std::vector<BlockInfo> locationBlocks = extractBlocks(block, "location");

	// Build cleanBlock by skipping location blocks
	std::string cleanBlock;
	cleanBlock.reserve(block.size());
	size_t lastPos = 0;

	for (size_t i = 0; i < locationBlocks.size(); i++) {

		// Copy until start of the block
		cleanBlock.append(block, lastPos, locationBlocks[i].startPos - lastPos);
		// Jump to next block
		lastPos = locationBlocks[i].endPos;
	}

	// Append remaining content after last location
	if (lastPos < block.size())
		cleanBlock.append(block, lastPos, block.size() - lastPos);

	// Parse directives (listen, server_name, etc.)
	std::vector<std::string> lines = splitTokens(cleanBlock, '\n');
	for (size_t i = 0; i < lines.size(); i++) {

		// trim each line
		std::string line = trim(lines[i]);
		if (line.empty())
			continue;

		// Remove ";" at the end
		if (line[line.length() - 1] == ';')
			line = line.substr(0, line.length() - 1);

		// Extract tokens from the line
		std::vector<std::string> tokens = splitTokens(line, ' ');

		if (tokens.empty())
			continue;

		// Parse directive
		if (tokens[0] == "listen" && tokens.size() >= 2)
			server.setPort(parseIntSafe(tokens[1].c_str(), "Server #" + sizetToString(serverIndex)));
		else if (tokens[0] == "server_name" && tokens.size() >= 2)
			server.setServerName(tokens[1]);
		else if (tokens[0] == "error_page" && tokens.size() >= 3)
			server.addErrorPage(parseIntSafe(tokens[1].c_str(), "Server #" + sizetToString(serverIndex)), tokens[2]);
		else if (tokens[0] == "client_max_body_size" && tokens.size() >= 2)
			server.setMaxBodySize(parseSize(tokens[1], "Server #" + sizetToString(serverIndex)));
	}

	// Parse each location block
	for (size_t i = 0; i < locationBlocks.size(); i++) {
		Location loc;
		parseLocationBlock(locationBlocks[i].content, loc);
		server.addLocation(loc);
	}
}

void Config::parseLocationBlock(const std::string &block, Location &location) {
	// Extract path from header
	size_t openBrace = block.find("{");
	if (openBrace == std::string::npos)
		return;

	std::string header = block.substr(0, openBrace);
	std::vector<std::string> headerTokens = splitTokens(trim(header), ' ');
	std::string path = "/";

	if (headerTokens.size() >= 2)
		path = headerTokens[1];
	location.setPath(path);

	// Extract content between braces
	size_t closeBrace = block.rfind('}');
	if (closeBrace == std::string::npos)
		return;
	std::string content = block.substr(openBrace + 1, closeBrace - openBrace - 1);

	// Parse lines
	std::vector<std::string> lines = splitTokens(content, '\n');
	for (size_t i = 0; i < lines.size(); i++) {
		// trim each line
		std::string line = trim(lines[i]);
		if (line.empty())
			continue;

		// Remove ";" at the end
		if (line[line.length() - 1] == ';')
			line = line.substr(0, line.length() - 1);

		// Extract tokens from the line
		std::vector<std::string> tokens = splitTokens(line, ' ');
		if (tokens.empty())
			continue;

		// Parse directive
		if (tokens[0] == "root" && tokens.size() >= 2)
			location.setRoot(tokens[1]);
		else if (tokens[0] == "upload_path" && tokens.size() >= 2)
			location.setUploadPath(tokens[1]);
		else if (tokens[0] == "cgi_extension" && tokens.size() >= 2)
			location.setCgiExtension(tokens[1]);
		else if (tokens[0] == "cgi_path" && tokens.size() >= 2)
			location.setCgiPath(tokens[1]);
		else if (tokens[0] == "autoindex" && tokens.size() >= 2)
			location.setAutoIndex(tokens[1] == "on" || tokens[1] == "true");
		else if (tokens[0] == "return" && tokens.size() >= 2)
			location.setRedirect(tokens[tokens.size() - 1]);
		else if (tokens[0] == "index" && tokens.size() >= 2)
			for (size_t j = 1; j < tokens.size(); j++)
				location.addIndex(tokens[j]);
		else if (tokens[0] == "allowed_methods" && tokens.size() >= 2)
			for (size_t j = 1; j < tokens.size(); j++)
				location.addAllowedMethod(tokens[j]);
	}
}

size_t Config::parseSize(const std::string &sizeStr, const std::string &context) {

	if (sizeStr.empty())
	{
		std::cerr << "Warning [" << context << "]: Size '" << sizeStr << "' Empty size value\n";
		return 0;
	}

	// Extract all except the last letter
	size_t len = sizeStr.length();
	char lastChar = sizeStr[len - 1];

	// Check if last char is an unit
	std::string numStr = sizeStr;
	size_t multiplier = 1;

	if (lastChar == 'M' || lastChar == 'm') {
		numStr = sizeStr.substr(0, len - 1);
		multiplier = 1024 * 1024;
	} else if (lastChar == 'K' || lastChar == 'k') {
		numStr = sizeStr.substr(0, len - 1);
		multiplier = 1024;
	} else if (lastChar == 'G' || lastChar =='g') {
		numStr = sizeStr.substr(0, len - 1);
		multiplier = 1024 * 1024 * 1024;
	}

	if (numStr.empty())
	{
		std::cerr << "Warning [" << context << "]: Size '" << sizeStr << "' has unit but no number\n";
		return 0;
	}

	for (size_t i = 0; i < numStr.length(); i++) {

		if (numStr[i] < '0' || numStr[i] > '9')
		{
			std::cerr << "Warning [" << context << "]: Size '" << sizeStr << "' contains invalid characters (only digits allowed)\n";
			return 0;
		}
	}

	size_t num = parseIntSafe(numStr.c_str(), context);

	// Check overflow befor multiplication
	if (multiplier > 1 && num > ((size_t)-1) / multiplier)
		throw std::runtime_error("parseSize [" + context + "]: Size '" + sizeStr + "' causes overflow");

	return num * multiplier;
}

bool Config::validate() const {

	std::vector<std::string> errors;

	// Check if there are at least one server
	if (_servers.empty())
		errors.push_back("No server configured");

	// Check each server
	for (size_t i = 0; i < _servers.size(); i++) {

		const ServerConfig &server = _servers[i];
		int port = server.getPort();
		std::string serverName = server.getServerName();

		// Msg prefix
		std::string prefix = "Server " + sizetToString(i) + " (" + serverName + "): ";

		// Check port range
		if (port < 1 || port > 65535)
			errors.push_back(prefix + "Invalid port " + intToString(port));

		// Check for duplicate Port
		for (size_t j = i + 1; j < _servers.size(); j++) {
			if (_servers[j].getPort() ==  port) {
				// Same port ok if different server name
				if (_servers[j].getServerName() ==  server.getServerName())
					errors.push_back(prefix + "Duplicate port " +  intToString(port) + " with same server name");
			}
		}

		// Check size
		size_t maxSize = server.getMaxBodySize();
		if (maxSize == 0)
			errors.push_back(prefix + "Invalid max body size (0 bytes not allowed)");

		// Check locations
		const std::vector<Location> &locations = server.getLocations();
		for (size_t j = 0; j < locations.size(); j++) {

			const Location &loc = locations[j];
			std::string path = loc.getPath();
			std::string locPrefix = prefix + "Location '" + path + "': ";

			// Check Path starts with '/'
			if (path.empty() || path[0] != '/')
				errors.push_back(locPrefix + "Invalid path (must start with /)");

			// Check CGI extension has corresponding CGI path
			if (!loc.getCgiExtension().empty() && loc.getCgiPath().empty())
				errors.push_back(locPrefix + "CGI extension without CGI path");

			// Check HTTP methods are valid
			const std::vector<std::string> &methods = loc.getAllowedMethods();
			for (size_t k = 0; k < methods.size(); k++) {
				const std::string &method = methods[k];
				if (!isValidHttpMethod(method))
					errors.push_back(locPrefix + "Invalid HTTP method '" + method + "'");
			}
		}
	}

	if (!errors.empty()) {

		std::string msg = "Config validation failed:\n";

		for (size_t i = 0; i < errors.size(); i++)
			msg += " - " + errors[i] + "\n";

		throw std::runtime_error(msg);
	}

	return true;
}

// Public method(s)
bool Config::parse(const std::string &filePath) {
	try {

		std::string content = readFile(filePath);
		if (content.empty())
			return false;

		// Remove Comment
		content = removeComments(content);

		std::vector<BlockInfo> serverBlocks = extractBlocks(content, "server");
		for (size_t i = 0; i < serverBlocks.size(); i++) {
			ServerConfig server;
			parseServerBlock(serverBlocks[i].content, server, i);
			_servers.push_back(server);
		}
		return validate();
	} catch (const std::exception& e) {
		std::cerr << "Config::parse error: " << e.what() << std::endl;
		return false;
	}
}

// Getter(s)
const std::vector<ServerConfig> &Config::getServers() const {
	return _servers;
}
