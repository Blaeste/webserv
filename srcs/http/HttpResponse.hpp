/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:33:36 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/02 15:43:39 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s) ******************************************************************
#include <string>	// std::string
#include <map>		// std::map
#include <vector>	// std::vector

// Forward declaration(s) ------------------------------------------------------
class HttpRequest;

// Class ***********************************************************************
class HttpResponse
{
	private:
		// Attribute(s) --------------------------------------------------------
		int _statusCode; ///< HTTP status code
		std::string _statusMessage; ///< HTTP status message
		std::map<std::string, std::string> _headers; ///< Headers
		std::string _body; ///< Body

	public:
		// Default constructor -------------------------------------------------
		HttpResponse();

		// Getter(s) -----------------------------------------------------------
		int getStatus() const { return _statusCode; }
		const std::string &getBody() const { return _body; }

		// Setter(s) -----------------------------------------------------------
		void setHeader(const std::string &key, const std::string &value) { _headers[key] = value; }
		void setBody(const std::string &body) { _body = body; }

		/**
		 * @brief Sets the HTTP status code and corresponding message.
		 * @param code The HTTP status code.
		 */
		void setStatus(int code);

		// Public method(s) ----------------------------------------------------
		/**
		 * @param method The HTTP method (to skip body for HEAD)
		 * @return The raw HTTP response string.
		 */
		std::string build(const std::string &method = "GET") const;

		/**
		 * @brief Serves a static file as the HTTP response body.
		 * @param path The path to the file to serve.
		 * @param root The location root that bounds what can be served.
		 * @return 200 on success, or an HTTP error code (403, 404, 500).
		 */
		int serveFile(const std::string &path, const std::string &root);

		/**
		 * @brief Serves a directory listing for the given path.
		 * @param path The directory path.
		 * @param uri The request URI.
		 * @return 200 on success, or 404 if not a directory.
		 */
		int serveDirectoryListing(const std::string &path, const std::string &uri);

		/**
		 * @brief Handles DELETE requests for the given path.
		 * @param path The path to the resource to delete.
		 * @param uploadRoot The upload directory that bounds deletions.
		 * @return 204 on success, or an HTTP error code (403, 500).
		 */
		int serveDelete(const std::string &path, const std::string &uploadRoot);

		/**
		 * @brief Handles file upload by saving uploaded files to a directory.
		 * @param request The HTTP request containing uploaded files.
		 * @param uploadDir The directory where files should be saved.
		 * @return 201 on success, or an HTTP error code (400, 403, 500).
		 */
		int handleUpload(const HttpRequest &request, const std::string &uploadDir);

		/**
		 * @brief Serves an OPTIONS response with allowed HTTP methods.
		 * @param allowedMethods A vector of allowed HTTP methods for this location.
		 */
		void serveOptions(const std::vector<std::string> &allowedMethods);

	private:
		// Private method(s) ---------------------------------------------------
		/**
		 * @brief Gets the standard status message for a given HTTP status code.
		 * @param code The HTTP status code.
		 * @return The corresponding status message as a string.
		 */
		std::string getStatusMessage(int code) const;
};
