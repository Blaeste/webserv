/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:29 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/11 13:16:28 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------
#include "ServerConfig.hpp"

// Default constructor ---------------------------------------------------------
ServerConfig::ServerConfig()
	: _port(8080)
	, _serverName("localhost")
	, _maxBodySize(1048576)
	, _cgiTimeout(90)
{}

// Getters ---------------------------------------------------------------------
std::string ServerConfig::getErrorPage(int code) const {
	const std::map<int, std::string>::const_iterator it = _errorPages.find(code);
	if (it != _errorPages.end())
		return it->second;
	return "";
}
