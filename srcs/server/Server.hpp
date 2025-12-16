/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:51 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/16 12:53:28 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Gere la boucle poll()
// Accepte les nouvelles connexions
// Dispatche les evenement aux clients
// gere plusieur ports decoute

#pragma once

#include <vector>
#include <poll.h>
// #include "Client.hpp"
// #include "../config/Config.hpp"

class Server
{
private:
	// TODO: vector<ServerConfig> _configs;
	// TODO: vector<Client> _clients;
	// TODO: vector<pollfd> _pollFds;
	// TODO: vector<int> _listenSockets;
	// TODO: bool _running;

public:
	Server();
	~Server();

	// TODO: void init(const Config &config);
	// TODO: void run();
	// TODO: void stop();

private:
	// TODO: void setupListenSockets();
	// TODO: void acceptNewClient(int listenSocket);
	// TODO: void handleClientRead(int clientIndex);
	// TODO: void handleClientWrite(int clientIndex);
	// TODO: void closeClient(int clientIndex);
	// TODO: void checkTimeouts();
};
