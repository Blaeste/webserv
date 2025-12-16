/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Config.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:17 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/16 13:50:01 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Parse le fichier de config
// Contient vector<ServerConfig>
// Valide la configuration

#pragma once

#include <string>
#include <vector>
// #include "ServerConfig.hpp"

class Config
{
private:
	// TODO: std::vector<ServerConfig> _servers;
	// TODO: std::string _filePath;

public:
	Config();
	~Config();

	// TODO: bool parse(const std::string &filePath);
	// TODO: const std::vector<ServerConfig> &getServers() const;

private:
	// TODO: void parseServerBlock(/* ... */);
	// TODO: void parseLocationBlock(/* ... */);
	// TODO: bool validate() const;
};
