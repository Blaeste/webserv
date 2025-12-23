/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:51 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/23 11:27:45 by gdosch           ###   ########.fr       */
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
#include "../config/Config.hpp"

class Server {

	private:

		std::vector<ServerConfig> _configs;
		// TODO: vector<Client> _clients;
		std::vector<pollfd> _pollFds;
		std::vector<int> _listenSockets;
		bool _running;

	public:

		Server();
		~Server();

		void init(const Config &config);
		void run();
		void stop();

	private:

		void setupListenSockets();
		void acceptNewClient(int listenSocket);
		void handleClientRead(size_t clientIndex);
		// TODO: void handleClientWrite(int clientIndex);
		// TODO: void closeClient(int clientIndex);
		// TODO: void checkTimeouts();
		bool isListenSocket(int fd) const;

};
