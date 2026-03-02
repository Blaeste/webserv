/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MimeTypes.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:47 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/02 15:47:48 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

// Include(s) ******************************************************************
#include <map>		// std::map
#include <string>	// std::string

// Class ***********************************************************************
class MimeTypes {
	private:
		// Attribute(s) --------------------------------------------------------
		static const std::map<std::string, std::string> _types; ///< MIME type mappings by file extension

	public:
		// Public method(s) ----------------------------------------------------
		static const std::string& get(const std::string& extension);

	private:
		// Private method(s) ---------------------------------------------------
		static std::map<std::string, std::string> createMap();
};
