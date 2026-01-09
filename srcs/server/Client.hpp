/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:44 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/09 12:52:02 by eschwart         ###   ########.fr       */
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

// Class
class Client{

	private:

		// Attribute(s)

			int _socket;
			int _localPort;
			std::string _clientIp;
			std::string _readBuffer;
			HttpRequest _request;
			HttpResponse _response;
			time_t _lastActivity;
			bool _requestComplete;
			bool _responseReady;
			std::string _sessionId;

	public:

		// Default constructor

			Client(int socket, const std::string &clientIp);

		// Accessor(s)

			int getLocalPort()const;
			int getSocket() const;
			const std::string &getClientIp() const;
			bool hasTimedOut(time_t timeout) const;
			void updateActivity();
			const HttpRequest& getRequest() const;
			bool isRequestComplete() const;

		// Public method(s)

			bool readData();
			void buildResponse(const ServerConfig& config, Router& router, std::map<std::string, SessionData>& sessions);
			void buildErrorResponse(int statusCode);
			bool sendResponse();
			void setLocalPort(int port);

	private:

		// Private method(s)

			void handleSession(std::map<std::string, SessionData>& sessions);
};
