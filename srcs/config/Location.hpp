/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:55 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/17 13:44:14 by eschwart         ###   ########.fr       */
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
	Location();
	~Location();

	// =========================================================================
	// Setters => set replace val, add add val
	void setPath(const std::string &path);
	void setRoot(const std::string &root);
	void addAllowedMethod(const std::string &method);
	void addIndex(const std::string &index);
	void setAutoIndex(bool autoIndex);
	void setUploadPath(const std::string &path);
	void setRedirect(const std::string &redirect);
	void setCgiExtension(const std::string &ext);
	void setCgiPath(const std::string &path);

	// =========================================================================
	// Getters
	const std::string &getPath() const;
	const std::string &getRoot() const;
	const std::vector<std::string> &getAllowedMethods() const;
	const std::vector<std::string> &getIndex() const;
	bool getAutoIndex() const;
	const std::string &getUploadPath() const;
	const std::string &getRedirect() const;
	const std::string &getCgiExtension() const;
	const std::string &getCgiPath() const;

	// =========================================================================
	// Methods
	bool isMethodAllowed(const std::string &method) const;
};
