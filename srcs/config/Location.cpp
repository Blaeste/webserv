/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/09 13:56:54 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------

#include "Location.hpp"
#include <algorithm>	// std::find, std::transform
#include <cctype>		// ::toupper

// Default constructor ---------------------------------------------------------

Location::Location() :
	_path(""),
	_root(""),
	_autoIndex(false),
	_uploadPath(""),
	_redirect(""),
	_cgiExtension(""),
	_cgiPath(""),
	_maxBodySize(0)
{
	_allowedMethods.push_back("GET");
	_allowedMethods.push_back("POST");
	_allowedMethods.push_back("DELETE");
	_allowedMethods.push_back("HEAD");
	_allowedMethods.push_back("OPTIONS");
	_index.reserve(4); // generally no more
}

// Public method(s) ------------------------------------------------------------

void Location::addAllowedMethod(const std::string& method)
{
	std::string m = method;
	std::transform(m.begin(), m.end(), m.begin(), ::toupper);

	// Anti double
	if (isMethodAllowed(m))
		return;

	_allowedMethods.push_back(m);
}

void Location::addIndex(const std::string& index)
{
	if (std::find(_index.begin(), _index.end(), index) != _index.end())
		return;

	_index.push_back(index);
}

bool Location::isMethodAllowed(const std::string& method) const
{
	return std::find(_allowedMethods.begin(), _allowedMethods.end(), method)
		!= _allowedMethods.end();
}
