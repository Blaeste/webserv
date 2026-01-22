/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lmarck <lmarck@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:49 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/21 14:36:39 by lmarck           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "utils.hpp"
#include <cerrno>
#include <cstdlib> // for strtol
#include <cstring>
#include <ctime>	// for getHttpDate -> strftime
#include <dirent.h> // for listDirectory
#include <fcntl.h>	// open(), O_RDONLY
#include <iostream>
#include <sstream>		// for urlDecode
#include <sys/stat.h>	// stat() => files info
#include <unistd.h>		// read(), close()
#include <sys/socket.h> // for getsockname()
#include <netinet/in.h> // for sockaddr_in, ntohs()
#include <limits.h>
#include <cstdio> //snprintf

// Function(s)
std::string trim(const std::string &str)
{
	const std::string whitespace = " \t\n\r\f\v";

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

bool fileExists(const std::string &path)
{
	return (access(path.c_str(), F_OK) == 0);
}

bool isDirectory(const std::string &path)
{
	struct stat sb;

	// Check if file exist else return false
	if (stat(path.c_str(), &sb) != 0)
		return false;

	// Check if is directory with macro, st_mode => permissions type
	return S_ISDIR(sb.st_mode);
}

std::string getFileExtension(const std::string &path)
{
	size_t pos = path.find_last_of('.');
	size_t slash = path.find_last_of('/');

	// Si pas de point ou le point est avant le dernier slash
	if (pos == std::string::npos || (slash != std::string::npos && pos < slash))
		throw std::runtime_error("getFileExtension: invalid or missing extension in path: " + path);

	// Si le point est juste après le slash (fichier caché)
	if (slash != std::string::npos && pos == slash + 1)
		throw std::runtime_error("getFileExtension: hidden file, no extension in path: " + path);

	// Si le point est le premier caractère (fichier caché sans extension)
	if (slash == std::string::npos && pos == 0)
		throw std::runtime_error("getFileExtension: hidden file, no extension in path: " + path);

	// Extension valide
	return path.substr(pos);
}

std::string readFile(const std::string &path)
{
	int fd = open(path.c_str(), O_RDONLY);
	if (fd < 0)
		throw std::runtime_error("Failed to open file: " + path);

	char buffer[4096];
	std::string result;
	ssize_t bytes_read;

	while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
		result.append(buffer, bytes_read);

	close(fd);

	if (bytes_read < 0)
		throw std::runtime_error("Failed to read file: " + path);

	return result;
}

std::string intToString(int value)
{
	std::stringstream ss;
	ss << value;
	return ss.str();
}

std::vector<std::string> listDirectory(const std::string &path)
{
	std::vector<std::string> entries;

	DIR *dir = opendir(path.c_str());
	if (!dir)
	{
		std::cerr << "[listDirectory] opendir failed for path: " << path << std::endl;
		return entries; // return empty vector if error
	}

	struct dirent *entry;
	while ((entry = readdir(dir)))
	{
		std::string name = entry->d_name;
		// Skip . and ..
		if (name != "." && name != "..")
			entries.push_back(name);
	}

	closedir(dir);
	return entries;
}

std::string normalizeHeaderKey(const std::string &key)
{
	std::string result = key;
	for (size_t i = 0; i < result.length(); ++i)
		if (result[i] >= 'A' && result[i] <= 'Z')
			result[i] = result[i] + 32;
	return result;
}

std::string generateSessionId()
{

	const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const size_t idLength = 32;
	const size_t charsetSize = sizeof(charset) - 1;

	// Open urandom
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		throw std::runtime_error("Failed to open /dev/urandom (generateSesssionId)");

	// Read urandom
	unsigned char randomBytes[idLength];
	if (read(fd, randomBytes, idLength) != (ssize_t)idLength)
	{
		close(fd);
		throw std::runtime_error("Failed to read from /dev/urandom (generateSesssionId)");
	}
	close(fd);

	// Genrate session id base on urandom read
	std::string id;
	for (size_t i = 0; i < idLength; i++)
		id += charset[randomBytes[i] % charsetSize];
	return id;
}

void safeClose(int fd)
{
	if (close(fd) < 0)
		std::cerr << "[safeClose] close failed on fd " << fd << ": " << std::strerror(errno) << std::endl;
}

// Normalize a path by splitting components and resolving '.' and '..'.
// This does not touch the filesystem (no realpath), so it works with
// non-existent paths as long as the resulting layout is under the intended root.
static std::string normalizePath(const std::string &raw)
{
	std::vector<std::string> stack;
	size_t i = 0;

	while (i < raw.size())
	{
		// Skip repeated '/'
		while (i < raw.size() && raw[i] == '/')
			++i;
		size_t start = i;
		while (i < raw.size() && raw[i] != '/')
			++i;

		if (start == i)
			continue;

		std::string part = raw.substr(start, i - start);
		if (part == "." || part.empty())
			continue;
		if (part == "..")
		{
			if (!stack.empty())
				stack.pop_back(); // climb up one level when possible
			continue;
		}
		stack.push_back(part);
	}

	std::string normalized = "/";
	for (size_t j = 0; j < stack.size(); ++j)
	{
		normalized += stack[j];
		if (j + 1 < stack.size())
			normalized += "/";
	}
	return normalized;
}

bool isPathSafe(const std::string &path, const std::string &root)
{
	// 1) Decode percent-encoding to reject encoded traversal attempts early.
	std::string decodedPath = urlDecode(path);
	std::string decodedRoot = urlDecode(root);

	// 2) Quick traversal guard: plain ".." after decode is already suspicious.
	if (decodedPath.find("..") != std::string::npos)
		return false;

	// 3) Normalize both root and target paths (pure string math, no realpath).
	std::string normalizedRoot = normalizePath(decodedRoot);
	std::string normalizedPath = normalizePath(decodedPath);

	// 4) Empty roots are not acceptable for containment checks.
	if (normalizedRoot.empty())
		return false;

	// 5) The target must start with the root and either match exactly or have a '/'.
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

std::string sizetToString(size_t n)
{
	std::stringstream ss;
	ss << n;
	return ss.str();
}

std::vector<std::string> splitTokens(const std::string &str, char delimiter)
{

	std::vector<std::string> result;
	std::string buffer;

	for (size_t i = 0; i < str.length(); i++)
	{

		bool isDelimiter = false;

		// If delimiter is ' '  accept /t /n /r
		if (delimiter == ' ')
		{
			if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')
				isDelimiter = true;
		}
		else
		{
			if (str[i] == delimiter)
				isDelimiter = true;
		}

		if (isDelimiter)
		{
			if (!buffer.empty())
			{
				result.push_back(buffer);
				buffer.clear();
			}
		}
		else
		{
			buffer += str[i];
		}
	}

	if (!buffer.empty())
		result.push_back(buffer);

	return result;
}

int getSocketPort(int fd)
{
	sockaddr_in addr;

	socklen_t len = sizeof(addr);
	if (getsockname(fd, (sockaddr *)&addr, &len) == 0)
		return ntohs(addr.sin_port);
	return -1;
}

int parseIntSafe(const std::string &str, const std::string &context)
{ // Like atoi but better

	// Like atoi but better

	// Check string empty
	if (str.empty())
		throw std::runtime_error("parseIntSafe [" + context + "]: empty string");

	// Check if all cha is digit
	size_t start = 0;

	if (str[0] == '+' || str[0] == '-')
		start = 1;

	if (start >= str.length())
		throw std::runtime_error("parseIntSafe [" + context + "]: only sign character");

	for (size_t i = start; i < str.length(); i++)
		if (str[i] < '0' || str[i] > '9')
			throw std::runtime_error("parseIntSafe [" + context + "]: invalid character in: " + str);

	// Use strtol for convert with overflow detection
	char *endptr;
	errno = 0;
	long val = strtol(str.c_str(), &endptr, 10);

	// Check errors
	if (errno == ERANGE || val > INT_MAX || val < INT_MIN)
		throw std::runtime_error("parseIntSafe [" + context + "]: value out of range: " + str);

	if (endptr == str.c_str() || *endptr != '\0')
		throw std::runtime_error("parseIntSafe [" + context + "]: conversion failed for: " + str);

	return static_cast<int>(val);
}

std::string toLowercase(const std::string &str)
{
	std::string result = str;
	for (size_t i = 0; i < result.length(); i++)
	{
		if (result[i] >= 'A' && result[i] <= 'Z')
			result[i] += 32;
	}
	return result;
}

bool isValidHttpMethod(const std::string &method)
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

std::string urlDecode(const std::string &url)
{
	std::string decoded;

	for (size_t i = 0; i < url.length(); i++)
	{
		if (url[i] == '%' && i + 2 < url.length())
		{
			// Decode %XX
			char hex[3] = {url[i + 1], url[i + 2], '\0'};
			char *endptr;
			long value = strtol(hex, &endptr, 16);

			// Valid hex
			if (*endptr == '\0')
			{
				decoded += static_cast<char>(value);
				i += 2; // skip 2 next char
			}
			else
			{
				decoded += url[i]; // if invalid keep at it is
			}
		}
		else if (url[i] == '+')
		{
			decoded += ' '; // + = space in URL encoding
		}
		else
		{
			decoded += url[i];
		}
	}
	return decoded;
}

std::string toUpperString(const std::string &str)
{
	std::string result = str;
	for (size_t i = 0; i < result.size(); i++)
		result[i] = std::toupper(static_cast<unsigned char>(result[i]));
	return result;
}

std::string getHttpDate()
{
	time_t now = time(NULL);
	struct tm *gmt = gmtime(&now);
	char buffer[100];
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
	return std::string(buffer);
}
