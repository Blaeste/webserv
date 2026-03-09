/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ServerBlock.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/09 15:36:25 by gdosch            #+#    #+#             */
/*   Updated: 2026/03/09 15:36:26 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------

#include "ServerBlock.hpp"

// Default constructor ---------------------------------------------------------

ServerBlock::ServerBlock()
	: _port(DEFAULT_PORT)
	, _serverName("localhost")
	, _maxBodySize(DEFAULT_MAX_BODY_SIZE)
	, _cgiTimeout(DEFAULT_CGI_TIMEOUT)
{}

// Getter(s) -------------------------------------------------------------------

std::string ServerBlock::getErrorPage(int code) const
{
	const std::map<int, std::string>::const_iterator it = _errorPages.find(code);
	if (it != _errorPages.end())
		return it->second;
	return "";
}
