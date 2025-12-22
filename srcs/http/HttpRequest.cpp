/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpRequest.cpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/16 10:21:18 by eschwart          #+#    #+#             */
/*   Updated: 2025/12/22 17:42:06 by gdosch           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "HttpRequest.hpp"
#include "../utils/utils.hpp"
#include <vector>

HttpRequest::HttpRequest() : _isComplete(false) {}

HttpRequest::~HttpRequest() {}

bool HttpRequest::appendData(const std::string &data) {
    _rawData += data;
    if (_rawData.find("\r\n\r\n") != std::string::npos)
        return parse();
    return false;
}

bool HttpRequest::isComplete() const {
    return _isComplete;
}

bool HttpRequest::parse() {
    std::vector<std::string> lines = split(_rawData, '\n');
    if (lines.empty())
        return false;
    std::string firstLine = trim(lines[0]);
    std::vector<std::string> parts = split(firstLine, ' ');
    if (parts.size() != 3)
        return false;
    _method = parts[0];
    _uri = parts[1];
    _version = trim(parts[2]);
    for (size_t i = 1; i < lines.size(); i++)
    {
        std::string line = trim(lines[i]);
        if (line.empty())
            break;
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos)
        {
            std::string key = trim(line.substr(0, colonPos));
            std::string value = trim(line.substr(colonPos + 1));
            _headers[key] = value;
        }
    }
    _isComplete = true;
    return true;
}
