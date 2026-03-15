/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:44 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/14 22:07:14 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
# define CLIENT_HPP

// Include(s) ------------------------------------------------------------------

# include "../http/HttpRequest.hpp"
# include "../http/HttpResponse.hpp"
# include "../utils/Logger.hpp"
# include <string>						// std::string
# include <ctime>						// time_t, std::time

// Forward declaration(s) ------------------------------------------------------

class	Location;
class	ServerBlock;
class	Router;
struct	RouteMatch;
struct	SessionData;
struct	CgiProcess;
struct	CgiResult;

// Enum(s) ---------------------------------------------------------------------

enum	ClientState
{
	STATE_KEEPALIVE,			// Reading request or writing response
	STATE_PROCESSING			// Processing request (buildResponse, CGI execution)
};

// Typedef(s) ------------------------------------------------------------------

typedef	std::map<std::string, SessionData>	SessionMap;

// Class -----------------------------------------------------------------------

class	Client
{
	private:

		// Constant(s)

		static const size_t		SESSION_ID_LENGTH = 32;	// Number of random characters in a generated session ID

		// Attribute(s)

		int						_socket;				// Client socket file descriptor
		std::string				_clientIp;				// Client IP address
		HttpRequest				_request;				// Current HTTP request being processed
		HttpResponse			_response;				// HTTP response to be sent to client
		time_t					_lastActivity;			// Timestamp of last client activity
		bool					_requestComplete;		// Indicates if the HTTP request is fully received
		bool					_responseReady;			// Indicates if the HTTP response is ready to send
		bool					_closeAfterResponse;	// Flag to close connection after sending response
		bool					_requestLogged;			// Flag to track if request start was logged
		std::string				_sessionId;				// Session identifier for this client
		ClientState				_state;					// Current state (keepalive or processing)
		CgiProcess*				_CgiProcess;			// Active CGI process (NULL if none)
		std::string				_cachedResponse;		// Cached response for progressive sending
		size_t					_bytesSent;				// Bytes already sent from cached response
		time_t					_cgiStartTime;			// Start time for CGI timeout tracking
		time_t					_requestStartTime;		// Start time for request timing (from first data)
		const ServerBlock*		_server;				// Server configuration for logging
		RequestInfo				_logInfo;				// Cached request info for logging (set once, reused at end)

		// Private method(s)

		/** @brief Generates a cryptographically random session ID of SESSION_ID_LENGTH characters. */
		static std::string		generateSessionId();

		/** @brief Sets Connection header to "keep-alive" or "close" based on _closeAfterResponse. */
		void					applyConnectionHeader();

		/** @brief Handles the /counter-api endpoint, returning session visit count as JSON. */
		void					handleCounterApi(SessionMap& sessions);

		/** @brief Dispatches a validated request to the appropriate handler based on method and route. */
		void					dispatchRequest(const ServerBlock& server, const RouteMatch& match);

		/** @brief Creates a new session, sets its cookie header, and assigns it to the client. */
		void					createSession(SessionMap& sessions, const std::string& sessionId);

		/** @brief Creates or validates the session cookie and updates the sessions map. */
		void					handleSession(SessionMap& sessions);


	public:

		// Default constructor

		Client(int socket, const std::string& clientIp);

		// Getter(s)

		int						getSocket() const					{ return _socket; };
		const std::string&		getClientIp() const					{ return _clientIp; };
		const HttpRequest&		getRequest() const					{ return _request; }
		int						getResponseStatus() const			{ return _response.getStatus(); }
		size_t					getResponseBodySize() const			{ return _response.getBody().size(); }
		size_t					getRequestBodySize() const			{ return _request.getBody().size(); }
		bool					isRequestComplete() const			{ return _requestComplete; }
		bool					isResponseReady() const				{ return _responseReady; }
		bool					shouldCloseAfterResponse() const	{ return _closeAfterResponse; }
		bool					isRequestLogged() const				{ return _requestLogged; }
		CgiProcess*				getCgiProcess() const				{ return _CgiProcess; }
		time_t					getRequestStartTime() const			{ return _requestStartTime; }
		const RequestInfo&		getLogInfo() const					{ return _logInfo; }

		// Setter(s)

		void					updateActivity()					{ _lastActivity = std::time(NULL); }
		void					setState(ClientState state)			{ _state = state; }
		void					setCgiProcess(CgiProcess* cgi)		{ _CgiProcess = cgi; }

		// Public method(s)

		/** @brief Returns true if the client has exceeded the idle or processing timeout. */
		bool					hasTimedOut(time_t readTimeout, time_t processingTimeout) const;

		/** @brief Schedules connection closure after the current response is fully sent. */
		void					markCloseAfterResponse();

		/** @brief Records CGI start time and stores config reference for timeout enforcement. */
		void					setCgiTiming(const ServerBlock& server);

		/** @brief Records the server name and port for request logging at the start of processing. */
		void					logRequestStart(const std::string& serverName, int port);

		/** @brief Completes the request log with end-of-request metrics and sends it to the logger. */
		void					logRequestEnd();

		/** @brief Reads from socket into the request parser; returns false on disconnect or error. */
		bool					readData(const ServerBlock* server = NULL);

		/** @brief Routes the request and builds the appropriate HTTP response. */
		void					buildResponse(const ServerBlock& server, Router& router, SessionMap& sessions);

		/** @brief Builds the response from a completed CGI execution result. */
		void					buildResponseFromCGI(const CgiResult& result);

		/**
		 * @brief Builds an error response, resolving a custom error page from server if available.
		 * @param server Optional — used to look up configured error_page paths.
		 */
		void					buildErrorResponse(int statusCode, const ServerBlock* server = NULL);

		/** @brief Sends the cached response progressively; returns true when fully sent. */
		bool					sendResponse();

		/** @brief Resets request/response state to handle the next request on this connection. */
		void					resetForNextRequest();
};

#endif
