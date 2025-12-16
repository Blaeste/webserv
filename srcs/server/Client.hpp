/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:44 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/16 13:37:20 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Un Client = une connexion socket
// Buffer de lecture (requete)
// Buffer decriture (reponse)
// Etat de la connexion
// Timestamp pour timeout

#pragma once

#include <string>
#include <ctime>
// #include "../http/HttpRequest.hpp"
// #include "../http/HttpResponse.hpp"

class Client
{
private:
	// TODO: int _socket;
	// TODO: std::string _readBuffer;
	// TODO: std::string _writeBuffer;
	// TODO: HttpRequest _request;
	// TODO: HttpResponse _response;
	// TODO: time_t _lastActivity;
	// TODO: bool _requestComplete;
	// TODO: bool _responseReady;

public:
	Client(/* args */);
	~Client();

	// TODO: int getSocket() const;
	// TODO: bool hasTimedOut(time_t timeout) const;
	// TODO: void updateActivity();

	// TODO: void readData();
	// TODO: void writeData();
	// TODO: void processRequest();
	// TODO: void prepareResponse();
};
