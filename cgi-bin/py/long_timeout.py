#!/usr/bin/env python3
import sys
import time

# Send headers first
sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")
sys.stdout.flush()

# Sleep for 10 seconds (should trigger timeout at 5s)
time.sleep(10)

# This should never be sent
print("<html><body><h1>This should NOT appear (timeout)</h1></body></html>")
