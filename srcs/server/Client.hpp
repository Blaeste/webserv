/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:44 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/09 14:49:25 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

// Include(s) ------------------------------------------------------------------

# include "../http/HttpRequest.hpp"
# include "../http/HttpResponse.hpp"
# include <string>						// std::string
# include <ctime>						// time_t, std::time

// Forward declaration(s) ------------------------------------------------------

class	Location;
class	ServerConfig;
class	Router;
struct	SessionData;
struct	CgiProcess;
struct	CgiResult;

// Enum(s) ---------------------------------------------------------------------

enum	ClientState
{
	STATE_KEEPALIVE, // Reading request or writing response
	STATE_PROCESSING // Processing request (buildResponse, CGI execution)
};

// Class -----------------------------------------------------------------------

class	Client
{
	private:

		// Attribute(s)

		int					_socket;				// Client socket file descriptor
		std::string			_clientIp;				// Client IP address
		HttpRequest			_request;				// Current HTTP request being processed
		HttpResponse		_response;				// HTTP response to be sent to client
		time_t				_lastActivity;			// Timestamp of last client activity
		bool				_requestComplete;		// Indicates if the HTTP request is fully received
		bool				_responseReady;			// Indicates if the HTTP response is ready to send
		bool				_closeAfterResponse;	// Flag to close connection after sending response
		bool				_requestLogged;			// Flag to track if request start was logged
		std::string			_sessionId;				// Session identifier for this client
		ClientState			_state;					// Current state (keepalive or processing)
		CgiProcess*			_CgiProcess;			// Active CGI process (NULL if none)
		std::string			_cachedResponse;		// Cached response for progressive sending
		size_t				_bytesSent;				// Bytes already sent from cached response
		time_t				_cgiStartTime;			// Start time for CGI timeout tracking
		time_t				_requestStartTime;		// Start time for request timing (from first data)
		const ServerConfig*	_serverConfig;			// Server configuration for logging
		std::string			_pendingInput;			// Restes d'une requête suivante déjà reçue

		// Private method(s)

		/** @brief Creates or validates the session cookie and updates the sessions map. */
		void				handleSession(std::map<std::string, SessionData>& sessions);

		/** @brief Sets Connection header to "keep-alive" or "close" based on _closeAfterResponse. */
		void				applyConnectionHeader();

	public:

		// Default constructor

		Client(int socket, const std::string& clientIp);

		// Getter(s)

		int					getSocket() const					{ return _socket; };
		const std::string&	getClientIp() const					{ return _clientIp; };
		const HttpRequest&	getRequest() const					{ return _request; }
		int					getResponseStatus()					{ return _response.getStatus(); }
		size_t				getResponseBodySize() const			{ return _response.getBody().size(); }
		size_t				getRequestBodySize() const			{ return _request.getBody().size(); }
		bool				isRequestComplete() const			{ return _requestComplete; }
		bool				isResponseReady() const				{ return _responseReady; }
		bool				shouldCloseAfterResponse() const	{ return _closeAfterResponse; }
		bool				isRequestLogged() const				{ return _requestLogged; }
		CgiProcess*			getCgiProcess() const				{ return _CgiProcess; }
		time_t				getRequestStartTime() const			{ return _requestStartTime; }

		// Setter(s)

		void				updateActivity()					{ _lastActivity = std::time(NULL); }
		void				setState(ClientState state)			{ _state = state; }
		void				setCgiProcess(CgiProcess* cgi)		{ _CgiProcess = cgi; }
		void				markRequestLogged()					{ _requestLogged = true; }

		// Public method(s)

		/** @brief Returns true if the client has exceeded the idle or processing timeout. */
		bool				hasTimedOut(time_t readTimeout, time_t processingTimeout) const;

		/** @brief Schedules connection closure after the current response is fully sent. */
		void				markCloseAfterResponse();

		/** @brief Records CGI start time and stores config reference for timeout enforcement. */
		void				setCGITiming(const ServerConfig& config);

		// Public method(s)

		/** @brief Reads from socket into the request parser; returns false on disconnect or error. */
		bool				readData(const ServerConfig* config = NULL);

		/** @brief Routes the request and builds the appropriate HTTP response. */
		void				buildResponse(const ServerConfig& config, Router& router, std::map<std::string, SessionData>& sessions);

		/** @brief Builds the response from a completed CGI execution result. */
		void				buildResponseFromCGI(const CgiResult& result);

		/**
		 * @brief Builds an error response, resolving a custom error page from config if available.
		 * @param config Optional — used to look up configured error_page paths.
		 */
		void				buildErrorResponse(int statusCode, const ServerConfig* config = NULL);

		/** @brief Sends the cached response progressively; returns true when fully sent. */
		bool				sendResponse();

		/** @brief Stashes leftover bytes from _rawData (next pipelined request) into _pendingInput. */
		void				stashLeftoverFromRequest();

		/** @brief Resets request/response state to handle the next request on this connection. */
		void				resetForNextRequest();
};

#endif
