/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: lmarck <lmarck@42.fr>                      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:44 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/06 12:39:23 by gdosch           ###   ########.fr       */
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

class Client{

	private:

		// Attribute(s)
		int _socket;
		std::string _readBuffer;
		HttpRequest _request;
		HttpResponse _response;
		time_t _lastActivity;
		bool _requestComplete;
		bool _responseReady;
		std::string _sessionId;

		// Private method(s)
		void handleSession(std::map<std::string, SessionData>& sessions);

	public:

		// Default constructor
		Client(int socket);

		// Accessor(s)
		int getSocket() const;
		bool hasTimedOut(time_t timeout) const;
		void updateActivity();
		const HttpRequest& getRequest() const;
		bool isRequestComplete() const;

		// Public method(s)
		bool readData();
		void buildResponse(const ServerConfig& config, Router& router, std::map<std::string, SessionData>& sessions);
		void buildErrorResponse(int statusCode);
		bool sendResponse();

};
