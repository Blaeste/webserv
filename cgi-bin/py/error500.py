#!/usr/bin/env python3

# Intentional server error for testing
import sys

# Don't send valid headers, just crash
raise RuntimeError("Intentional 500 error test")
