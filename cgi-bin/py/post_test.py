#!/usr/bin/env python3
import os
import sys

print("Content-Type: text/html\r")
print("\r")
print("<html><body>")
print("<h1>POST Test</h1>")
print("<p>Method: {}</p>".format(os.environ.get('REQUEST_METHOD', 'N/A')))
print("<p>Content-Type: {}</p>".format(os.environ.get('CONTENT_TYPE', 'N/A')))
print("<p>Content-Length: {}</p>".format(os.environ.get('CONTENT_LENGTH', 'N/A')))

# Read POST body from stdin
content_length = int(os.environ.get('CONTENT_LENGTH', 0))
if content_length > 0:
    body = sys.stdin.read(content_length)
    print("<p>Body: {}</p>".format(body))

print("</body></html>")
