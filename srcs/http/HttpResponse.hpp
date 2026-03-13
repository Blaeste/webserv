/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpResponse.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:33:36 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/13 11:16:03 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPRESPONSE_HPP
# define HTTPRESPONSE_HPP

// Include(s) ------------------------------------------------------------------

# include "../webserv.hpp"
# include <string>			// std::string

// Fallback define(s) - Only on Linux / Mac ------------------------------------

#ifndef O_NOFOLLOW
# define O_NOFOLLOW 0
#endif

// Forward declaration(s) ------------------------------------------------------

class	HttpRequest;

// Class -----------------------------------------------------------------------

class	HttpResponse
{
	private:

		// Constant(s)

		static const size_t	HTTP_DATE_BUFFER_SIZE	= 100;	// "Www, DD Mmm YYYY HH:MM:SS GMT\0" fits in 100 bytes
		static const int	UPLOAD_FILE_PERMISSIONS	= 0644;	// Octal: rw-r--r--

		// Attribute(s)

		int					_statusCode;	// HTTP status code
		std::string			_statusMessage;	// HTTP status message
		headerMap			_headers;		// Headers
		std::string			_body;			// Body

		// Private method(s)

		/** @brief Returns the standard HTTP reason phrase for the given status code. */
		static std::string	getStatusMessage(int code);

		/** @brief Returns the current UTC date formatted for the HTTP Date header. */
		static std::string	getHttpDate();

	public:

		// Default constructor
		
		HttpResponse();

		// Getter(s)

		int					getStatus() const											{ return _statusCode; }
		const std::string&	getBody() const												{ return _body; }

		// Setter(s)

		void				setHeader(const std::string& key, const std::string& value)	{ _headers[key] = value; }
		void				setBody(const std::string& body)							{ _body = body; }
		void				setStatus(int code);

		// Public method(s)

		/** @brief Serializes the response to a raw HTTP/1.1 string; omits body for HEAD. */
		std::string			build(const std::string& method = "GET") const;

		/**
		 * @brief Serves a static file as the HTTP response body.
		 * @param path The path to the file to serve.
		 * @param root The location root that bounds what can be served.
		 * @return 200 on success, or an HTTP error code (403, 404, 500).
		 */
		int					serveFile(const std::string& path, const std::string& root);

		/** @brief Generates an HTML directory listing; returns 200, or 404 if not a directory. */
		int					serveDirectoryListing(const std::string& path, const std::string& uri);

		/** @brief Deletes the file at path; returns 204, or 403/500 on error. */
		int					serveDelete(const std::string& path, const std::string& uploadRoot);

		/** @brief Saves uploaded files to uploadDir; returns 201, or 400/403/500 on error. */
		int					handleUpload(const HttpRequest& request, const std::string& uploadDir);

		/** @brief Responds to OPTIONS with the Allow header listing the provided methods. */
		void				serveOptions(const stringVector& allowedMethods);
};

#endif
