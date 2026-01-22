/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lmarck <lmarck@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:41 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/21 14:36:35 by lmarck           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "HttpResponse.hpp"
#include "HttpRequest.hpp"
#include "../utils/utils.hpp"
#include "../utils/MimeTypes.hpp"
#include <cstdio>  // remove()
#include <fcntl.h> // open()
#include <iostream>
#include <unistd.h> // write(), close()
#include <cerrno>

// Default constructor
HttpResponse::HttpResponse() : _statusCode(200),
							   _statusMessage("OK")
{
}

// Setter(s)
void HttpResponse::setStatus(int code)
{
	_statusCode = code;
	_statusMessage = getStatusMessage(code);
}

void HttpResponse::setHeader(const std::string &key, const std::string &value)
{
	_headers[key] = value;
}

void HttpResponse::setBody(const std::string &body)
{
	_body = body;
}

// Getters(s)
int HttpResponse::getStatus()
{
	return _statusCode;
}

const std::string &HttpResponse::getBody() const
{
	return _body;
}

// Private Method(s)
std::string HttpResponse::getStatusMessage(int code) const
{
	switch (code)
	{
	case 200:
		return "OK";
	case 201:
		return "Created";
	case 204:
		return "No Content";
	case 301:
		return "Moved Permanently";
	case 302:
		return "Found";
	case 400:
		return "Bad Request";
	case 403:
		return "Forbidden";
	case 404:
		return "Not Found";
	case 405:
		return "Method Not Allowed";
	case 413:
		return "Payload Too Large";
	case 500:
		return "Internal Server Error";
	case 501:
		return "Not Implemented";
	case 504:
		return "Gateway Timeout";
	default:
		return "Unknown";
	}
}

// Public Method(s)
std::string HttpResponse::build() const
{
	std::string response;

	// Status line: "HTTP/1.1 200 OK\r\n"
	response = "HTTP/1.1 " + intToString(_statusCode) + " " + _statusMessage + "\r\n";

	// Standard Headers
	response += "Server: webserv/1.0\r\n";
	response += "Date: " + getHttpDate() + "\r\n";
	response += "Connection: close\r\n";

	// Custom Headers
	std::map<std::string, std::string>::const_iterator it;

	for (it = _headers.begin(); it != _headers.end(); ++it)
		response += it->first + ": " + it->second + "\r\n";

	// Content-Length (always send except for 204 No Content and 304 Not Modified)
	if (_statusCode != 204 && _statusCode != 304)
		response += "Content-Length: " + intToString(_body.size()) + "\r\n";

	// Separate headers from body
	response += "\r\n";

	// Body
	response += _body;

	return response;
}

void HttpResponse::serveError(int code, const std::string &errorPagePath)
{
	setStatus(code);

	try
	{
		if (!errorPagePath.empty() && fileExists(errorPagePath))
		{
			std::string content = readFile(errorPagePath);
			setHeader("Content-Type", "text/html");
			setBody(content);
			return;
		}

		std::string defaultErrorPage = "www/error_pages/" + intToString(code) + ".html";
		if (fileExists(defaultErrorPage))
		{
			std::string content = readFile(defaultErrorPage);
			setHeader("Content-Type", "text/html");
			setBody(content);
			return;
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "[HttpResponse] readFile error: " << e.what() << std::endl;
	}

	// Default error page
	std::string body =
		"<html>\n"
		"<head><title>Error " +
		intToString(code) + "</title></head>\n"
							"<body>\n"
							"<h1>Error " +
		intToString(code) + " - " + getStatusMessage(code) + "</h1>"
															 "<p>The requested resource could not be found.</p>"
															 "</body>\n"
															 "</html>";

	setHeader("Content-Type", "text/html");
	setBody(body);
}

void HttpResponse::serveFile(const std::string &path, const std::string &root)
{
	// Check if it's a directory + security check
	if (isDirectory(path) || !isPathSafe(path, root))
	{
		serveError(403, "");
		return;
	}

	// Check if file exists
	if (!fileExists(path))
	{
		serveError(404, "");
		return;
	}

	try
	{
		// Read file content
		std::string content = readFile(path);

		// Get MIME type (default to application/octet-stream if no extension)
		std::string contentType = "application/octet-stream";

		try
		{
			std::string ext = getFileExtension(path);
			contentType = MimeTypes::get(ext); // Assign, not declare!
		}
		catch (const std::exception &)
		{
			// No extension or hidden file → use default MIME type
		}

		// Build response
		setStatus(200);
		setHeader("Content-Type", contentType);
		setBody(content);
	}
	catch (const std::exception &e)
	{
		std::cerr << "[HttpResponse] serveFile error: " << e.what() << std::endl;
		serveError(500, "");
	}
}

// Security: Escape HTML special characters to prevent XSS injection
// Converts &, <, >, " to their HTML entities
static std::string htmlEscape(const std::string &s)
{
	std::string out;

	for (size_t i = 0; i < s.size(); i++)
	{

		char c = s[i];
		if (c == '&')
			out += "&amp;";
		else if (c == '<')
			out += "&lt;";
		else if (c == '>')
			out += "&gt;";
		else if (c == '"')
			out += "&quot;";
		else
			out += c;
	}
	return out;
}

void HttpResponse::serveDirectoryListing(const std::string &path, const std::string &uri)
{
	// Check if path is a directory
	if (!isDirectory(path))
	{
		serveError(404, "");
		return;
	}

	// Get list of files/directories
	std::vector<std::string> entries = listDirectory(path);

	// Generate HTML page with correct URI for links
	std::string body =
		"<html>\n"
		"<head><title>Index of " +
		uri + "</title></head>\n"
			  "<body>\n"
			  "<h1>Index of " +
		uri + "</h1>\n"
			  "<hr>\n"
			  "<ul>";

	// Add each entry as a clickable link
	for (size_t i = 0; i < entries.size(); ++i)
	{
		std::string href = uri;
		// Add trailing slash if needed
		if (!uri.empty() && uri[uri.length() - 1] != '/')
			href += "/";
		href += entries[i];

		std::string escapedName = htmlEscape(entries[i]);
		std::string escapedHref = htmlEscape(href);
		body += "<li><a href=\"" + escapedHref + "\">" + escapedName + "</a></li>\n";
	}

	body +=
		"</ul>\n"
		"<hr>\n"
		"</body>\n"
		"</html>";

	// Build response
	setStatus(200);
	setHeader("Content-Type", "text/html");
	setBody(body);
}

void HttpResponse::serveDelete(const std::string &path, const std::string &uploadRoot)
{
	// Check if file exists and not a directory
	if (isDirectory(path) || !isPathSafe(path, uploadRoot))
	{
		serveError(403, "");
		return;
	}

	// Try to delete the file
	if (!std::remove(path.c_str()))
		setStatus(204); // Success: 204 No Content
	else
	{
		std::cerr << "[HttpResponse] serveDelete: remove failed for " << path << std::endl;
		serveError(500, "");
	}
}

// Security: Sanitize filename to prevent path traversal and injection attacks
// Replaces dangerous characters: "..", "/", "\", null bytes, control chars
static std::string sanitizeFilename(const std::string &filename)
{
	std::string safe = filename;

	// Block ".." (directory traversal)
	size_t pos = 0;
	while ((pos = safe.find("..", pos)) != std::string::npos)
		safe.replace(pos, 2, "__");

	for (size_t i = 0; i < safe.size(); i++)
	{
		// Replace "/" and "\" with "_"
		if (safe[i] == '/' || safe[i] == '\\')
			safe[i] = '_';

		// Block dangerous character (null bytes, control chars)
		if (safe[i] == '\0' || safe[i] < 32)
			safe[i] = '_';
	}

	if (safe.empty())
		safe = "unnamed_file";

	return safe;
}

// Security: Write all data handling partial writes and EINTR interruptions
// Returns true if all data written successfully, false on error
static bool writeAll(int fd, const char *buf, size_t len)
{
	size_t off = 0;

	while (off < len)
	{
		ssize_t w = write(fd, buf + off, len - off);

		if (w <= 0)
			return false;
		off += (size_t)w;
	}
	return true;
}

void HttpResponse::handleUpload(const HttpRequest &request, const std::string &uploadDir)
{

	// Security check: only allow uploads inside the configured upload directory
	if (!isPathSafe(uploadDir, uploadDir))
	{
		serveError(403, "");
		return;
	}

	const std::vector<UploadedFile> &files = request.getUploadedFiles();

	if (files.empty())
	{
		serveError(400, ""); // Bad request - no files
		return;
	}

	// Save each file
	for (size_t i = 0; i < files.size(); ++i)
	{

		std::string safeName = sanitizeFilename(files[i].filename);
		std::string filePath = uploadDir + '/' + safeName;
		if (!isPathSafe(filePath, uploadDir))
		{
			serveError(403, "");
			return;
		}

		// Open file for writing
		// Security: O_NOFOLLOW prevents symlink attacks (don't follow symbolic links)
		int fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, 0644);

		if (fd < 0)
		{
			std::cerr << "[handleUpload] open failed: " << filePath << std::endl;
			serveError(500, ""); // Failed to save
			return;
		}

		// Write content (handle partial write)
		if (!writeAll(fd, files[i].content.data(), files[i].content.size()))
		{
			safeClose(fd);
			std::cerr << "[handleUpload] write failed: " << filePath << std::endl;
			serveError(500, "");
			return;
		}
		safeClose(fd);
	}

	// Success 201 Created
	setStatus(201);
	setHeader("Content-Type", "text/html");
	setBody("<html><body><h1>Upload successful!</h1></body></html>");
}
