# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    test.py                                            :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/24 19:56:53 by gdosch            #+#    #+#              #
#    Updated: 2025/12/24 19:56:54 by gdosch           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

#!/usr/bin/env python3
import os

print("Content-Type: text/html\r")
print("\r")
print("<html><body>")
print("<h1>Python CGI Test</h1>")
print("<p>Method: {}</p>".format(os.environ.get('REQUEST_METHOD', 'N/A')))
print("<p>Query: {}</p>".format(os.environ.get('QUERY_STRING', 'N/A')))
print("</body></html>")
