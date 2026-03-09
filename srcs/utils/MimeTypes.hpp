/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MimeTypes.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:47 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/09 13:43:57 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MIMETYPES_HPP
# define MIMETYPES_HPP

// Include(s) ------------------------------------------------------------------

# include <map>		// std::map
# include <string>	// std::string

// Typedef(s) ------------------------------------------------------------------

typedef	std::map<std::string, std::string>	mimeTypeMap;

// Class -----------------------------------------------------------------------

class MimeTypes
{
	private:

		// Attribute(s)

		static const mimeTypeMap	_types; // MIME type mappings by file extension

		// Private method(s)

		/** @brief Builds and returns the static extension→MIME type map. */
		static mimeTypeMap			createMap();

	public:

		// Public method(s)

		/** @brief Returns the MIME type for extension, or "application/octet-stream" if unknown. */
		static const std::string&	get(const std::string& extension);

};

#endif
