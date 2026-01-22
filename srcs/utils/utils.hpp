/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lmarck <lmarck@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:52 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/21 14:36:37 by lmarck           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s)
#include <string>
#include <vector>
#include <set>

// Function prototype(s)

/**
 * @brief Utility functions for string manipulation, file handling, and date formatting.
 * @param str The input string to trim.
 * @return A new string with leading and trailing whitespace removed.
 */
std::string trim(const std::string &str);

/**
 * @brief Checks if a character is a hexadecimal digit.
 * @param c The character to check.
 * @return true if the character is a hex digit, false otherwise.
 */
bool isHexDigit(char c);

/**
 * @brief Decodes a URL-encoded string.
 * @param url The URL-encoded string.
 * @return The decoded string.
 */
std::string urlDecode(const std::string &url);

/**
 * @brief Checks if a file exists at the given path.
 * @param path The file path to check.
 * @return true if the file exists, false otherwise.
 */
bool fileExists(const std::string &path);

/**
 * @brief Checks if the given path is a directory.
 * @param path The path to check.
 * @return true if the path is a directory, false otherwise.
 */
bool isDirectory(const std::string &path);

/**
 * @brief Retrieves the file extension from a given file path.
 * @param path The file path.
 * @return The file extension, including the dot (e.g., ".txt"), or an empty string if none exists.
 */
std::string getFileExtension(const std::string &path);

/**
 * @brief Gets the current date and time formatted for HTTP headers.
 * @return A string representing the current date and time in HTTP format.
 */
std::string getHttpDate();

/**
 * @brief Reads the entire content of a file into a string.
 * @param path The path to the file.
 * @return A string containing the file's content.
 */
std::string readFile(const std::string &path);

/**
 * @brief Gets the size of a file in bytes.
 * @param path The path to the file.
 * @return The size of the file in bytes, or 0 if the file does not exist or is not a regular file.
 */
size_t getFileSize(const std::string &path);

/**
 * @brief Converts an integer to a string.
 * @param value The integer value to convert.
 * @return A string representation of the integer.
 */
std::string intToString(int value);

/**
 * @brief Lists all entries in a directory.
 * @param path The path to the directory.
 * @return A vector containing the names of all entries in the directory (excluding "." and "..").
 */
std::vector<std::string> listDirectory(const std::string &path);

/**
 * @brief Normalizes an HTTP header key to capitalize the first letter of each word.
 * @param key The header key to normalize (e.g., "content-type", "CONTENT-TYPE").
 * @return The normalized header key (e.g., "Content-Type").
 */
std::string normalizeHeaderKey(const std::string &key);

/**
 * @brief Generates a random session identifier for HTTP session management.
 * @return A unique string suitable for use as a session_id cookie value.
 */
std::string generateSessionId();

/**
 * @brief Closes a file descriptor and logs an error if the operation fails.
 * @param fd The file descriptor to close.
 */
void safeClose(int fd);

/**
 * @brief Validates that a file path stays under the provided root.
 * @param path The resolved file path you want to access (can be relative).
 * @param root The root directory that bounds access (can be relative).
 * @return true if the normalized path remains inside the normalized root, false otherwise.
 */
bool isPathSafe(const std::string &path, const std::string &root);

/**
 * @brief Sets a file descriptor to non-blocking mode.
 * @param fd The file descriptor to configure.
 * @throws std::runtime_error if fcntl() fails.
 */
void setNonBlocking(int fd);

/**
 * @brief Converts a size_t value to a string.
 * @param n The size_t value to convert.
 * @return A string representation of the size_t value.
 */
std::string sizetToString(size_t n);

/**
 * @brief Split string and filter empty tokens
 * @param str String to split
 * @param delimiter Delimiter character
 * @return Vector of non-empty tokens
 */
std::vector<std::string> splitTokens(const std::string &str, char delimiter);

/**
 * @brief Gets the local port number of a socket.
 * @param fd The socket file descriptor.
 * @return The port number in host byte order, or -1 if getsockname() fails.
 */
int getSocketPort(int fd);

/**
 * @brief Safely parses a string to an integer with error handling.
 * @param str The string to parse.
 * @param context Context for error messages (e.g., "port", "error_code").
 * @return The parsed integer value.
 * @throws std::runtime_error if parsing fails or value is out of range.
 */
int parseIntSafe(const std::string &str, const std::string &context);

/**
 * @brief Converts a string to lowercase.
 * @param str The input string.
 * @return A new string with all characters converted to lowercase.
 */
std::string toLowercase(const std::string &str);

/**
 * @brief Checks if the given HTTP method is valid.
 * @param method The HTTP method to validate (e.g., "GET", "POST
 * @return true if the method is valid, false otherwise.
 */
bool isValidHttpMethod(const std::string &method);

/**
 * @brief Converts a string to uppercase.
 * @param str The input string.
 * @return A new string with all characters converted to uppercase.
 */
std::string toUpperString(const std::string &str);
