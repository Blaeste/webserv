/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   utils.hpp                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:22:52 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/17 12:22:15 by eschwart         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#pragma once

#include <string>
#include <vector>

std::string trim(const std::string &str);
std::vector<std::string> split(const std::string &str, char delimiter);
bool isHexDigit(char c);
std::string urlDecode(const std::string &url);
bool fileExists(const std::string &path);
bool isDirectory(const std::string &path);
std::string getFileExtension(const std::string &path);
std::string getHttpDate();
std::string readFile(const std::string &path);
size_t getFileSize(const std::string &path);
