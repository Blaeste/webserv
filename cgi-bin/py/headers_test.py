#!/usr/bin/env python3
import os

print("Content-Type: text/html\r")
print("\r")
print("<html><body><h1>All CGI Environment</h1><ul>")

for key, value in sorted(os.environ.items()):
    print("<li><b>{}:</b> {}</li>".format(key, value))

print("</ul></body></html>")
