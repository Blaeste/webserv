/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lmarck <lmarck@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:33:36 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/09 19:30:32 by lmarck           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s)
#include <string>
#include <map>


// Forward declaration(s)
class HttpRequest;

// Class
class HttpResponse
{

	private:

		// Attribute(s)

			int _statusCode; // HTTP status code
			std::string _statusMessage; // HTTP status message
			std::map<std::string, std::string> _headers; // Headers
			std::string _body; // Body

	public:

		// Default constructor

			HttpResponse();

		// Public method(s)

			int getStatus();

			const std::string &getBody() const;

			/**
			 * @brief Sets the HTTP status code and corresponding message.
			 * @param code The HTTP status code.
			 */
			void setStatus(int code);

			/**
			 * @brief Sets a header key-value pair.
			 * @param key The header field name.
			 * @param value The header field value.
			 */
			void setHeader(const std::string &key, const std::string &value);

			/**
			 * @brief Sets the body of the HTTP response.
			 * @param body The body content as a string.
			 */
			void setBody(const std::string &body);

			/**
			 * @brief Builds the complete HTTP response as a raw string.
			 * @return The raw HTTP response string.
			 */
			std::string build() const;

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
			 */
			void serveError(int code, const std::string &errorPagePath);

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

	private:

		// Private method(s)

			/**
			 * @brief Gets the standard status message for a given HTTP status code.
			 * @param code The HTTP status code.
			 * @return The corresponding status message as a string.
			 */
			std::string getStatusMessage(int code) const;

};
