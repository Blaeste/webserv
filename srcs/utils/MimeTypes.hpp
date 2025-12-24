/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   MimeTypes.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:47 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/24 12:27:15 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <map>
#include <string>

class MimeTypes {

	private:

		MimeTypes();
		~MimeTypes();

		static const std::map<std::string, std::string> _types;
		static std::map<std::string, std::string> createMap();

	public:

		static const std::string& get(const std::string& extension);

};
