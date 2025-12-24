/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:52 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/24 19:00:58 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// =============================================================================
// Includes

#pragma once

#include <string>
#include <vector>

// =============================================================================
// Function prototypes

/**
 * @brief Utility functions for string manipulation, file handling, and date formatting.
 * @param str The input string to trim.
 * @return A new string with leading and trailing whitespace removed.
 */
std::string trim(const std::string &str);

/**
 * @brief Splits a string into a vector of substrings based on a delimiter.
 * @param str The input string to split.
 * @param delimiter The character used as the delimiter.
 * @return A vector containing the split substrings.
 */
std::vector<std::string> split(const std::string &str, char delimiter);

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
