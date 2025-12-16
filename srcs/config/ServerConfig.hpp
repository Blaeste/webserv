/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:38 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/16 13:02:16 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Port decoute
// Server name
// Error pages
// Max body size
// vector<Location> locations

#pragma once

#include <string>
#include <vector>
#include <map>
// #include "Location.hpp"

class ServerConfig
{
private:
	// TODO: int _port;
	// TODO: std::string _serverName;
	// TODO: std::map<int, std::string> _errorPages;
	// TODO: size_t _maxBodySize;
	// TODO: std::vector<Location> _locations;

public:
	ServerConfig(/* args */);
	~ServerConfig();

	// TODO: Getters/Setters for all attributes
	// TODO: const Location *matchLocation(const std::string &uri) const;
};

