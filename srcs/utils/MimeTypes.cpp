/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MimeTypes.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:44 by eschwart          #+#    #+#             */
/*   Updated: 2026/01/06 12:46:35 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

// Include(s)
#include "MimeTypes.hpp"

// Private method(s)
std::map<std::string, std::string> MimeTypes::createMap() {
	std::map<std::string, std::string> map;

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

const std::map<std::string, std::string> MimeTypes::_types = MimeTypes::createMap();

// Public method(s)
const std::string& MimeTypes::get(const std::string& extension) {
	std::map<std::string, std::string>::const_iterator it = _types.find(extension);
	if (it != _types.end())
		return it->second;
	static const std::string defaultType = "application/octet-stream";
	return defaultType;
}
