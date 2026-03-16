/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:52 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/16 15:32:35 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef UTILS_HPP
# define UTILS_HPP

// Include(s) ------------------------------------------------------------------

# include "../webserv.hpp"
# include <string>			// std::string
# include <vector>			// std::vector

// Function prototype(s) -------------------------------------------------------

/** @brief Trims leading and trailing whitespace from str. */
std::string		trim(const std::string& str);

/** @brief Decodes percent-encoded characters in a URL string. */
std::string		urlDecode(const std::string& url);

/** @brief Returns true if a file or directory exists at path. */
bool			fileExists(const std::string& path);

/** @brief Returns true if path points to a directory. */
bool			isDirectory(const std::string& path);

/** @brief Returns the extension of path including the dot (e.g., ".html"); throws if none. */
std::string		getFileExtension(const std::string& path);

/** @brief Reads the entire content of a file and returns it as a string. */
std::string		readFile(const std::string& path, const std::string& caller);

/** @brief Converts any integral value to its decimal string representation. */
std::string		intToString(long long value);

/** @brief Closes fd and logs a warning on failure, using caller as context. */
void			safeClose(int& fd, const std::string& caller);

/**
 * @brief Checks that path stays within root after normalization (prevents directory traversal).
 * @return true if the resolved path is inside root, false otherwise.
 */
bool			isPathSafe(const std::string& path, const std::string& root);

/** @brief Sets fd to non-blocking mode; throws std::runtime_error on fcntl() failure. */
void			setNonBlocking(int fd);

/** @brief Splits str on delimiter and returns only non-empty tokens. */
stringVector	splitTokens(const std::string& str, char delimiter);

/**
 * @brief Parses str to int; throws std::runtime_error on failure or out-of-range.
 * @param context Label used in the error message (e.g., "port", "status code").
 */
int				parseIntSafe(const std::string& str, const std::string& context);

/** @brief Returns a lowercased copy of str. */
std::string		toLowercase(const std::string& str);

/** @brief Joins root and path into a single path, avoiding double slashes. */
std::string		joinPath(const std::string& root, const std::string& path);

#endif
