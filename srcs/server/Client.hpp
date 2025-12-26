/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:44 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/26 14:14:07 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Un Client = une connexion socket
// Buffer de lecture (requete)
// Buffer decriture (reponse)
// Etat de la connexion
// Timestamp pour timeout

#pragma once

#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include <ctime>
#include <string>

// Forward declarations
class ServerConfig;
class Router;

class Client {

	private:

		int _socket;
		std::string _readBuffer;
		HttpRequest _request;
		HttpResponse _response;
		time_t _lastActivity;
		bool _requestComplete;
		bool _responseReady;
		// std::string _writeBuffer;

	public:

		Client(int socket);
		~Client();

		int getSocket() const;
		bool hasTimedOut(time_t timeout) const;
		void updateActivity();
		const HttpRequest& getRequest() const;
		bool isRequestComplete() const;

		bool readData();
		void buildResponse(const ServerConfig& config, Router& router);
		void buildErrorResponse(int statusCode);
		bool sendResponse();
		// TODO: void writeData();
		// TODO: void processRequest();
		// TODO: void prepareResponse();

};
