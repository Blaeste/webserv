/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:19:51 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/23 19:45:26 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Gere la boucle poll()
// Accepte les nouvelles connexions
// Dispatche les evenement aux clients
// gere plusieur ports decoute

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
		void setupListenSockets();
		bool isListenSocket(int fd) const;
		void acceptNewClient(int listenSocket);

		// Client read
		const ServerConfig* selectConfig(const HttpRequest& request) const;
		void buildResponse(HttpResponse& response, const RouteMatch& match);
		void buildErrorResponse(HttpResponse& response, int statusCode);
		void handleClientRead(size_t clientIndex);
		// TODO: void handleClientWrite(int clientIndex);
		// TODO: void closeClient(int clientIndex);
		// TODO: void checkTimeouts();

};
