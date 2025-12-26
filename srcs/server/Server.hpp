/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:51 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/26 15:24:46 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include "Client.hpp"
#include "Router.hpp"
#include "../config/Config.hpp"
#include "../http/HttpResponse.hpp"
#include <poll.h>
#include <vector>

class Server {

	private:

		std::vector<ServerConfig> _configs;
		std::vector<Client> _clients;
		std::vector<pollfd> _pollFds;
		std::vector<int> _listenSockets;
		bool _running;
		Router _router;

	public:

		Server();
		~Server();

		// Public method(s)
		void init(const Config &config);
		void run();
		void stop();

	private:

		// private method(s)
		void checkTimeouts();
		void setupListenSockets();
		bool isListenSocket(int fd) const;
		void acceptNewClient(int listenSocket);

		// Client handling
		const ServerConfig* selectConfig(const HttpRequest& request) const;
		void handleClientRead(size_t clientIndex);
		// TODO: void handleClientWrite(int clientIndex);
		// TODO: void closeClient(int clientIndex);
		// TODO: void checkTimeouts();

};
