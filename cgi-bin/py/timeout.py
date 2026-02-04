#!/usr/bin/env python3
import sys
import time

sys.stdout.write("Content-Type: text/html\r\n")
sys.stdout.write("\r\n")
sys.stdout.flush()

# Sleep 6 sec
time.sleep(100)

print("<html><body><h1>This should timeout if timeout is set over 100S</h1></body></html>")
