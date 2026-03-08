/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MimeTypes.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:47 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/08 18:31:22 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef MIMETYPES_HPP
# define MIMETYPES_HPP

// Include(s) ------------------------------------------------------------------

# include <map>		// std::map
# include <string>	// std::string

// Class -----------------------------------------------------------------------

class MimeTypes
{
	private:

		// Attribute(s)
		static const std::map<std::string, std::string> _types; // MIME type mappings by file extension

		// Private method(s)
		static std::map<std::string, std::string> createMap();

	public:

		// Public method(s)
		static const std::string& get(const std::string& extension);

};

#endif
