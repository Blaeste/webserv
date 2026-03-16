/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:41 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/16 15:31:37 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------

#include "HttpResponse.hpp"
#include "HttpRequest.hpp"
#include "../utils/Logger.hpp"
#include "../utils/MimeTypes.hpp"
#include "../utils/utils.hpp"
#include <cstdio>					// std::remove
#include <ctime>					// std::time, std::gmtime, std::strftime
#include <dirent.h>					// opendir, readdir, closedir, DIR, struct dirent
#include <fcntl.h>					// open, O_WRONLY, O_CREAT, O_TRUNC, O_NOFOLLOW
#include <unistd.h>					// write

// Default constructor ---------------------------------------------------------

HttpResponse::HttpResponse()
	: _statusCode(200)
	, _statusMessage("OK")
{}

// Setter(s) -------------------------------------------------------------------

void HttpResponse::setStatus(int code)
{
	_statusCode = code;
	_statusMessage = getStatusMessage(code);
}

// Private method(s) -----------------------------------------------------------

std::string HttpResponse::getHttpDate()
{
	time_t now = std::time(NULL);
	struct tm* gmt = std::gmtime(&now);
	if (!gmt)
		return "";
	char buffer[HTTP_DATE_BUFFER_SIZE];
	std::strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", gmt);
	return std::string(buffer);
}

std::string HttpResponse::getStatusMessage(int code)
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
	case 414:
		return "URI Too Long";
	case 431:
		return "Request Header Fields Too Large";
	case 500:
		return "Internal Server Error";
	case 501:
		return "Not Implemented";
	case 504:
		return "Gateway Timeout";
	case 505:
		return "HTTP Version Not Supported";
	default:
		return "Unknown";
	}
}

// Public method(s) ------------------------------------------------------------

std::string HttpResponse::build(const std::string& method) const
{
	std::string response;

	// Status line: "HTTP/1.1 200 OK\r\n"
	response = "HTTP/1.1 " + intToString(_statusCode) + " " + _statusMessage + "\r\n";

	// Standard Headers
	response += "Server: webserv/1.0\r\n";
	response += "Date: " + getHttpDate() + "\r\n";
	if (_headers.find("Connection") == _headers.end())
		response += "Connection: close\r\n"; // default if not specified

	// Custom Headers
	headerMap::const_iterator it;

	for (it = _headers.begin(); it != _headers.end(); ++it)
		response += it->first + ": " + it->second + "\r\n";

	// Content-Length (always send except for 204 No Content and 304 Not Modified)
	if (_statusCode != 204 && _statusCode != 304)
		response += "Content-Length: " + intToString(_body.size()) + "\r\n";

	// Separate headers from body
	response += "\r\n";

	// Body (skip for HEAD method)
	if (method != "HEAD")
		response += _body;

	return response;
}

int HttpResponse::serveFile(const std::string& path, const std::string& root)
{
	// Check if it's a directory + security check
	if (isDirectory(path) || !isPathSafe(path, root))
		return 403;

	// Check if file exists
	if (!fileExists(path))
		return 404;

	try
	{
		// Read file content
		std::string content = readFile(path, "HttpResponse");

		// Get MIME type (default to application/octet-stream if no extension)
		std::string contentType = "application/octet-stream";

		try
		{
			std::string ext = getFileExtension(path);
			contentType = MimeTypes::get(ext);
		}
		catch (const std::exception&)
		{
			// No extension or hidden file → use default MIME type
		}

		// Build response
		setStatus(200);
		setHeader("Content-Type", contentType);
		setBody(content);
		return 200;
	}
	catch (const std::exception& e)
	{
		Logger::logMessage(RED "[HttpResponse] Error: " RESET "serveFile: " + std::string(e.what()));
		return 500;
	}
}

// Security: Escape HTML special characters to prevent XSS injection
// Converts &, <, >, " to their HTML entities
static std::string htmlEscape(const std::string& s)
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

static stringVector listDirectory(const std::string& path)
{
	stringVector entries;

	DIR* dir = opendir(path.c_str());
	if (!dir)
	{
		Logger::logMessage(RED "[HttpResponse] Error: " RESET "listDirectory: opendir failed for path: " + path);
		return entries;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)))
	{
		std::string name = entry->d_name;
		if (name != "." && name != "..")
			entries.push_back(name);
	}

	closedir(dir);
	return entries;
}

int HttpResponse::serveDirectoryListing(const std::string& path, const std::string& uri)
{
	// Check if path is a directory
	if (!isDirectory(path))
		return 404;

	// Get list of files/directories
	stringVector entries = listDirectory(path);

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
	return 200;
}

int HttpResponse::serveDelete(const std::string& path, const std::string& uploadRoot)
{
	// Check if file exists and not a directory
	if (isDirectory(path) || !isPathSafe(path, uploadRoot))
		return 403;

	// Try to delete the file
	if (!std::remove(path.c_str()))
	{
		setStatus(204); // Success: 204 No Content
		return 204;
	}
	else
	{
		Logger::logMessage(RED "[HttpResponse] Error: " RESET "serveDelete: remove failed for " + path);
		return 500;
	}
}

// Security: Sanitize filename to prevent path traversal and injection attacks
// Replaces dangerous characters: "..", "/", "\", null bytes, control chars
static std::string sanitizeFilename(const std::string& filename)
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
static bool writeAll(int fd, const char* buf, size_t len)
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

int HttpResponse::handleUpload(const HttpRequest& request, const std::string& uploadDir)
{

	// Security check: only allow uploads inside the configured upload directory
	if (!isPathSafe(uploadDir, uploadDir))
		return 403;

	const std::vector<UploadedFile>& files = request.getUploadedFiles();

	if (files.empty())
		return 400;

	// Save each file
	for (size_t i = 0; i < files.size(); ++i)
	{

		std::string safeName = sanitizeFilename(files[i].filename);
		std::string filePath = uploadDir + '/' + safeName;
		if (!isPathSafe(filePath, uploadDir))
			return 403;

		// Open file for writing
		// Security: O_NOFOLLOW prevents symlink attacks (don't follow symbolic links)
		int fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW, UPLOAD_FILE_PERMISSIONS);

		if (fd < 0)
		{
			Logger::logMessage(RED "[HttpResponse] Error: " RESET "handleUpload: open failed: " + filePath);
			return 500;
		}

		// Write content (handle partial write)
		if (!writeAll(fd, files[i].content.data(), files[i].content.size()))
		{
			safeClose(fd, "HttpResponse");
			Logger::logMessage(RED "[HttpResponse] Error: " RESET "handleUpload: write failed: " + filePath);
			return 500;
		}
		safeClose(fd, "HttpResponse");
	}

	// Success 201 Created
	setStatus(201);
	setHeader("Content-Type", "text/html");
	setBody("<html><body><h1>Upload successful!</h1></body></html>");
	return 201;
}

void HttpResponse::serveOptions(const stringVector& allowedMethods)
{
	// Build the Allow header with comma-separated methods
	std::string allow;
	for (size_t i = 0; i < allowedMethods.size(); ++i)
	{
		if (i > 0)
			allow += ", ";
		allow += allowedMethods[i];
	}

	setStatus(200);
	setHeader("Allow", allow);
	setHeader("Content-Type", "text/plain");
	setBody("");
}
