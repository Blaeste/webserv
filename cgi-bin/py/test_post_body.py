#!/usr/bin/env python3
import sys
import os
print("Content-Type: text/plain\r")
print("\r")
print(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD')}")
print(f"CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH')}")
body = sys.stdin.read()
print(f"Body: {body}")
