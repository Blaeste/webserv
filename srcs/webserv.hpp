/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   webserv.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/03/09 14:58:01 by gdosch            #+#    #+#             */
/*   Updated: 2026/03/09 15:10:24 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef WEBSERV_HPP
# define WEBSERV_HPP

// Include(s) ------------------------------------------------------------------

# include <map>		// std::map
# include <string>	// std::string
# include <vector>	// std::vector

// Constant(s) -----------------------------------------------------------------

enum { READ_BUFFER_SIZE = 4096 };   // Shared I/O buffer size for pipes and sockets

// Common typedef(s) -----------------------------------------------------------

typedef	std::vector<std::string>			stringVector;
typedef	std::map<std::string, std::string>	headerMap;
typedef	std::map<std::string, std::string>	cookieMap;

#endif
