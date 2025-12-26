/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:41 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/26 14:44:51 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// =============================================================================
// Includes

#include "HttpResponse.hpp"
#include "../utils/utils.hpp"
#include "../utils/MimeTypes.hpp"

#include <fcntl.h>   // open()
#include <unistd.h> // write(), close(), unlink()

// =============================================================================
// Handlers

// Default constructor
HttpResponse::HttpResponse() :
	_statusCode(200),
	_statusMessage("OK")
{
}

HttpResponse::~HttpResponse()
{
}

// =============================================================================
// Setters

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

// =============================================================================
// Methods

std::string HttpResponse::getStatusMessage(int code) const
{
	switch(code)
	{
		case 200: return "OK";
		case 201: return "Created";
		case 204: return "No Content";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 413: return "Payload Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 504: return "Gateway Timeout";
		default: return "Unknown";
	}
}

std::string HttpResponse::build() const
{
	std::string response;

	// Status line: "HTTP/1.1 200 OK\r\n"
	response = "HTTP/1.1 " + intToString(_statusCode) + " " + _statusMessage + "\r\n";

	// Headers
	std::map<std::string, std::string>::const_iterator it;

	for (it = _headers.begin(); it != _headers.end(); ++it)
		response += it->first + ": " + it->second + "\r\n";

	// Content length auto if body exist
	if (!_body.empty())
		response += "Content-Length: " + intToString(_body.length()) + "\r\n";

	// Separate headers from body
	response += "\r\n";

	// Body
	response += _body;

	return response;
}

void HttpResponse::serveError(int code, const std::string &errorPagePath)
{
	setStatus(code);

	// If custom error page exist from config
	if (!errorPagePath.empty() && fileExists(errorPagePath))
	{
		std::string content = readFile(errorPagePath);
		setHeader("Content-Type", "text/html");
		setBody(content);
		return;
	}

	// Try default error page in www/error_pages/
	std::string defaultErrorPage = "www/error_pages/" + intToString(code) + ".html";
	if (fileExists(defaultErrorPage))
	{
		std::string content = readFile(defaultErrorPage);
		setHeader("Content-Type", "text/html");
		setBody(content);
		return;
	}

	// Default error page
	std::string body =
		"<html>\n"
		"<head><title>Error " + intToString(code) + "</title></head>\n"
		"<body>\n"
		"<h1>Error " + intToString(code) + " - " + getStatusMessage(code) + "</h1>"
		"<p>The requested resource could not be found.</p>"
		"</body>\n"
		"</html>";

	setHeader("Content-Type", "text/html");
	setBody(body);
}

void HttpResponse::serveFile(const std::string &path)
{
    // Check if file exists
    if (!fileExists(path))
    {
        serveError(404, "");
        return;
    }

    // Check if it's a directory
    if (isDirectory(path))
    {
        serveError(403, "");
        return;
    }

    // Read file content
    std::string content = readFile(path);

    // Get MIME type using MimeTypes class
    std::string ext = getFileExtension(path);
    std::string contentType = MimeTypes::get(ext);

    // Build response
    setStatus(200);
    setHeader("Content-Type", contentType);
    setBody(content);
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
        "<head><title>Index of " + uri + "</title></head>\n"
        "<body>\n"
        "<h1>Index of " + uri + "</h1>\n"
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
        
        body += "<li><a href=\"" + href + "\">" + entries[i] + "</a></li>\n";
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

void HttpResponse::serveDelete(const std::string &path)
{
	// Check if file exist
	if (!fileExists(path))
	{
		serveError(404, "");
		return;
	}

	// Check if it directory (cannot del dir)
	if (isDirectory(path))
	{
		serveError(403, "");
		return;
	}

	// Try to delete the file
	if (unlink(path.c_str()) == 0)
		setStatus(204); // Success: 204 No Content
	else
		serveError(500, "");
}

// void HttpResponse::servePut(const std::string &path, const std::string &body)
// {
// 	// Check if file already exists
// 	bool fileExisted = fileExists(path);

// 	// Open file for writing (create or replace)
// 	int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
// 	if (fd < 0)
// 	{
// 		serveError(500, "");
// 		return;
// 	}

// 	// Write body to file
// 	ssize_t written = write(fd, body.c_str(), body.length());
// 	close(fd);

// 	if (written < 0 || (size_t)written != body.length())
// 	{
// 		serveError(500, "");
// 		return;
// 	}

// 	// Success 201 Created if new, 204 No content if replaced
// 	if (fileExisted)
// 		setStatus(204);
// 	else
// 		setStatus(201);
// }

void HttpResponse::handleUpload(const HttpRequest &request, const std::string &uploadDir)
{
	const std::vector<UploadedFile> &files = request.getUploadedFiles();

	if (files.empty())
	{
		serveError(400, ""); // Bad request - no files
		return;
	}

	// Save each file
	for (size_t i = 0; i < files.size(); ++i)
	{
		std::string filePath = uploadDir + '/' + files[i].filename;

		// Open file fpr writing
		int fd = open(filePath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd < 0)
		{
			serveError(500, ""); // Failed to save
			return;
		}

		// Write content
		ssize_t written = write(fd, files[i].content.c_str(), files[i].content.length());
		close(fd);

		if ( written < 0 || (size_t)written != files[i].content.length())
		{
			serveError(500, ""); // Failed to write
			return;
		}
	}

	// Success 201 Created
	setStatus(201);
	setHeader("Content-Type", "text/html");
	setBody("<html><body><h1>Upload successful!</h1></body></html>");
}
