/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:46 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/16 10:25:31 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "Location.hpp"
#include "../utils/utils.hpp"
#include <algorithm>

// Default constructor
Location::Location() :
	_path(""),
	_root(""),
	_autoIndex(false),
	_uploadPath(""),
	_redirect(""),
	_cgiExtension(""),
	_cgiPath("")
{
	_allowedMethods.reserve(3); // GET POST DELETE
	_index.reserve(4); // generaly no more
}

// Setter(s)
void Location::setPath(const std::string &path)
{
	_path = path;
}

void Location::setRoot(const std::string &root)
{
	_root = root;
}

void Location::addAllowedMethod(const std::string &method)
{
	std::string m = toUpperString(method);

	// Checking
	if (m != "GET" && m != "POST" && m != "DELETE")
		return;

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

void Location::setAutoIndex(bool autoIndex)
{
	_autoIndex = autoIndex;
}

void Location::setUploadPath(const std::string &path)
{
	_uploadPath = path;
}

void Location::setRedirect(const std::string &redirect)
{
	_redirect = redirect;
}

void Location::setCgiExtension(const std::string &ext)
{
	_cgiExtension = ext;
}

void Location::setCgiPath(const std::string &path)
{
	_cgiPath = path;
}

// Getter(s)
const std::string &Location::getPath() const
{
	return _path;
}

const std::string &Location::getRoot() const
{
	return _root;
}

const std::vector<std::string> &Location::getAllowedMethods() const
{
	return _allowedMethods;
}

const std::vector<std::string> &Location::getIndex() const
{
	return _index;
}

bool Location::getAutoIndex() const
{
	return _autoIndex;
}

const std::string &Location::getUploadPath() const
{
	return _uploadPath;
}

const std::string &Location::getRedirect() const
{
	return _redirect;
}

const std::string &Location::getCgiExtension() const
{
	return _cgiExtension;
}

const std::string &Location::getCgiPath() const
{
	return _cgiPath;
}

// Public Method(s)
bool Location::isMethodAllowed(const std::string &method) const {
	return std::find(_allowedMethods.begin(), _allowedMethods.end(), method)
		!= _allowedMethods.end();
}
