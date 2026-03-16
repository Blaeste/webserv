/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/09 15:36:08 by gdosch            #+#    #+#             */
/*   Updated: 2026/03/16 15:34:09 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------

#include "ConfigParser.hpp"
#include "../utils/utils.hpp"
#include <iostream>				// std::cerr
#include <set>					// std::set
#include <stdexcept>			// std::runtime_error

// Static helper(s) ------------------------------------------------------------

static bool isValidHttpMethod(const std::string& method)
{
	static std::set<std::string> validMethods;

	if (validMethods.empty())
	{
		validMethods.insert("GET");
		validMethods.insert("POST");
		validMethods.insert("DELETE");
		validMethods.insert("PUT");
		validMethods.insert("HEAD");
		validMethods.insert("OPTIONS");
	}
	return validMethods.find(method) != validMethods.end();
}

static bool isKnownServerDirective(const std::string &directive)
{
	static std::set<std::string> valideDirectives;

	if (valideDirectives.empty())
	{
		valideDirectives.insert("listen");
		valideDirectives.insert("server_name");
		valideDirectives.insert("error_page");
		valideDirectives.insert("client_max_body_size");
		valideDirectives.insert("cgi_timeout");
	}

	return valideDirectives.find(directive) != valideDirectives.end();
}

static bool isKnownLocationDirective(const std::string &directive)
{
	static std::set<std::string> valideDirectives;

	if (valideDirectives.empty())
	{
		valideDirectives.insert("root");
		valideDirectives.insert("upload_path");
		valideDirectives.insert("cgi_extension");
		valideDirectives.insert("cgi_path");
		valideDirectives.insert("autoindex");
		valideDirectives.insert("return");
		valideDirectives.insert("index");
		valideDirectives.insert("allowed_methods");
		valideDirectives.insert("client_max_body_size");
	}

	return valideDirectives.find(directive) != valideDirectives.end();
}

// Private method(s) -----------------------------------------------------------

std::string ConfigParser::removeComments(const std::string& content) const
{
	std::string result;
	result.reserve(content.size());
	stringVector lines = splitTokens(content, '\n');

	for (size_t i = 0; i < lines.size(); i++)
	{

		std::string line = lines[i];

		// find # pos
		size_t commentPos = line.find('#');

		if (commentPos != std::string::npos)
		{
			// Keep only part before #
			line = line.substr(0, commentPos);
		}

		// Add line to result
		result += line + "\n";
	}

	return result;
}

blockVector ConfigParser::extractBlocks(const std::string& content, const std::string& keyword) const
{
	blockVector blocks;
	blocks.reserve(8); // classical prealloc
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

		// Check if only whitespace between keyword and "{"
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
		// If keywordPos == 0, start of file, valid

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

void ConfigParser::parseServerBlock(const std::string& block, ServerBlock& server, size_t serverIndex) const
{
	// Extract all location blocks first
	blockVector locationBlocks = extractBlocks(block, "location");

	// Build cleanBlock by skipping location blocks
	std::string cleanBlock;
	cleanBlock.reserve(block.size());
	size_t lastPos = 0;

	for (size_t i = 0; i < locationBlocks.size(); i++)
	{
		// Copy until start of the block
		cleanBlock.append(block, lastPos, locationBlocks[i].startPos - lastPos);
		// Jump to next block
		lastPos = locationBlocks[i].endPos;
	}

	// Append remaining content after last location
	if (lastPos < block.size())
		cleanBlock.append(block, lastPos, block.size() - lastPos);

	// Parse directives (listen, server_name, etc.)
	stringVector lines = splitTokens(cleanBlock, '\n');
	for (size_t i = 0; i < lines.size(); i++)
	{
		// trim each line
		std::string line = trim(lines[i]);
		if (line.empty())
			continue;

		// Remove ";" at the end
		if (line[line.length() - 1] == ';')
			line = line.substr(0, line.length() - 1);
		else
			throw std::runtime_error("Config syntax error: missing ';' at end of directive: " + line);

		// Extract tokens from the line
		stringVector tokens = splitTokens(line, ' ');

		if (tokens.empty())
			continue;

		// Parse directive
		if (tokens[0] == "listen" && tokens.size() >= 2)
			server.setPort(parseIntSafe(tokens[1].c_str(), "Server #" + intToString(serverIndex)));
		else if (tokens[0] == "server_name" && tokens.size() >= 2)
			server.setServerName(tokens[1]);
		else if (tokens[0] == "error_page" && tokens.size() >= 3)
		{
			std::string path = tokens[tokens.size() - 1]; // last token = path
			for (size_t j = 1; j < tokens.size() - 1; j++) // all the other are error code
				server.addErrorPage(parseIntSafe(tokens[j].c_str(), "Server #" + intToString(serverIndex)), path);
		}
		else if (tokens[0] == "client_max_body_size" && tokens.size() >= 2)
			server.setMaxBodySize(parseSize(tokens[1], "Server #" + intToString(serverIndex)));
		else if (tokens[0] == "cgi_timeout" && tokens.size() >= 2)
			server.setCgiTimeout(parseIntSafe(tokens[1].c_str(), "Server #" + intToString(serverIndex)));
		else
		{
			if(isKnownServerDirective(tokens[0]))
				std::cerr << "Warning [Serveur #" << serverIndex << "]: directive require value '" << tokens[0] << "'\n";
			else
				std::cerr << "Warning [Serveur #" << serverIndex << "]: Unknown directive '" << tokens[0] << "'\n";
		}
	}

	// Parse each location block
	for (size_t i = 0; i < locationBlocks.size(); i++)
	{
		Location loc;
		parseLocationBlock(locationBlocks[i].content, loc);
		server.addLocation(loc);
	}
}

void ConfigParser::parseLocationBlock(const std::string& block, Location& location) const
{
	// Extract path from header
	size_t openBrace = block.find("{");
	if (openBrace == std::string::npos)
		return;

	std::string header = block.substr(0, openBrace);
	stringVector headerTokens = splitTokens(trim(header), ' ');
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
	stringVector lines = splitTokens(content, '\n');
	for (size_t i = 0; i < lines.size(); i++)
	{
		// trim each line
		std::string line = trim(lines[i]);
		if (line.empty())
			continue;

		// Remove ";" at the end
		if (line[line.length() - 1] == ';')
			line = line.substr(0, line.length() - 1);
		else
			throw std::runtime_error("Config syntax error: missing ';' at end of directive: " + line);

		// Extract tokens from the line
		stringVector tokens = splitTokens(line, ' ');
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
		{
			// clear old default location
			location.clearAllowedMethods();
			for (size_t j = 1; j < tokens.size(); j++)
				location.addAllowedMethod(tokens[j]);
		}
		else if (tokens[0] == "client_max_body_size" && tokens.size() >= 2)
			location.setMaxBodySize(parseSize(tokens[1], "Location"));
		else
		{
			if(isKnownLocationDirective(tokens[0]))
				std::cerr << "Warning [Location '" << path << "']: directive require value '" << tokens[0] << "'\n";
			else
				std::cerr << "Warning [Location '" << path << "']: Unknown directive '" << tokens[0] << "'\n";
		}
	}
}

size_t ConfigParser::parseSize(const std::string& sizeStr, const std::string& context) const
{
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
	else if (lastChar == 'G' || lastChar =='g')
	{
		numStr = sizeStr.substr(0, len - 1);
		multiplier = 1024 * 1024 * 1024;
	}

	if (numStr.empty())
	{
		std::cerr << "Warning [" << context << "]: Size '" << sizeStr << "' has unit but no number\n";
		return 0;
	}

	for (size_t i = 0; i < numStr.length(); i++)
	{
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

bool ConfigParser::validate() const
{
	stringVector errors;

	// Check if there are at least one server
	if (_servers.empty())
		errors.push_back("No server configured");

	// Check each server
	for (size_t i = 0; i < _servers.size(); i++)
	{
		const ServerBlock& server = _servers[i];
		int port = server.getPort();
		std::string serverName = server.getServerName();

		// Msg prefix
		std::string prefix = "Server " + intToString(i) + " (" + serverName + "): ";

		// Check port range
		if (port < 1 || port > MAX_PORT_NUMBER)
			errors.push_back(prefix + "Invalid port " + intToString(port));

		// Check for duplicate Port
		for (size_t j = i + 1; j < _servers.size(); j++)
		{
			if (_servers[j].getPort() == port)
			{
				// Same port ok if different server name
				if (_servers[j].getServerName() == server.getServerName())
					errors.push_back(prefix + "Duplicate port " + intToString(port) + " with same server name");
			}
		}

		// Check size
		size_t maxSize = server.getMaxBodySize();
		if (maxSize == 0)
			errors.push_back(prefix + "Invalid max body size (0 bytes not allowed)");

		// Check locations
		const locationVector& locations = server.getLocations();
		for (size_t j = 0; j < locations.size(); j++)
		{
			const Location& loc = locations[j];
			std::string path = loc.getPath();
			std::string locPrefix = prefix + "Location '" + path + "': ";

			// Check Path starts with '/'
			if (path.empty() || path[0] != '/')
				errors.push_back(locPrefix + "Invalid path (must start with /)");

			// Check root is defined
			if (loc.getRoot().empty())
				errors.push_back(locPrefix + "Missing root directive");

			// Check CGI extension has corresponding CGI path
			if (!loc.getCgiExtension().empty() && loc.getCgiPath().empty())
				errors.push_back(locPrefix + "CGI extension without CGI path");

			// Check HTTP methods are valid
			const stringVector& methods = loc.getAllowedMethods();
			for (size_t k = 0; k < methods.size(); k++)
			{
				const std::string& method = methods[k];
				if (!isValidHttpMethod(method))
					errors.push_back(locPrefix + "Invalid HTTP method '" + method + "'");
			}
		}

		// Check for duplicate loc path
		for (size_t j = 0; j < locations.size(); j++)
			for (size_t k = j + 1; k < locations.size(); k++)
				if (locations[j].getPath() == locations[k].getPath())
					errors.push_back(prefix + "Duplicate location '" + locations[j].getPath() + "'");

		// Check that root location "/" exist
		bool hasRootLocation = false;

		for (size_t j = 0; j < locations.size(); j++)
			if (locations[j].getPath() == "/")
			{
				hasRootLocation = true;
				break;
			}
		if (!hasRootLocation)
			errors.push_back(prefix + "Missing root location '/'");

		// Check error pages
		const std::map<int, std::string>& errorPages = server.getErrorPages();

		for (std::map<int, std::string>::const_iterator it = errorPages.begin(); it != errorPages.end(); ++it)
		{
			int code = it->first;
			const std::string& pagePath = it->second;

			// Check HTTP error code range (4xx and 5xx only) (redirection not supported 3xx)
			if (code < 400 || code > 599)
				errors.push_back(prefix + "Invalid error_page code " + intToString(code) + " (must be 400-599)");

			// Check that the error page file exists (resolve against "/" location root)
			std::string resolvedPath;
			for (size_t j = 0; j < locations.size(); ++j)
				if (locations[j].getPath() == "/")
				{
					resolvedPath = joinPath(locations[j].getRoot(), pagePath);
					break;
				}

			if (resolvedPath.empty())
				resolvedPath = joinPath(".", pagePath);

			// If not found just display warning non blocking
			if (!fileExists(resolvedPath))
				std::cerr << "Warning [" << prefix << "] error_page " << intToString(code) << " file not found: " << resolvedPath << std::endl;
		}
	}

	if (!errors.empty())
	{
		std::string msg = "Config validation failed:\n";

		for (size_t i = 0; i < errors.size(); i++)
			msg += " - " + errors[i] + "\n";

		throw std::runtime_error(msg);
	}

	return true;
}

// Public method(s) ------------------------------------------------------------

bool ConfigParser::parse(const std::string& filePath)
{
	try {

		std::string content = readFile(filePath, "ConfigParser");
		if (content.empty())
			return false;

		// Remove Comment
		content = removeComments(content);

		blockVector serverBlocks = extractBlocks(content, "server");
		for (size_t i = 0; i < serverBlocks.size(); i++)
		{
			ServerBlock server;
			parseServerBlock(serverBlocks[i].content, server, i);
			_servers.push_back(server);
		}
		return validate();
	} catch (const std::exception& e) {
		std::cerr << "ConfigParser::parse error: " << e.what() << std::endl;
		return false;
	}
}
