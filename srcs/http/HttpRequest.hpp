/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:27 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/10 13:54:11 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTPREQUEST_HPP
# define HTTPREQUEST_HPP

// Include(s) ------------------------------------------------------------------

# include "../webserv.hpp"
# include <string>			// std::string
# include <vector>			// std::vector

// Structure(s) ----------------------------------------------------------------

struct	UploadedFile
{
	std::string	filename;	 // Original filename from client
	std::string	contentType; // MIME type (e.g., "image/png")
	std::string	content;	 // Raw file content
};

// Typedef(s) ------------------------------------------------------------------

typedef	std::vector<UploadedFile>	fileVector;

// Class -----------------------------------------------------------------------

class	HttpRequest
{
	private:

		// Constant(s)

		static const size_t	MAX_REQUEST_SIZE		= 110 * 1048576;	// 110 MB
		static const size_t	MAX_URI_LENGTH			= 8192;
		static const size_t	MAX_HEADER_COUNT		= 100;
		static const size_t	MAX_HEADER_SIZE			= 8192;
		static const size_t	MAX_REQUEST_LINE_SIZE	= 16384;
		static const size_t	MAX_METHOD_LENGTH		= 16;
		static const size_t	MAX_CHUNK_SIZE			= 5 * 1048576;		// 5 MB
		static const size_t	MAX_BODY_SIZE			= 110 * 1048576;	// 110 MB

		// Attribute(s)

		std::string			_method;					// Method (GET, POST, etc.)
		std::string			_uri;						// URI (/index.html, /api/data, etc.)
		std::string			_version;					// HTTP version (HTTP/1.1)
		headerMap			_headers;					// Headers
		std::string			_body;						// Body
		std::string			_rawData;					// Raw request data
		bool				_isComplete;				// Is the request complete
		int					_errorCode;					// HTTP error code
		fileVector			_uploadedFiles;				// Uploaded files (for multipart/form-data)
		bool				_headersParsed;				// Have we already parsed the headers
		size_t				_bodyStart;					// Offset to body start inside _rawData
		bool				_isChunked;					// Transfer-Encoding: chunked detected
		size_t				_contentLength;				// Parsed Content-Length when present
		size_t				_chunkParsePos;				// Cursor while parsing chunked body
		size_t				_chunkTotalSize;			// Accumulated chunk payload size
		bool				_chunkDone;					// Final chunk parsed
		size_t				_consumedBytes;				// Octets consommés dans _rawData pour cette requête

		// Private method(s)

		/** @brief Entry point: drives header and body parsing on _rawData. */
		bool				parse();
	
		/** @brief Parses the first line (method, URI, version) from the header block. */
		bool				parseRequestLine(const std::string& headerBlock);
	
		/** @brief Parses header fields from the header block into _headers. */
		bool				parseHeaders(const std::string& headerBlock);
	
		/** @brief Parses a chunked Transfer-Encoding body and accumulates decoded payload. */
		bool				parseChunked();
	
		/** @brief Parses a multipart/form-data body and populates _uploadedFiles. */
		bool				parseMultipart(const std::string& boundary);
	
		/** @brief Stores error code and marks the request complete to trigger an error response. */
		bool				setError(int code);

	public:

		// Default constructor

		HttpRequest();

		// Getter(s)

		const std::string&	getMethod() const			{ return _method; }
		const std::string&	getUri() const				{ return _uri; }
		const std::string&	getVersion() const			{ return _version; }
		const std::string&	getBody() const				{ return _body; }
		const headerMap&	getHeaders() const			{ return _headers; }
		const fileVector&	getUploadedFiles() const	{ return _uploadedFiles; }
		int					getErrorCode() const		{ return _errorCode; }
		bool				headersParsed() const		{ return _headersParsed; }
		size_t				getContentLength() const	{ return _contentLength; }
		bool				isChunked() const			{ return _isChunked; }
		bool				isComplete() const			{ return _isComplete; }
		
		cookieMap			getCookies() const;
		std::string			getHeader(const std::string& key) const;
		std::string			getLeftover() const;

		// Public method(s)

		/** @brief Appends raw socket data and triggers parsing; returns true when request is complete. */
		bool				appendData(const std::string& data);

};

#endif
