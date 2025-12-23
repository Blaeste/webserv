/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:49 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/23 11:19:58 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// =============================================================================
// Includes

#include "utils.hpp"
#include <fcntl.h> // open(), O_RDONLY
#include <unistd.h> // read(), close()
#include <sys/stat.h> // stat() => files info
#include <sstream> // for urlDecode
#include <cstdlib> // for strtol
#include <ctime> // for getHttpDate -> strftime

// =============================================================================
// Functions

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

std::vector<std::string> split(const std::string &str, char delimiter)
{
	std::vector<std::string> result;
	std::string buffer;

	for (size_t i = 0; i < str.length(); i++)
	{
		// Search for delimiter
		if (str[i] == delimiter)
		{
			result.push_back(buffer);
			buffer.clear();
		}
		else
			buffer += str[i];
	}
	result.push_back(buffer);
	return result;
}

bool isHexDigit(char c)
{
	return	(c >= '0' && c <= '9') ||
			(c >= 'A' && c <= 'Z') ||
			(c >= 'a' && c <= 'z');
}

std::string urlDecode(const std::string &url)
{
	std::string result;

	for (size_t i = 0; i  < url.length(); i++)
	{
		if (url[i] == '%' && i + 2 < url.length() &&
			isHexDigit(url[i + 1]) && isHexDigit(url[i + 2]))
		{
			// extract the 2 hex characters
			char hex[3] = { url[i + 1], url[i + 2], '\0' };

			// convert in ASCII
			char decoded = static_cast<char>(strtol(hex, NULL, 16));
			result += decoded;

			i += 2; // jump 2 charactere (hex we just decode)
		}

		// Query + => espace
		else if (url[i] == '+')
			result += ' ';

		// Just write url
		else
			result += url[i];
	}
	return result;
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

	// If not '.' or if '.' is befor last '/'
	if (pos == std::string::npos || (slash != std::string::npos && pos < slash))
		return ""; // TODO: Throw exception

	// If '.' is the first character of the file (hidden file) with path /path/.hidden
	if (slash != std::string::npos && pos == slash + 1)
		return ""; // TODO: Throw exception

	// If '.' is the first character of the file (hidden file) .hidden
	if (slash == std::string::npos && pos == 0)
		return ""; // TODO: Throw exception

	// Return start '.' to the end => extension
	return path.substr(pos);
}

std::string getHttpDate()
{
	// Actual timestamp
	time_t now = time(NULL);
	// Convert into GMT
	struct tm *gmt = gmtime(&now);

	// Format it
	char buffer[100];
	strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);

	return std::string(buffer);
}

std::string readFile(const std::string &path)
{
	int fd = open(path.c_str(), O_RDONLY);

	if (fd < 0)
		return ""; // TODO: Throw exception

	char buffer[4096];
	std::string result;
	ssize_t bytes_read;

	while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0)
		result.append(buffer, bytes_read);

	close(fd);

	if (bytes_read < 0)
		return ""; // TODO: Throw exception

	return result;
}

size_t getFileSize(const std::string &path)
{
	struct stat sb;

	// check if file exist
	if (stat(path.c_str(), &sb) != 0)
		return 0; // TODO: Throw exception

	// check if regular file (not folder etc.)
	if (!S_ISREG(sb.st_mode))
		return 0; // TODO: Throw exception

	return sb.st_size;
}

std::string intToString(int value)
{
	std::stringstream ss;
	ss << value;
	return ss.str();
}
