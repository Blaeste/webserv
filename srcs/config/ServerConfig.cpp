/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerConfig.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:29 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/09 14:44:40 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------

#include "ServerConfig.hpp"

// Default constructor ---------------------------------------------------------

ServerConfig::ServerConfig()
	: _port(DEFAULT_PORT)
	, _serverName("localhost")
	, _maxBodySize(DEFAULT_MAX_BODY_SIZE)
	, _cgiTimeout(DEFAULT_CGI_TIMEOUT)
{}

// Getter(s) -------------------------------------------------------------------

std::string ServerConfig::getErrorPage(int code) const
{
	const std::map<int, std::string>::const_iterator it = _errorPages.find(code);
	if (it != _errorPages.end())
		return it->second;
	return "";
}
