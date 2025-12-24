#!/usr/bin/env python3
import os

print("Content-Type: text/html\r")
print("\r")
print("<html><body>")
print("<h1>Python CGI Test</h1>")
print("<p>Method: {}</p>".format(os.environ.get('REQUEST_METHOD', 'N/A')))
print("<p>Query: {}</p>".format(os.environ.get('QUERY_STRING', 'N/A')))
print("</body></html>")
