/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.cpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:49 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/12 11:14:19 by gdosch           ###   ########.fr       */
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

std::vector<std::string> split(const std::string &str, char delimiter) {
	std::vector<std::string> result;
	std::string buffer;

	for (size_t i = 0; i < str.length(); i++) {
		// Search for delimiter
		if (str[i] == delimiter) {
			result.push_back(buffer);
			buffer.clear();
		}
		else
			buffer += str[i];
	}
	result.push_back(buffer);
	return result;
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
	std::string id;
	for (size_t i = 0; i < idLength; i++)
		id += charset[std::rand() % (sizeof(charset) - 1)];
	return id;
}

void safeClose(int fd) {
	if (close(fd) < 0)
		std::cerr << "[safeClose] close failed on fd " << fd << ": " << std::strerror(errno) << std::endl;
}

bool isPathSafe(const std::string &path) {
	// Refuse any path containing ".."
	if (path.find("..") != std::string::npos)
		return false;
	// Check if path starts with allowed directories
	const std::string allowedDirs[] = {"./www/", "www/", "./uploads/", "uploads/", "./cgi-bin/", "cgi-bin/"};
	for (size_t i = 0; i < sizeof(allowedDirs) / sizeof(allowedDirs[0]); i++)
		if (path.compare(0, allowedDirs[i].size(), allowedDirs[i]) == 0)
			return true;
	return false;
}

void setNonBlocking(int fd) {
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		throw std::runtime_error("fcntl(F_GETFL) failed");
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl(F_SETFL) failed");
}
