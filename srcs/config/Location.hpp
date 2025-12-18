/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:55 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/18 09:49:55 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Path de la location ("/", "/upload", etc.)
// Methode autorisees
// Root directory
// Index files
// CGI settings
// Upload path
// Redirection

// =============================================================================
// Includes

#pragma once

#include <string>
#include <vector>

// =============================================================================
// Typedef


// =============================================================================
// Defines


// =============================================================================
// Class Location

class Location
{

private:
	// =========================================================================
	// Attributs

	// "/", "/uploads", etc.
	std::string _path;

	// "./wwww"
	std::string _root;

	// ["GET", "POST"]
	std::vector<std::string> _allowedMethods;

	// ["index.html"]
	std::vector<std::string> _index;

	// true/false
	bool _autoIndex;

	// "./uploads"
	std::string _uploadPath;

	// "" or URL redirection
	std::string _redirect;

	// ".php"
	std::string _cgiExtension;

	// "usr/bin/php-cgi"
	std::string _cgiPath;

public:
	// =========================================================================
	// Handlers

	// Default constructor
	Location();

	// Destructor
	~Location();

	// =========================================================================
	// Setters => set replace val, add add val

	/**
	 * @brief Sets the path of the location.
	 * @param path The path to set (e.g., "/uploads").
	 */
	void setPath(const std::string &path);

	/**
	 * @brief Sets the root directory for this location.
	 * @param root The root directory path.
	 */
	void setRoot(const std::string &root);

	/**
	 * @brief Adds an allowed HTTP method to the location.
	 * @param method The HTTP method to allow (e.g., "GET", "POST").
	 */
	void addAllowedMethod(const std::string &method);

	/**
	 * @brief Adds an index file to the location.
	 * @param index The name of the index file to add.
	 */
	void addIndex(const std::string &index);

	/**
	 * @brief Sets the auto-indexing option.
	 * @param autoIndex true to enable auto-indexing, false to disable.
	 */
	void setAutoIndex(bool autoIndex);

	/**
	 * @brief Sets the upload path.
	 * @param path The upload path.
	 */
	void setUploadPath(const std::string &path);

	/**
	 * @brief Sets the redirection URL.
	 * @param redirect The redirection URL.
	 */
	void setRedirect(const std::string &redirect);

	/**
	 * @brief Sets the CGI file extension.
	 * @param ext The CGI file extension (e.g., ".php").
	 */
	void setCgiExtension(const std::string &ext);

	/**
	 * @brief Sets the CGI executable path.
	 * @param path The path to the CGI executable.
	 */
	void setCgiPath(const std::string &path);

	// =========================================================================
	// Getters

	/**
	 * @brief Gets the path of the location.
	 * @return The path as a string.
	 */
	const std::string &getPath() const;

	/**
	 * @brief Gets the root directory.
	 * @return The root directory as a string.
	 */
	const std::string &getRoot() const;

	/**
	 * @brief Gets the list of allowed HTTP methods.
	 * @return A constant reference to a vector of allowed methods.
	 */
	const std::vector<std::string> &getAllowedMethods() const;

	/**
	 * @brief Gets the list of index files.
	 * @return A constant reference to a vector of index file names.
	 */
	const std::vector<std::string> &getIndex() const;

	/**
	 * @brief Gets the auto-indexing setting.
	 * @return true if auto-indexing is enabled, false otherwise.
	 */
	bool getAutoIndex() const;

	/**
	 * @brief Gets the upload path.
	 * @return The upload path as a string.
	 */
	const std::string &getUploadPath() const;

	/**
	 * @brief Gets the redirection URL.
	 * @return The redirection URL as a string.
	 */
	const std::string &getRedirect() const;

	/**
	 * @brief Gets the CGI file extension.
	 * @return The CGI file extension as a string.
	 */
	const std::string &getCgiExtension() const;

	/**
	 * @brief Gets the CGI executable path.
	 * @return The CGI executable path as a string.
	 */
	const std::string &getCgiPath() const;

	// =========================================================================
	// Methods

	/**
	 * @brief Checks if a given HTTP method is allowed in this location.
	 * @param method The HTTP method to check (e.g., "GET", "POST").
	 * @return true if the method is allowed, false otherwise.
	 */
	bool isMethodAllowed(const std::string &method) const;
};
