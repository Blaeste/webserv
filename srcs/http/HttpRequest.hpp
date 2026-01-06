/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:27 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/06 12:32:26 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Includes
#include <string>
#include <vector>
#include <map>

struct UploadedFile
{
	std::string filename;
	std::string contentType;
	std::string content;
};

class HttpRequest
{

private:

	// Attribute(s)
	std::string _method; // Method (GET, POST, etc.)
	std::string _uri; // URI (/index.html, /api/data, etc.)
	std::string _version; // HTTP version (HTTP/1.1)
	std::map<std::string, std::string> _headers; // Headers
	std::string _body; // Body
	std::string _rawData; // Raw request data
	bool _isComplete; // Is the request complete
	std::vector<UploadedFile> _uploadedFiles; // Uploaded files (for multipart/form-data)

public:

	// Default constructor
	HttpRequest();

	// Public method(s)

		/**
		 * @brief Appends raw data to the HTTP request and attempts to parse it.
		 * @param data The raw data to append.
		 * @return true if the request is complete after appending, false otherwise.
		 */
		bool appendData(const std::string &data);

	/**
		 * @brief Checks if the HTTP request is complete.
		 * @return true if the request is complete, false otherwise.
		 */
		bool isComplete() const;

		/**
		 * @brief Gets a specific header value by key.
		 * @param key The header name (e.g., "Content-Type").
		 * @return The header value if found, empty string otherwise.
		 */
		std::string getHeader(const std::string &key) const;

	// Getters
	const std::string &getMethod() const { return _method; }
	const std::string &getUri() const { return _uri; }
	const std::string &getVersion() const { return _version; }
	const std::string& getBody() const { return _body; }
	const std::map<std::string, std::string> &getHeaders() const { return _headers; }
	const std::vector<UploadedFile> &getUploadedFiles() const { return _uploadedFiles; }
	std::map<std::string, std::string> getCookies() const;

private:

	// Private method(s)

		/**
		 * @brief Parses the request line from the header block.
		 * @param headerBlock The header block containing the request line.
		 * @return true if parsing was successful, false otherwise.
		 */
		bool parse();

		/**
		 * @brief Parses the request line from the header block.
		 * @param headerBlock The header block containing the request line.
		 * @return true if parsing was successful, false otherwise.
		 */
		bool parseRequestLine(const std::string &headerBlock);

		/**
		 * @brief Parses the headers from the header block.
		 * @param headerBlock The header block containing the headers.
		 * @return true if parsing was successful, false otherwise.
		 */
		bool parseHeaders(const std::string &headerBlock);

		/**
		 * @brief Parses the body of a chunked transfer encoded request.
		 * @return true if parsing was successful, false otherwise.
		 */
		bool parseChunked();

		/**
		 * @brief Parses multipart/form-data body.
		 * @param boundary The boundary string used to separate parts.
		 * @return true if parsing was successful, false otherwise.
		 */
		bool parseMultipart(const std::string &boundary);

};
