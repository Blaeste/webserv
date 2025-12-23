/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:41 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/23 13:48:02 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// =============================================================================
// Includes

#include "HttpResponse.hpp"
#include "../utils/utils.hpp"

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
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
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
	// Check if file exist
	if (!fileExists(path))
	{
		serveError(404, "");
		return;
	}

	// Check if its a directory
	if (isDirectory(path))
	{
		serveError(403, "");
		return;
	}

	// Read file content
	std::string content = readFile(path);

	// Chose content type by file extension
	std::string ext = getFileExtension(path);
	std::string contentType = "text/plain"; // Default

	if (ext == ".html" || ext == ".htm")
		contentType = "text/html";
	else if (ext == ".css")
		contentType = "text/css";
	else if (ext == ".js")
		contentType = "application/javascript";
	else if (ext == ".json")
		contentType = "application/json";
	else if (ext == ".png")
		contentType = "image/png";
	else if (ext == ".jpg" || ext == ".jpeg")
		contentType = "image/jpeg";
	else if (ext == ".gif")
		contentType = "image/gif";
	else if (ext == ".pdf")
		contentType = "application/pdf";

	// Build response
	setStatus(200);
	setHeader("Content-Type", contentType);
	setBody(content);
}

void HttpResponse::serveDirectoryListing(const std::string &path)
{
	// Check if path is a directory
	if (!isDirectory(path))
	{
		serveError(404, "");
		return;
	}

	// Get list of directories/files
	std::vector<std::string> files = listDirectory(path);

	// Generate HTML page
	std::string body =
		"<html>\n"
		"<head><title>Index of " + path + "</title></head>\n"
		"<body>\n"
		"<h1>Index of " + path + "</h1>\n"
		"<hr>\n"
		"<ul>";

	// add each entries as a link
	for (size_t i = 0; i < files.size(); ++i)
		body += "<li><a href=\"" + files[i] + "\">" + files[i] + "</a></li>\n";

	// add the rest of body
	body +=
		"</ul>\n"
		"<hr>"
		"</body>\n"
		"</html>";

	// Build response
	setStatus(200);
	setHeader("Content-Type", "text/html");
	setBody(body);
}
