/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/02/11 13:24:54 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------
#include "Location.hpp"
#include "../utils/utils.hpp"
#include <algorithm>

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
	_allowedMethods.reserve(3); // GET POST DELETE
	_index.reserve(4); // generally no more
}

// Setter(s) -------------------------------------------------------------------
void Location::addAllowedMethod(const std::string &method)
{
	std::string m = toUpperString(method);

	// Anti double
	if (isMethodAllowed(m))
		return;

	_allowedMethods.push_back(m);
}

void Location::addIndex(const std::string &index)
{
	if (std::find(_index.begin(), _index.end(), index) != _index.end())
		return;

	_index.push_back(index);
}

// Public Method(s) ------------------------------------------------------------
bool Location::isMethodAllowed(const std::string &method) const {
	return std::find(_allowedMethods.begin(), _allowedMethods.end(), method)
		!= _allowedMethods.end();
}
