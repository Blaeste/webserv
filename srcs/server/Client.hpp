/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:44 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/05 13:11:34 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s)
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include <ctime>
#include <string>

// Forward declaration(s)
class ServerConfig;
class Router;
struct SessionData;
struct CGIProcess;
struct CGIResult;

// Enum(s)
enum ClientState
{
	STATE_KEEPALIVE, // Reading request or writing response
	STATE_PROCESSING // Processing request (buildResponse, CGI execution)
};

// Class
class Client
{

private:
	// Attribute(s)

	int _socket;
	std::string _clientIp;
	HttpRequest _request;
	HttpResponse _response;
	time_t _lastActivity;
	bool _requestComplete;
	bool _responseReady;
	bool _closeAfterResponse;
	std::string _sessionId;
	ClientState _state;
	CGIProcess *_cgiProcess;		   // NULL if no CGI active
	std::string _cachedResponse;	   // Cached response for progressive sending
	size_t _bytesSent;				   // Bytes already sent from cached response
	timeval _cgiStartTime;			   // Start time for CGI requests
	const ServerConfig *_serverConfig; // Server config for logging

public:
	// Default constructor

	Client(int socket, const std::string &clientIp);

	// Accessor(s)

	int getSocket() const;
	const std::string &getClientIp() const;
	bool hasTimedOut(time_t readTimeout, time_t processingTimeout) const; // ← Modified signature
	void updateActivity();
	const HttpRequest &getRequest() const;
	int getResponseStatus();
	size_t getResponseBodySize() const;
	bool isRequestComplete() const;
	bool isResponseReady() const;
	bool shouldCloseAfterResponse() const;
	void markCloseAfterResponse();
	void setState(ClientState state);
	CGIProcess *getCGIProcess() const;
	void setCGIProcess(CGIProcess *cgi);
	void setCGITiming(const ServerConfig &config);

	// Public method(s)

	bool readData();
	void buildResponse(const ServerConfig &config, Router &router, std::map<std::string, SessionData> &sessions);
	void buildResponseFromCGI(const CGIResult &result);
	void buildErrorResponse(int statusCode);
	bool sendResponse();

private:
	// Private method(s)

	void handleSession(std::map<std::string, SessionData> &sessions);
};
