/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:55 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/11 13:23:58 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s) ******************************************************************
#include <string>
#include <vector>

// Class ***********************************************************************
class Location
{
	private:
		// Attribute(s) --------------------------------------------------------

		std::string _path; ///< Location path prefix
		std::string _root; ///< Root directory for file serving
		std::vector<std::string> _allowedMethods; ///< Allowed HTTP methods
		std::vector<std::string> _index; ///< Default index file names
		bool _autoIndex; ///< Enable directory listing
		std::string _uploadPath; ///< Directory for file uploads
		std::string _redirect; ///< URL redirection target
		std::string _cgiExtension; ///< CGI file extension
		std::string _cgiPath; ///< CGI interpreter path
		size_t _maxBodySize; ///< Max body size (0 = use server default)

	public:
		// Default constructor -------------------------------------------------
		Location();

		// Setters -------------------------------------------------------------
		void setPath(const std::string &path) { _path = path; }
		void setRoot(const std::string &root) { _root = root; }
		void setAutoIndex(bool autoIndex) { _autoIndex = autoIndex; }
		void setUploadPath(const std::string &path) { _uploadPath = path; }
		void setRedirect(const std::string &redirect) { _redirect = redirect; }
		void setCgiExtension(const std::string &ext) { _cgiExtension = ext; }
		void setCgiPath(const std::string &path) { _cgiPath = path; }
		void setMaxBodySize(size_t size) { _maxBodySize = size; }

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

		// Getter(s) -----------------------------------------------------------
		const std::string &getPath() const { return _path; }
		const std::string &getRoot() const { return _root; }
		const std::vector<std::string> &getAllowedMethods() const { return _allowedMethods; }
		const std::vector<std::string> &getIndex() const { return _index; }
		bool getAutoIndex() const { return _autoIndex; }
		const std::string &getUploadPath() const { return _uploadPath; }
		const std::string &getRedirect() const { return _redirect; }
		const std::string &getCgiExtension() const { return _cgiExtension; }
		const std::string &getCgiPath() const { return _cgiPath; }
		size_t getMaxBodySize() const { return _maxBodySize; }

		// Public method(s) ----------------------------------------------------
		/**
		 * @brief Checks if a given HTTP method is allowed in this location.
		 * @param method The HTTP method to check (e.g., "GET", "POST").
		 * @return true if the method is allowed, false otherwise.
		 */
		bool isMethodAllowed(const std::string &method) const;
};
