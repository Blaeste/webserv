/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:49 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/16 10:17:16 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "utils.hpp"
#include <cerrno>
#include <cstdlib> // for strtol
#include <cstring>
#include <ctime> // for getHttpDate -> strftime
#include <dirent.h> // for listDirectory
#include <fcntl.h> // open(), O_RDONLY
#include <iostream>
#include <sstream> // for urlDecode
#include <sys/stat.h> // stat() => files info
#include <unistd.h> // read(), close()
#include <sys/socket.h>  // for getsockname()
#include <netinet/in.h>  // for sockaddr_in, ntohs()
#include <limits.h>
#include <cstdio> //snprintf

// Function(s)
std::string trim(const std::string &str) {
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

bool fileExists(const std::string &path) {
	return (access(path.c_str(), F_OK) == 0);
}

bool isDirectory(const std::string &path) {
	struct stat sb;

	// Check if file exist else return false
	if (stat(path.c_str(), &sb) != 0)
		return false;

	// Check if is directory with macro, st_mode => permissions type
	return S_ISDIR(sb.st_mode);
}

std::string getFileExtension(const std::string &path) {
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

std::string readFile(const std::string &path) {
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

std::string intToString(int value) {
	std::stringstream ss;
	ss << value;
	return ss.str();
}

std::vector<std::string> listDirectory(const std::string &path) {
	std::vector<std::string> entries;

	DIR *dir = opendir(path.c_str());
	if (!dir) {
		std::cerr << "[listDirectory] opendir failed for path: " << path << std::endl;
		return entries; // return empty vector if error
	}

	struct dirent *entry;
	while ((entry = readdir(dir))) {
		std::string name = entry->d_name;
		// Skip . and ..
		if (name != "." && name != "..")
			entries.push_back(name);
	}

	closedir(dir);
	return entries;
}

std::string normalizeHeaderKey(const std::string& key) {
	std::string result = key;
	for (size_t i = 0; i < result.length(); ++i)
		if (result[i] >= 'A' && result[i] <= 'Z')
			result[i] = result[i] + 32;
	return result;
}

std::string generateSessionId() {

	const char charset[] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
	const size_t idLength = 32;
	const size_t charsetSize = sizeof(charset)- 1;

	// Open urandom
	int fd = open("/dev/urandom", O_RDONLY);
	if (fd < 0)
		throw std::runtime_error("Failed to open /dev/urandom (generateSesssionId)");

	// Read urandom
	unsigned char randomBytes[idLength];
	if (read(fd, randomBytes, idLength) != (ssize_t)idLength) {
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

void safeClose(int fd) {
	if (close(fd) < 0)
		std::cerr << "[safeClose] close failed on fd " << fd << ": " << std::strerror(errno) << std::endl;
}

bool isPathSafe(const std::string &path) {

	// Url decode (protect against hex %2e%2e)
	std::string decoded = urlDecode(path);

	// Refuse any path containing ".."
	if (decoded.find("..") != std::string::npos)
		return false;

	// Get canonical path with realpath
	char resolvedPath[PATH_MAX];

	// Try realpath if fail normalize it
	if (!realpath(decoded.c_str(), resolvedPath)) {
		// File doest exist try resolve parent directory
		std::string parentPath = decoded;
		std::string fileName;
		size_t lastSlash = parentPath.find_last_of('/');

		if (lastSlash == std::string::npos) {
			// No slash - relative path in current dir
			char cwd[PATH_MAX];
			if (!getcwd(cwd, sizeof(cwd)))
				return false;

			fileName = decoded;
			snprintf(resolvedPath, PATH_MAX, "%s/%s", cwd, fileName.c_str());
		} else {
			// Has slash
			parentPath = parentPath.substr(0, lastSlash);

			char parentResolved[PATH_MAX];
			if (!realpath(parentPath.c_str(), parentResolved))
				return false; // Parent doesn't exist

			fileName = decoded.substr(lastSlash + 1); // skip '/'
			snprintf(resolvedPath, PATH_MAX, "%s/%s", parentResolved, fileName.c_str());
		}
	}

	// Static computed only once path gen with cwd
	static std::vector<std::string> allowedDirs;
	if (allowedDirs.empty()) {

		char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd))) {

			std::string baseDir(cwd);
			allowedDirs.push_back(baseDir + "/www");
			allowedDirs.push_back(baseDir + "/uploads");
			allowedDirs.push_back(baseDir + "/cgi-bin");
		}
	}

	std::string canonical(resolvedPath);
	for (size_t i = 0; i < allowedDirs.size(); i++) {
		size_t prefixLen = allowedDirs[i].size();

		// Check for start match
		if (canonical.compare(0, prefixLen, allowedDirs[i]) == 0) {
			// Check for
			// - end string (path = autorised path)
			// - one '/' (subdirectory valid)
			if (canonical.length() == prefixLen || canonical[prefixLen] == '/')
				return true;
		}
	}
	return false;
}

bool isPathSafeForUpload(const std::string &path) {

	// Url decode (protect against hex %2e%2e)
	std::string decoded = urlDecode(path);

	// Refuse any path containing ".."
	if (decoded.find("..") != std::string::npos)
		return false;

	// Get canonical path with realpath
	char resolvedPath[PATH_MAX];
	std::string checkPath = decoded;

	// Try realpath on the path itself
	if (realpath(decoded.c_str(), resolvedPath) ==  NULL) {
		// Path doesn't exist - try parent directory(file upload case)
		std::string parentPath = decoded;
		size_t lastSlash = parentPath.find_last_of('/');

		if (lastSlash != std::string::npos) {
			parentPath = parentPath.substr(0, lastSlash);
			std::string fileName = decoded.substr(lastSlash + 1);

			char parentResolved[PATH_MAX];
			if (!realpath(parentPath.c_str(), parentResolved))
				return false;

			snprintf(resolvedPath, PATH_MAX, "%s/%s", parentResolved, fileName.c_str());
		} else {
			// No slash - current directory
			char cwd[PATH_MAX];
			if (!getcwd(cwd, sizeof(cwd)))
				return false;
			snprintf(resolvedPath, PATH_MAX, "%s/%s", cwd, decoded.c_str());
		}
	}

	// Static computed only once path gen with cwd
	static std::vector<std::string> allowedDirs;
	if (allowedDirs.empty()) {

		char cwd[PATH_MAX];
		if (getcwd(cwd, sizeof(cwd))) {

			std::string baseDir(cwd);
			allowedDirs.push_back(baseDir + "/uploads");
		}
	}

	std::string canonical(resolvedPath);
	for (size_t i = 0; i < allowedDirs.size(); i++) {
		size_t prefixLen = allowedDirs[i].size();

		// Check for start match
		if (canonical.compare(0, prefixLen, allowedDirs[i]) == 0) {
			// Check for
			// - end string (path = autorised path)
			// - one '/' (subdirectory valid)
			if (canonical.length() == prefixLen || canonical[prefixLen] == '/')
				return true;
		}
	}
	return false;
}

void setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error("fcntl(F_GETFL) failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl(F_SETFL) failed");
}

std::string sizetToString(size_t n) {
	std::stringstream ss;
	ss << n;
	return ss.str();
}

std::vector<std::string> splitTokens(const std::string &str, char delimiter) {

	std::vector<std::string> result;
	std::string buffer;

	for (size_t i = 0; i < str.length(); i++) {

		bool isDelimiter = false;

		// If delimiter is ' '  accept /t /n /r
		if (delimiter == ' ') {
			if (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r')
				isDelimiter = true;
		} else {
			if (str[i] == delimiter)
				isDelimiter = true;
		}

		if (isDelimiter) {
			if (!buffer.empty()) {
				result.push_back(buffer);
				buffer.clear();
			}
		} else {
			buffer += str[i];
		}
	}

	if (!buffer.empty())
		result.push_back(buffer);

	return result;
}

int getSocketPort(int fd) {
	sockaddr_in addr;

	socklen_t len = sizeof(addr);
	if (getsockname(fd, (sockaddr*)&addr, &len) == 0)
		return ntohs(addr.sin_port);
	return -1;
}


int parseIntSafe(const std::string &str, const std::string &context) { // Like atoi but better

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

bool isValidHttpMethod(const std::string &method) {
	static std::set<std::string> validMethods;

	if (validMethods.empty()) {
		validMethods.insert("GET");
		validMethods.insert("POST");
		validMethods.insert("DELETE");
		validMethods.insert("PUT");
		validMethods.insert("HEAD");
		validMethods.insert("OPTIONS");
	}
	return validMethods.find(method) != validMethods.end();
}

std::string urlDecode(const std::string &url) {
	std::string decoded;

	for (size_t i = 0; i < url.length(); i++) {
		if (url[i] == '%' && i + 2 < url.length()) {
			// Decode %XX
			char hex[3] = {url[i+1], url[i+2], '\0'};
			char *endptr;
			long value = strtol(hex, &endptr, 16);

			// Valid hex
			if (*endptr == '\0') {
				decoded += static_cast<char>(value);
				i += 2; // skip 2 next char
			} else {
				decoded += url[i]; // if invalid keep at it is
			}
		} else if (url[i] == '+') {
			decoded += ' '; // + = space in URL encoding
		} else {
			decoded += url[i];
		}
	}
	return decoded;
}
