#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import html
import re
from urllib.parse import parse_qs

def validate_email(email):
	"""Validate email format"""
	pattern = r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$'
	return re.match(pattern, email) is not None

def sanitize_input(text):
	"""Sanitize user input to prevent XSS"""
	return html.escape(text.strip())

def main():
	# Read POST data
	content_length = int(os.environ.get('CONTENT_LENGTH', 0))

	if content_length == 0:
		print("Status: 400 Bad Request")
		print("Content-Type: text/html")
		print()
		print("<html><body><h1>Error: No data received</h1>")
		print("<div class='home-links'><a href='/contact.html' class='home-button'><span>Try again</span></a></div>")
		print("</body></html>")
		return

	# Read and parse POST data
	post_data = sys.stdin.read(content_length)
	form_data = parse_qs(post_data)

	# Extract and sanitize fields
	name = sanitize_input(form_data.get('name', [''])[0])
	email = sanitize_input(form_data.get('email', [''])[0])
	message = sanitize_input(form_data.get('message', [''])[0])

	# Validation
	errors = []

	if not name or len(name) < 2:
		errors.append("Name must be at least 2 characters")

	if not email or not validate_email(email):
		errors.append("Invalid email adress")

	if not message or len(message) < 10:
		errors.append("Message must be at least 10 characters")

	# Send response
	sys.stdout.write("Content-Type: text/html; charset=utf-8\r\n")
	sys.stdout.write("\r\n")

	if errors:
		print("<!DOCTYPE html>")
		print("<html><head>")
		print("<title>Error</title>")
		print("<link rel='stylesheet' href='/css/style.css'>")
		print("<script src='/js/header.js' defer></script>")
		print("</head><body>")
		print("<div class='container'>")
		print("<div class='whiteBox'>")
		print("<h1>❌ Validation Error</h1>")
		print("<ul style='color:red; text-align:center; margin-bottom:8px;'>")
		for error in errors:
			print(f"<li>{error}</li>")
		print("</ul>")
		print("<div class='home-links'>")
		print("<a href='/contact.html' class='home-button'><span>← Go back</span></a>")
		print("</div>")
		print("</div>")
		print("</div></body></html>")
	else:
		# Success
		print("<!DOCTYPE html>")
		print("<html><head>")
		print("<title>Success</title>")
		print("<link rel='stylesheet' href='/css/style.css'>")
		print("<script src='/js/header.js' defer></script>")
		print("</head><body>")
		print("<div class='container'>")
		print("<div class='whiteBox'>")
		print("<h1>✓ Message Sent!</h1>")
		print("<div style='background:#e6ffe6; padding:20px; border-radius:5px; margin:20px 0;'>")
		print(f"<p><strong>From:</strong> {name} ({email})</p>")
		print(f"<p><strong>Message:</strong></p>")
		print(f"<p style='background:white; padding:15px; border-left:3px solid #0066cc;'>{message}</p>")
		print("</div>")
		
		print("<p style='font-style: italic; text-align: center; margin: 10px;'>Note: This is a demo, message not actually sent.</p>")
		print("<div class='home-links'>")
		print("<a href='/' class='home-button'><span>← Back to home</span></a>")
		print("</div>")
		print("</div>")
		print("</div></body></html>")

if __name__ == "__main__":
	main()
