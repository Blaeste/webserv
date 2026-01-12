/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:44 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/12 17:27:07 by gdosch           ###   ########.fr       */
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

// Enum(s)
enum ClientState {
	STATE_IDLE,       // Reading request or writing response
	STATE_PROCESSING  // Processing request (buildResponse, CGI execution)
};

// Class
class Client {

	private:

		// Attribute(s)

			int _socket;
			std::string _clientIp;
			HttpRequest _request;
			HttpResponse _response;
			time_t _lastActivity;
			bool _requestComplete;
			bool _responseReady;
			std::string _sessionId;
			ClientState _state;

	public:

		// Default constructor

			Client(int socket, const std::string &clientIp);

		// Accessor(s)

			int getSocket() const;
			const std::string &getClientIp() const;
			bool hasTimedOut(time_t readTimeout, time_t processingTimeout) const;  // ← Modified signature
			void updateActivity();
			const HttpRequest& getRequest() const;
			bool isRequestComplete() const;
			bool isResponseReady() const;
			void setState(ClientState state);

		// Public method(s)

			bool readData();
			void buildResponse(const ServerConfig& config, Router& router, std::map<std::string, SessionData>& sessions);
			void buildErrorResponse(int statusCode);
			bool sendResponse();

	private:

		// Private method(s)

			void handleSession(std::map<std::string, SessionData>& sessions);

};
