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
		 */
		void serveFile(const std::string &path, const std::string &root);

		/**
		 * @brief Serves an error page for the given HTTP status code.
		 * @param code The HTTP status code.
		 * @param errorPagePath The path to a custom error page file (optional).
		 * @param allowedMethods List of allowed methods (for 405 responses).
		*/
		void serveError(int code, const std::string &errorPagePath, const std::vector<std::string> &allowedMethods = std::vector<std::string>());

		/**
		 * @brief Serves a directory listing for the given path.
		 * @param path The directory path.
		 */
		void serveDirectoryListing(const std::string &path, const std::string &uri);

		/**
		 * @brief Handles DELETE requests for the given path.
		 * @param path The path to the resource to delete.
		 * @param uploadRoot The upload directory that bounds deletions.
		 */
		void serveDelete(const std::string &path, const std::string &uploadRoot);

		/**
		 * @brief Handles file upload by saving uploaded files to a directory.
		 * @param request The HTTP request containing uploaded files.
		 * @param uploadDir The directory where files should be saved.
		 */
		void handleUpload(const HttpRequest &request, const std::string &uploadDir);

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
