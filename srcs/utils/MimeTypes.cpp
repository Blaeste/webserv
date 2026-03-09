/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MimeTypes.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:44 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/09 13:26:48 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s) ------------------------------------------------------------------

#include "MimeTypes.hpp"

// Private method(s) -----------------------------------------------------------

mimeTypeMap MimeTypes::createMap() {
	mimeTypeMap map;

	// HTML/CSS/JS
	map[".html"] = "text/html";
	map[".htm"] = "text/html";
	map[".css"] = "text/css";
	map[".js"] = "application/javascript";

	// Images
	map[".jpg"] = "image/jpeg";
	map[".jpeg"] = "image/jpeg";
	map[".png"] = "image/png";
	map[".gif"] = "image/gif";
	map[".svg"] = "image/svg+xml";
	map[".ico"] = "image/x-icon";

	// Text
	map[".txt"] = "text/plain";
	map[".json"] = "application/json";
	map[".xml"] = "application/xml";

	// Other
	map[".pdf"] = "application/pdf";
	map[".zip"] = "application/zip";

	return map;
}

// Static variable initialization ----------------------------------------------

const mimeTypeMap MimeTypes::_types = MimeTypes::createMap();

// Public method(s) ------------------------------------------------------------

const std::string& MimeTypes::get(const std::string& extension) {
	mimeTypeMap::const_iterator it = _types.find(extension);
	if (it != _types.end())
		return it->second;
	static const std::string defaultType = "application/octet-stream";
	return defaultType;
}
