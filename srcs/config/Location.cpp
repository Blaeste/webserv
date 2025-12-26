/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.cpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:46 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/26 14:24:40 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// =============================================================================
// Includes

#include "Location.hpp"

// =============================================================================
// Handlers

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
}
// Destructor
Location::~Location()
{
}

// =============================================================================
// Setters

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
	_allowedMethods.push_back(method);
}

void Location::addIndex(const std::string &index)
{
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

// =============================================================================
// Getters

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

// =============================================================================
// Methods

bool Location::isMethodAllowed(const std::string &method) const {
    for (size_t i = 0; i < _allowedMethods.size(); i++)
        if (_allowedMethods[i] == method)
            return true;
    return false;
}
