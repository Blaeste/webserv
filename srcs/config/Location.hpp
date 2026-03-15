/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Location.hpp                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:20:55 by eschwart          #+#    #+#             */
/*   Updated: 2026/03/09 15:37:01 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef LOCATION_HPP
# define LOCATION_HPP

// Include(s) ------------------------------------------------------------------

# include "../webserv.hpp"
# include <string>			// std::string
# include <vector>			// std::vector

// Class -----------------------------------------------------------------------

class Location
{
	private:

		// Attribute(s)

		std::string			_path;				// Location path prefix
		std::string			_root;				// Root directory for file serving
		stringVector		_allowedMethods;	// Allowed HTTP methods
		stringVector		_index;				// Default index file names
		bool				_autoIndex;			// Enable directory listing
		std::string			_uploadPath;		// Directory for file uploads
		std::string			_redirect;			// URL redirection target
		std::string			_cgiExtension;		// CGI file extension
		std::string			_cgiPath;			// CGI interpreter path
		size_t				_maxBodySize;		// Max body size (0 = use server default)

	public:

		// Default constructor

		Location();

		// Getter(s)

		const std::string&	getPath() const								{ return _path; }
		const std::string&	getRoot() const								{ return _root; }
		const stringVector&	getAllowedMethods() const					{ return _allowedMethods; }
		const stringVector&	getIndex() const							{ return _index; }
		bool				getAutoIndex() const						{ return _autoIndex; }
		const std::string&	getUploadPath() const						{ return _uploadPath; }
		const std::string&	getRedirect() const							{ return _redirect; }
		const std::string&	getCgiExtension() const						{ return _cgiExtension; }
		const std::string&	getCgiPath() const							{ return _cgiPath; }
		size_t				getMaxBodySize() const						{ return _maxBodySize; }

		// Setter(s)

		void				setPath(const std::string& path)			{ _path = path; }
		void				setRoot(const std::string& root)			{ _root = root; }
		void				setAutoIndex(bool autoIndex)				{ _autoIndex = autoIndex; }
		void				setUploadPath(const std::string& path)		{ _uploadPath = path; }
		void				setRedirect(const std::string& redirect)	{ _redirect = redirect; }
		void				setCgiExtension(const std::string& ext)		{ _cgiExtension = ext; }
		void				setCgiPath(const std::string& path)			{ _cgiPath = path; }
		void				setMaxBodySize(size_t size)					{ _maxBodySize = size; }

		// Public method(s)

		/** @brief Adds a method to the allowed list (uppercased, no duplicates). */
		void				addAllowedMethod(const std::string& method);

		/** @brief Adds an index filename (no duplicates). */
		void				addIndex(const std::string& index);

		/** @brief Returns true if method is in the allowed list. */
		bool				isMethodAllowed(const std::string& method) const;

		/** @brief Clear default allowed method */
		void				clearAllowedMethods() { _allowedMethods.clear(); }
};

// Typedef(s) - class-dependent ------------------------------------------------

typedef	std::vector<Location>	locationVector;

#endif
