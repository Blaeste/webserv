/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:44 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/13 10:27:43 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s) ******************************************************************
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include <ctime>
#include <string>

// Forward declaration(s) ------------------------------------------------------
class ServerConfig;
class Router;
struct SessionData;
struct CGIProcess;
struct CGIResult;

// Enum(s) *********************************************************************
enum ClientState
{
	STATE_KEEPALIVE, // Reading request or writing response
	STATE_PROCESSING // Processing request (buildResponse, CGI execution)
};

// Class ***********************************************************************
class Client
{
	private:
		// Attribute(s) --------------------------------------------------------
		int _socket; ///< Client socket file descriptor
		std::string _clientIp; ///< Client IP address
		HttpRequest _request; ///< Current HTTP request being processed
		HttpResponse _response; ///< HTTP response to be sent to client
		time_t _lastActivity;  ///< Timestamp of last client activity
		bool _requestComplete;  ///< Indicates if the HTTP request is fully received
		bool _responseReady;  ///< Indicates if the HTTP response is ready to send
		bool _closeAfterResponse;  ///< Flag to close connection after sending response
		bool _requestLogged;  ///< Flag to track if request start was logged
		std::string _sessionId; ///< Session identifier for this client
		ClientState _state;  ///< Current state (keepalive or processing)
		CGIProcess *_cgiProcess; ///< Active CGI process (NULL if none)
		std::string _cachedResponse; ///< Cached response for progressive sending
		size_t _bytesSent; ///< Bytes already sent from cached response
		timeval _cgiStartTime; ///< Start time for CGI timeout tracking
		timeval _requestStartTime; ///< Start time for request timing (from first data)
		const ServerConfig *_serverConfig;  ///< Server configuration for logging

	public:
		// Default constructor -------------------------------------------------
		Client(int socket, const std::string &clientIp);

		// Getter(s) -----------------------------------------------------------
		int getSocket() const { return _socket; };
		const std::string &getClientIp() const { return _clientIp; };
		const HttpRequest &getRequest() const { return _request; }
		int getResponseStatus() { return _response.getStatus(); }
		size_t getResponseBodySize() const { return _response.getBody().size(); }
		bool isRequestComplete() const { return _requestComplete; }
		bool isResponseReady() const { return _responseReady; }
		bool shouldCloseAfterResponse() const { return _closeAfterResponse; }
		CGIProcess *getCGIProcess() const { return _cgiProcess; }
		const timeval &getRequestStartTime() const { return _requestStartTime; }

		// Setter(s) -----------------------------------------------------------
		void updateActivity() { _lastActivity = time(NULL); }
		void setState(ClientState state) { _state = state; }
		void setCGIProcess(CGIProcess *cgi) { _cgiProcess = cgi; }

		/**
		 * @brief Checks if the client connection has timed out
		 * @param readTimeout Timeout in seconds for reading requests
		 * @param processingTimeout Timeout in seconds for processing requests
		 * @return true if the client has exceeded the applicable timeout, false otherwise
		 */
		bool hasTimedOut(time_t readTimeout, time_t processingTimeout) const;

		/**
		 * @brief Marks this client connection to be closed after the response is sent
		 */
		void markCloseAfterResponse();

		/**
		 * @brief Sets the CGI start time for timeout tracking
		 * @param config Server configuration containing CGI timeout settings
		 */
		void setCGITiming(const ServerConfig &config);

		// Public method(s) ----------------------------------------------------
		/**
		 * @brief Reads incoming data from the client socket and parses the HTTP request
		 * @return true if data was successfully read and more data can be processed, false on error or connection closure
		 */
		bool readData(const ServerConfig *config = NULL);

		/**
		 * @brief Builds the HTTP response for the client request
		 * @param config Server configuration to use for response building
		 * @param router Router instance to handle request routing
		 * @param sessions Map of active sessions for session management
		 */
		void buildResponse(const ServerConfig &config, Router &router, std::map<std::string, SessionData> &sessions);

		/**
		 * @brief Builds the HTTP response from CGI script execution result
		 * @param result CGI execution result containing headers and body
		 */
		void buildResponseFromCGI(const CGIResult &result);

		/**
		 * @brief Builds an HTTP error response with the specified status code
		 * @param statusCode HTTP status code for the error response
		 */
		void buildErrorResponse(int statusCode);

		/**
		 * @brief Sends the HTTP response to the client socket
		 * @return true if the response was fully sent or partially sent successfully, false on error
		 */
		bool sendResponse();

	private:
		// Private method(s) ---------------------------------------------------
		/**
		 * @brief Handles session creation, validation and cookie management
		 * @param sessions Map of active sessions to update or query
		 */
		void handleSession(std::map<std::string, SessionData> &sessions);
};
