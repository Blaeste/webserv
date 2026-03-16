/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:49 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/16 15:32:53 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------

#include "utils.hpp"
#include "Logger.hpp"
#include <iostream>		// std::cerr
#include <limits>		// std::numeric_limits
#include <sstream>		// std::stringstream
#include <stdexcept>	// std::runtime_error
#include <cctype>		// std::tolower
#include <cerrno>		// errno
#include <cstdlib>		// std::strtol
#include <cstring>		// std::strerror
#include <fcntl.h>		// open, fcntl, O_RDONLY, O_NONBLOCK, F_GETFL, F_SETFL
#include <sys/stat.h>	// stat, S_ISDIR, struct stat
#include <unistd.h>		// read, close, access, F_OK

// Static helper(s) ------------------------------------------------------------

// Normalize a path by splitting components and resolving '.' and '..'.
// This does not touch the filesystem (no realpath), so it works with
// non-existent paths as long as the resulting layout is under the intended root.
static std::string normalizePath(const std::string& raw)
{
	stringVector parts = splitTokens(raw, '/');
	stringVector stack;

	for (size_t i = 0; i < parts.size(); ++i)
	{
		if (parts[i] == ".")
			continue;
		if (parts[i] == "..")
		{
			if (!stack.empty())
				stack.pop_back();
		}
		else
			stack.push_back(parts[i]);
	}

	std::string result = "/";
	for (size_t i = 0; i < stack.size(); ++i)
	{
		if (i > 0)
			result += "/";
		result += stack[i];
	}
	return result;
}

// Function(s) -----------------------------------------------------------------

std::string trim(const std::string& str)
{
	static const std::string whitespace = " \t\n\r\f\v";

	// Search for first none-whitespace character
	size_t start = str.find_first_not_of(whitespace);

	// if none character are found, return empty string
	if (start == std::string::npos)
		return "";

	// Search last none-whitespace character
	size_t end = str.find_last_not_of(whitespace);

	// Extract the substring
	return str.substr(start, end - start + 1);
}

bool fileExists(const std::string& path)
{
	return (access(path.c_str(), F_OK) == 0);
}

bool isDirectory(const std::string& path)
{
	struct stat sb;

	// Check if file exist else return false
	if (stat(path.c_str(), &sb) != 0)
		return false;

	// Check if is directory with macro, st_mode => permissions type
	return S_ISDIR(sb.st_mode);
}

std::string getFileExtension(const std::string& path)
{
	size_t pos = path.find_last_of('.');
	size_t slash = path.find_last_of('/');

	// If no dot or dot is before the last slash
	if (pos == std::string::npos || (slash != std::string::npos && pos < slash))
		throw std::runtime_error("getFileExtension: invalid or missing extension in path: " + path);

	// If dot is right after slash (hidden file)
	if (slash != std::string::npos && pos == slash + 1)
		throw std::runtime_error("getFileExtension: hidden file, no extension in path: " + path);

	// If dot is first character (hidden file without extension)
	if (slash == std::string::npos && pos == 0)
		throw std::runtime_error("getFileExtension: hidden file, no extension in path: " + path);

	// Valid extension
	return path.substr(pos);
}

std::string readFile(const std::string& path, const std::string& caller)
{
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0)
		throw std::runtime_error("Failed to open file: " + path);

	char buffer[READ_BUFFER_SIZE];
	std::string result;
	ssize_t bytes_read;

	// Regular disk files are exempt from poll() readiness checks
	while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
		result.append(buffer, bytes_read);

	safeClose(fd, caller);

	if (bytes_read < 0)
		throw std::runtime_error("Failed to read file: " + path);

	return result;
}

std::string intToString(long long value)
{
	std::stringstream ss;
	ss << value;
	return ss.str();
}

void safeClose(int& fd, const std::string& caller)
{
	if (fd == -1)
		return;
	if (close(fd) < 0)
	{
		if (Logger::hasStarted())
			Logger::logMessage(RED "[" + caller + "] Error: " RESET "safeClose: close failed on fd " + intToString(fd) + ": " + std::strerror(errno));
		else
			std::cerr << "[safeClose] close failed on fd " + intToString(fd) + ": " + std::strerror(errno) << std::endl;
	}
	fd = -1;
}

stringVector splitTokens(const std::string& str, char delimiter)
{
	stringVector result;
	std::string buffer;

	for (size_t i = 0; i < str.length(); i++)
	{
		bool isDelim = (delimiter == ' ')
			? (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')
			: (str[i] == delimiter);

		if (isDelim)
		{
			if (!buffer.empty())
			{
				result.push_back(buffer);
				buffer.clear();
			}
		}
		else
			buffer += str[i];
	}

	if (!buffer.empty())
		result.push_back(buffer);

	return result;
}

bool isPathSafe(const std::string& path, const std::string& root)
{
	// Decode percent-encoding to reject encoded traversal attempts early.
	std::string decodedPath = urlDecode(path);
	std::string decodedRoot = urlDecode(root);

	// Quick traversal guard: plain ".." after decode is already suspicious.
	if (decodedPath.find("..") != std::string::npos)
		return false;

	// Normalize both root and target paths (pure string math, no realpath).
	std::string normalizedRoot = normalizePath(decodedRoot);
	std::string normalizedPath = normalizePath(decodedPath);

	// Empty roots are not acceptable for containment checks.
	if (normalizedRoot.empty())
		return false;

	// Root "/" contains every absolute path by definition.
	if (normalizedRoot == "/")
		return normalizedPath[0] == '/';

	// The target must start with the root and either match exactly or have a '/'.
	if (normalizedPath.compare(0, normalizedRoot.size(), normalizedRoot) != 0)
		return false;
	if (normalizedPath.size() == normalizedRoot.size())
		return true;
	return normalizedPath[normalizedRoot.size()] == '/';
}

void setNonBlocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error("fcntl(F_GETFL) failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl(F_SETFL) failed");
}

int parseIntSafe(const std::string& str, const std::string& context)
{
	if (str.empty())
		throw std::runtime_error("parseIntSafe [" + context + "]: empty string");

	// Count digit characters (skip optional leading sign)
	size_t start = (str[0] == '+' || str[0] == '-') ? 1 : 0;
	size_t digitCount = str.length() - start;

	// Cap at 10 digits: max 10-digit number (9,999,999,999) always fits in
	// a 64-bit long, so strtol can never overflow without needing errno.
	if (digitCount == 0 || digitCount > 10)
		throw std::runtime_error("parseIntSafe [" + context + "]: value out of range: " + str);

	char* endptr;
	long val = std::strtol(str.c_str(), &endptr, 10);

	if (endptr == str.c_str() || *endptr != '\0')
		throw std::runtime_error("parseIntSafe [" + context + "]: invalid input: " + str);

	if (val > std::numeric_limits<int>::max() || val < std::numeric_limits<int>::min())
		throw std::runtime_error("parseIntSafe [" + context + "]: value out of range: " + str);

	return static_cast<int>(val);
}

std::string toLowercase(const std::string& str)
{
	std::string result = str;
	for (std::string::size_type i = 0; i < result.size(); ++i)
	{
		unsigned char c = static_cast<unsigned char>(result[i]);
		result[i] = static_cast<char>(std::tolower(c));
	}
	return result;
}

std::string urlDecode(const std::string& url)
{
	std::string decoded;

	for (size_t i = 0; i < url.length(); i++)
	{
		if (url[i] == '%' && i + 2 < url.length())
		{
			// Decode %XX
			char hex[3] = {url[i + 1], url[i + 2], '\0'};
			char* endptr;
			long value = std::strtol(hex, &endptr, 16);

			// Valid hex
			if (*endptr == '\0')
			{
				decoded += static_cast<char>(value);
				i += 2; // skip 2 next char
			}
			else
				decoded += url[i]; // if invalid keep at it is
		}
		else if (url[i] == '+')
			decoded += ' '; // + = space in URL encoding
		else
			decoded += url[i];
	}
	return decoded;
}

std::string joinPath(const std::string& root, const std::string& path)
{
	if (root.empty())
		return joinPath(".", path);
	if (path.empty())
		return root;
	bool rootEnds = root[root.size() - 1] == '/';
	bool pathStarts = path[0] == '/';
	if (rootEnds && pathStarts)
		return root + path.substr(1);
	if (!rootEnds && !pathStarts)
		return root + "/" + path;
	return root + path;
}
