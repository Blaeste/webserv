#!/usr/bin/env python3
import os
print("Content-Type: text/plain\r")
print("\r")
print(f"QUERY_STRING: {os.environ.get('QUERY_STRING', 'None')}")
print(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'None')}")
