#!/usr/bin/env python3
import sys
import time

sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")
sys.stdout.flush()

# Sleep 6 sec
time.sleep(6)

print("<html><body><h1>This should timeout</h1></body></html>")
