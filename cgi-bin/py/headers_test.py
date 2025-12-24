# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    headers_test.py                                    :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/24 19:56:49 by gdosch            #+#    #+#              #
#    Updated: 2025/12/24 21:08:41 by gdosch           ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

#!/usr/bin/env python3
import os

print("Content-Type: text/html\r")
print("\r")
print("<html><body><h1>All CGI Environment</h1><ul>")

for key, value in sorted(os.environ.items()):
    print("<li><b>{}:</b> {}</li>".format(key, value))

print("</ul></body></html>")
