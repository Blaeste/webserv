#!/usr/bin/env python3
import sys
import time
import os

# Get a unique ID from query string
query = os.environ.get('QUERY_STRING', '')
cgi_id = query.split('=')[1] if '=' in query else 'unknown'

# Send headers immediately
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")
sys.stdout.flush()

# Sleep 1.5 seconds to simulate medium processing
time.sleep(1.5)

# Send response
print(f"<html><body>")
print(f"<h1>CGI #{cgi_id} completed!</h1>")
print(f"<p>This CGI took 1.5 seconds to process.</p>")
print(f"</body></html>")