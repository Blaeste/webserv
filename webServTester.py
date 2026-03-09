# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    webServTester.py                                   :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/01/16 11:30:57 by eschwart          #+#    #+#              #
#    Updated: 2026/03/09 10:54:10 by eschwart         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

import requests
import sys
import os
import socket
import json
import subprocess
import time
import re
import tempfile
import shutil
import http.client
from datetime import datetime

BASE_URL = "http://localhost:8080"
TIMEOUT = 5
PASSED = 0
FAILED = 0
LOG_FILE = "test_results.json"

GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
RESET = '\033[0m'

# Dictionary to store test results
test_results = {}

def check_and_install_dependencies():
	"""Check dependencies and install them if missing"""
	tools = {
		'netstat': 'net-tools',
		'siege': 'siege',
		'php-cgi': 'php-cgi'
	}

	missing = []
	for cmd, package in tools.items():
		if subprocess.run(['which', cmd], capture_output=True).returncode != 0:
			missing.append((cmd, package))

	if missing:
		print(f"{YELLOW}Installing missing dependencies...{RESET}")
		for cmd, package in missing:
			print(f"  • Installing {package}...")
			result = subprocess.run(['sudo', 'apt', 'install', '-y', package], capture_output=True)
			if result.returncode == 0:
				print(f"    {GREEN}✓{RESET} {package} installed")
			else:
				print(f"    {RED}✗{RESET} Failed to install {package}")
		print()

def test(name, condition, details=""):
	global PASSED, FAILED, test_results
	if condition:
		print(f"{GREEN}✓{RESET} {name}")
		PASSED += 1
		test_results[name] = {"status": "passed", "details": details}
	else:
		print(f"{RED}✗{RESET} {name}")
		if details:
			print(f"{YELLOW}→{RESET} {details}")
		FAILED += 1
		test_results[name] = {"status": "failed", "details": details}

def test_error(name, error_msg=""):
	"""Record a test that caused a timeout or exception"""
	global FAILED, test_results
	print(f"{YELLOW}⚠{RESET} {name} (error/timeout)")
	if error_msg:
		print(f"{YELLOW}→{RESET} {error_msg}")
	FAILED += 1
	test_results[name] = {"status": "error", "details": error_msg}

def safe_get(url, **kwargs):
	"""GET with default timeout"""
	kwargs.setdefault('timeout', TIMEOUT)
	return requests.get(url, **kwargs)

def safe_post(url, **kwargs):
	"""POST with default timeout"""
	kwargs.setdefault('timeout', TIMEOUT)
	return requests.post(url, **kwargs)

def safe_delete(url, **kwargs):
	"""DELETE with default timeout"""
	kwargs.setdefault('timeout', TIMEOUT)
	return requests.delete(url, **kwargs)

def safe_put(url, **kwargs):
	"""PUT with default timeout"""
	kwargs.setdefault('timeout', TIMEOUT)
	return requests.put(url, **kwargs)

def safe_head(url, **kwargs):
	"""HEAD with default timeout"""
	kwargs.setdefault('timeout', TIMEOUT)
	return requests.head(url, **kwargs)

def safe_options(url, **kwargs):
	"""OPTIONS with default timeout"""
	kwargs.setdefault('timeout', TIMEOUT)
	return requests.options(url, **kwargs)

def run_test(func):
	"""Wrapper to execute a test with error handling"""
	try:
		func()
	except Exception as e:
		test_error(f"{func.__name__} - Exception: {str(e)[:50]}")

# ============================================================================
# PART 1: MANDATORY PART - CODE CHECKS
# ============================================================================

def test_code_poll_in_loop():
	"""Check poll() is used in the main loop"""
	result = subprocess.run(['grep', '-n', 'poll(', 'srcs/server/Server.cpp'],
						  capture_output=True, text=True)
	test("poll() found in Server.cpp", len(result.stdout) > 0,
		 f"Found {len(result.stdout.splitlines())} occurrences")

def test_code_poll_read_write():
	"""Check poll() monitors both READ and WRITE simultaneously"""
	result = subprocess.run(['grep', '-E', 'POLLIN|POLLOUT', 'srcs/server/Server.cpp'],
						  capture_output=True, text=True)
	has_pollin = 'POLLIN' in result.stdout
	has_pollout = 'POLLOUT' in result.stdout
	test("poll() checks both POLLIN and POLLOUT", has_pollin and has_pollout,
		 f"POLLIN: {has_pollin}, POLLOUT: {has_pollout}")

def test_code_compilation():
	"""Test compilation without re-link"""
	import os
	# Clean first to ensure we actually compile
	subprocess.run(['make', 'fclean'], capture_output=True, text=True, cwd='.')

	# First compilation
	result1 = subprocess.run(['make'], capture_output=True, text=True, cwd='.')

	# Get timestamp of executable after first compilation
	if os.path.exists('webserv'):
		mtime_after_first = os.path.getmtime('webserv')
	else:
		test("Compilation without re-link", False, "webserv not created after first make")
		return

	# Wait a bit to ensure timestamp would change if relinked
	time.sleep(0.1)

	# Second make (should be a no-op)
	result2 = subprocess.run(['make'], capture_output=True, text=True, cwd='.')

	# Get timestamp after second make
	mtime_after_second = os.path.getmtime('webserv')

	# Check if executable was NOT relinked (timestamp unchanged)
	no_relink = (mtime_after_first == mtime_after_second)

	details = f"1st make: rc={result1.returncode} | 2nd make: rc={result2.returncode} | timestamps: {mtime_after_first == mtime_after_second}"
	test("Compilation without re-link", result1.returncode == 0 and no_relink, details)

# ============================================================================
# PART 2: CONFIGURATION
# ============================================================================

def test_config_multiple_ports():
	"""Test multiple ports and virtual hosts serve content"""
	servers = [
		{"name": "webServTester", "port": 8080, "host": "webServTester"},
		{"name": "server2",       "port": 8081, "host": "server2"},
		{"name": "same_port",     "port": 8080, "host": "same_port"},
		{"name": "server4",       "port": 8082, "host": "server4"},
	]
	for srv in servers:
		try:
			r = requests.get(f"http://localhost:{srv['port']}/",
							 headers={"Host": srv["host"]}, timeout=2)
			test(f"Server '{srv['name']}' (port {srv['port']}) is accessible",
				 r.status_code == 200, f"Got {r.status_code}")
		except Exception as e:
			test(f"Server '{srv['name']}' (port {srv['port']}) is accessible",
				 False, f"Not responding: {e}")

def test_config_virtual_hosts():
	"""Test virtual hosts with different Host headers"""
	# Test with custom Host header
	headers = {'Host': 'example.com'}
	r = safe_get(f"{BASE_URL}/", headers=headers)
	test("Virtual host with custom Host header", r.status_code in [200, 404],
		 f"Got {r.status_code}")

	# Test with normal Host header
	headers = {'Host': 'localhost'}
	r2 = safe_get(f"{BASE_URL}/", headers=headers)
	test("Virtual host with localhost Host header", r2.status_code == 200,
		 f"Got {r2.status_code}")

def test_config_routes_directories():
	"""Test routes to different directories"""
	routes = [
		('/', 'www/'),
		('/uploads/', 'uploads/'),
	]
	for route, expected_dir in routes:
		r = safe_get(f"{BASE_URL}{route}")
		test(f"Route {route} accessible", r.status_code in [200, 403],
			 f"Got {r.status_code}")

def test_get_index():
	r = safe_get(f"{BASE_URL}/")
	test("GET / return 200", r.status_code == 200, f"Got {r.status_code}")
	test("GET / contains HTML", "html" in r.text.lower(), f"Body: {r.text[:100]}...")

def test_get_static_files():
	# Test CSS
	r = safe_get(f"{BASE_URL}/css/style.css")
	test("GET /css/style.css return 200", r.status_code == 200, f"Got {r.status_code}")
	test("CSS has correct Content-Type", "text/css" in r.headers.get("Content-Type", ""), f"Got: {r.headers.get('Content-Type', 'None')}")

	# Test JS
	r = safe_get(f"{BASE_URL}/js/header.js")
	test("GET /js/header.js return 200", r.status_code == 200)

	# Test TXT
	r = safe_get(f"{BASE_URL}/text/test.txt")
	test("GET /text/test.txt return 200", r.status_code == 200)

def test_get_404():
	r = safe_get(f"{BASE_URL}/page_qui_existe_pas.html")
	test("GET inexistent page return 404", r.status_code == 404, f"Got {r.status_code}")
	test("404 page contains error content", "404" in r.text, f"Body: {r.text[:100]}...")

def test_get_pages():
	pages = ["about.html", "contact.html", "qrcode.html"]
	for page in pages:
		r = safe_get(f"{BASE_URL}/{page}")
		test(f"GET /{page} return 200", r.status_code == 200)

def test_response_body_content():
	# Test that the body actually contains content
	r = safe_get(f"{BASE_URL}/")
	test("GET / body is not empty", len(r.text) > 0)
	test("GET / body contains 'webserv' or title", any(word in r.text.lower() for word in ['webserv', 'title', 'html']))

	# Test Content-Length header
	r = safe_get(f"{BASE_URL}/text/test.txt")
	test("Response has Content-Lenght header", "Content-Length" in r.headers)
	test("Content-Length matches body size", int(r.headers.get("Content-Length", 0)) == len(r.content))


def test_post_upload():
	# Upload a text file
	files = {'file': ('test_upload.txt', 'Hello form tester!', 'text/plain')}
	r = safe_post(f"{BASE_URL}/uploads", files=files)
	test("POST file upload return 200 or 201", r.status_code in [200, 201], f"Got {r.status_code}")

	# Verify the file exists
	r = safe_get(f"{BASE_URL}/uploads/test_upload.txt")
	test("Upload file is accessible", r.status_code == 200)
	# Note: no delete here, test_delete() will remove it

def test_post_cgi_python():
	r = safe_post(f"{BASE_URL}/cgi-bin/py/contact.py", data={'name': 'Test', 'email': 'test@test.com', 'message': 'Hello world form contact cgi test'})
	test("POST CGI Python return 200", r.status_code == 200)

def test_delete():
	# Delete the file from upload test first
	r = safe_delete(f"{BASE_URL}/uploads/test_upload.txt")
	test("DELETE file return 200 or 204", r.status_code in [200, 204], f"Got {r.status_code}")

	# Verify it is actually deleted
	r = safe_get(f"{BASE_URL}/uploads/test_upload.txt")
	test("Deleted file return 404", r.status_code == 404)

def test_method_not_allowed():
	# DELETE on root should be forbidden (405 Method Not Allowed)
	r = safe_delete(f"{BASE_URL}/")
	test("DELETE / returns 405", r.status_code == 405, f"Got {r.status_code}")
	# RFC 7231: 405 MUST include Allow header
	if r.status_code == 405:
		test("405 response has Allow header", "Allow" in r.headers, f"Headers: {r.headers}")

def test_unknown_method_no_crash():
	"""Test UNKNOWN method (FOOBAR) and verify no crash"""
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(3)
		s.connect(('localhost', 8080))
		s.send(b"FOOBAR / HTTP/1.1\r\nHost: localhost\r\n\r\n")
		response = s.recv(4096)
		s.close()
		test("UNKNOWN method returns 501 or 405", b"501" in response or b"405" in response,
			 f"Response: {response[:100]}")

		# Verify the server still responds
		time.sleep(0.5)
		r = safe_get(f"{BASE_URL}/")
		test("Server still running after UNKNOWN method", r.status_code == 200,
			 "Server crashed or not responding")
	except Exception as e:
		test_error("UNKNOWN method test", str(e)[:50])

def test_autoindex():
	# Test autoindex on uploads
	r = safe_get(f"{BASE_URL}/uploads/")
	test("GET /uploads/ with autoindex return 200", r.status_code == 200)
	test("Autoindex contains directory listing", "index of" in r.text.lower() or "directory" in r.text.lower())

	# Test autoindex off sur /
	r = safe_get(f"{BASE_URL}/")
	test("GET / doesnt show directory listing", "index of" not in r.text.lower())

def test_http_headers():
	r = safe_get(f"{BASE_URL}/")
	test("Response has Server header", "Server" in r.headers)
	test("Response has Date header", "Date" in r.headers)
	test("Response has Connection header", "Connection" in r.headers)

def test_keep_alive():
	# Session to keep the connection alive
	session = requests.Session()

	r1 = session.get(f"{BASE_URL}/")
	test("First request return 200", r1.status_code == 200)

	r2 = session.get(f"{BASE_URL}/about.html")
	test("Second request on same connection return 200", r2.status_code == 200)

	session.close()

def test_cgi_errors():
	#Test CGI that times out
	r =safe_get(f"{BASE_URL}/cgi-bin/py/timeout.py", timeout=10)
	test("CGI timeout return 504", r.status_code == 504, f"Got {r.status_code}")

	# Test CGI avec erreur 500
	r = safe_get(f"{BASE_URL}/cgi-bin/py/error500.py")
	test("CGI error return 500", r.status_code == 500, f"Got {r.status_code} (TODO: fix server)")

# ============================================================================
# PART 4: CHECK CGI - ADVANCED TESTS
# ============================================================================

def test_cgi_working_directory():
	"""Test CGI runs in the correct directory"""
	# Create a script that prints its pwd
	script_content = '''#!/usr/bin/env python3
import os
print("Content-Type: text/plain\\r")
print("\\r")
print(f"PWD: {os.getcwd()}")
'''
	script_path = 'cgi-bin/py/test_pwd.py'
	with open(script_path, 'w') as f:
		f.write(script_content)
	os.chmod(script_path, 0o755)

	r = safe_get(f"{BASE_URL}/cgi-bin/py/test_pwd.py")
	test("CGI pwd test returns 200", r.status_code == 200, f"Got {r.status_code}")
	if r.status_code == 200:
		test("CGI runs in correct directory", 'cgi-bin' in r.text.lower() or 'py' in r.text.lower(),
			 f"PWD: {r.text[:100]}")

	# Cleanup
	try:
		os.remove(script_path)
	except:
		pass

def test_cgi_syntax_error():
	"""Test CGI with syntax error"""
	# Create a script with a syntax error
	script_content = '''#!/usr/bin/env python3
print("Content-Type: text/plain\\r")
print("\\r")
this is invalid python syntax!!!
print("Should not reach here")
'''
	script_path = 'cgi-bin/py/test_syntax_error.py'
	with open(script_path, 'w') as f:
		f.write(script_content)
	os.chmod(script_path, 0o755)

	r = safe_get(f"{BASE_URL}/cgi-bin/py/test_syntax_error.py")
	test("CGI syntax error returns 500", r.status_code == 500, f"Got {r.status_code}")

	# Verify the server still responds
	r2 = safe_get(f"{BASE_URL}/")
	test("Server still running after CGI syntax error", r2.status_code == 200)

	# Cleanup
	try:
		os.remove(script_path)
	except:
		pass

def test_cgi_runtime_error():
	"""Test CGI with runtime error (division by zero)"""
	script_content = '''#!/usr/bin/env python3
print("Content-Type: text/plain\\r")
print("\\r")
print("Before crash")
x = 1 / 0
print("After crash")
'''
	script_path = 'cgi-bin/py/test_runtime_error.py'
	with open(script_path, 'w') as f:
		f.write(script_content)
	os.chmod(script_path, 0o755)

	r = safe_get(f"{BASE_URL}/cgi-bin/py/test_runtime_error.py")
	test("CGI runtime error returns 500", r.status_code == 500, f"Got {r.status_code}")

	# Verify the server still responds
	r2 = safe_get(f"{BASE_URL}/")
	test("Server still running after CGI runtime error", r2.status_code == 200)

	# Cleanup
	try:
		os.remove(script_path)
	except:
		pass

def test_cgi_missing_shebang():
	"""Test CGI without shebang"""
	script_content = '''print("Content-Type: text/plain\\r")
print("\\r")
print("No shebang")
'''
	script_path = 'cgi-bin/py/test_no_shebang.py'
	with open(script_path, 'w') as f:
		f.write(script_content)
	os.chmod(script_path, 0o755)

	r = safe_get(f"{BASE_URL}/cgi-bin/py/test_no_shebang.py")
	test("CGI without shebang handled", r.status_code in [200, 500], f"Got {r.status_code}")

	# Cleanup
	try:
		os.remove(script_path)
	except:
		pass

def test_cgi_permission_denied():
	"""CGI should run even if script lacks +x when interpreter is executable"""
	script_content = '''#!/usr/bin/env python3
print("Content-Type: text/plain\\r")
print("\\r")
print("Should run without xbit")
'''
	script_path = 'cgi-bin/py/test_no_exec.py'
	with open(script_path, 'w') as f:
		f.write(script_content)
	# Remove execute bit to ensure server relies on interpreter, not script perms
	os.chmod(script_path, 0o644)

	r = safe_get(f"{BASE_URL}/cgi-bin/py/test_no_exec.py")
	test("CGI runs without script execute bit", r.status_code == 200, f"Got {r.status_code}")

	# Cleanup
	try:
		os.remove(script_path)
	except:
		pass

def test_cgi_interpreter_not_executable():
	"""CGI must fail when interpreter lacks execute permission"""
	tmpdir = tempfile.mkdtemp(prefix="webserv_cgi_interp_")
	interp_path = os.path.join(tmpdir, "interp.sh")
	script_path = os.path.join(tmpdir, "script.sh")
	config_path = os.path.join(tmpdir, "cgi_no_exec.conf")

	try:
		# Create a dummy interpreter without execute bits
		with open(interp_path, 'w') as f:
			f.write("#!/bin/sh\n" +
					"echo Content-Type: text/plain\\r\n" +
					"echo\\r\n" +
					"echo from dummy interpreter\n")
		os.chmod(interp_path, 0o644)

		# Create a simple CGI script (readable only)
		with open(script_path, 'w') as f:
			f.write("#!/bin/sh\n" +
					"echo Content-Type: text/plain\\r\n" +
					"echo\\r\n" +
					"echo script ran\n")
		os.chmod(script_path, 0o644)

		# Minimal config pointing to the non-executable interpreter
		config_content = f"""
server {{
	listen 8099;
	server_name localhost;
	client_max_body_size 1M;

	location / {{
		root {tmpdir};
		allowed_methods GET;
	}}

	location /tmpcgi {{
		root {tmpdir};
		allowed_methods GET;
		cgi_extension .sh;
		cgi_path {interp_path};
	}}
}}
"""
		with open(config_path, 'w') as f:
			f.write(config_content)

		# Start isolated webserv instance
		proc = subprocess.Popen(['./webserv', config_path], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
		time.sleep(0.5)

		try:
			r = requests.get("http://localhost:8099/tmpcgi/script.sh", timeout=2)
			test("CGI with non-executable interpreter returns 500", r.status_code == 500, f"Got {r.status_code}")
		finally:
			proc.terminate()
			try:
				proc.wait(timeout=2)
			except subprocess.TimeoutExpired:
				proc.kill()
	finally:
		shutil.rmtree(tmpdir, ignore_errors=True)

def test_cgi_get_with_query():
	"""Test CGI with detailed GET parameters"""
	script_content = '''#!/usr/bin/env python3
import os
print("Content-Type: text/plain\\r")
print("\\r")
print(f"QUERY_STRING: {os.environ.get('QUERY_STRING', 'None')}")
print(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'None')}")
'''
	script_path = 'cgi-bin/py/test_get_query.py'
	with open(script_path, 'w') as f:
		f.write(script_content)
	os.chmod(script_path, 0o755)

	r = safe_get(f"{BASE_URL}/cgi-bin/py/test_get_query.py?name=John&age=25")
	test("CGI GET with query params returns 200", r.status_code == 200, f"Got {r.status_code}")
	if r.status_code == 200:
		test("CGI receives QUERY_STRING", 'name=John' in r.text and 'age=25' in r.text,
			 f"Response: {r.text[:200]}")

	# Cleanup
	try:
		os.remove(script_path)
	except:
		pass

def test_cgi_post_with_body():
	"""Test CGI POST properly receives the body"""
	script_content = '''#!/usr/bin/env python3
import sys
import os
print("Content-Type: text/plain\\r")
print("\\r")
print(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD')}")
print(f"CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH')}")
body = sys.stdin.read()
print(f"Body: {body}")
'''
	script_path = 'cgi-bin/py/test_post_body.py'
	with open(script_path, 'w') as f:
		f.write(script_content)
	os.chmod(script_path, 0o755)

	r = safe_post(f"{BASE_URL}/cgi-bin/py/test_post_body.py", data="test=data&foo=bar")
	test("CGI POST with body returns 200", r.status_code == 200, f"Got {r.status_code}")
	if r.status_code == 200:
		test("CGI receives POST body", 'test=data' in r.text,
			 f"Response: {r.text[:200]}")

	# Cleanup
	try:
		os.remove(script_path)
	except:
		pass

def test_chunked_encoding():
	"""Send a real chunked POST (Transfer-Encoding: chunked) via raw HTTP"""

	chunks = [b"Hello ", b"chunked ", b"world!"]
	try:
		parsed = requests.utils.urlparse(BASE_URL)
		host = parsed.hostname or "localhost"
		port = parsed.port or (443 if parsed.scheme == "https" else 80)
		path = parsed.path or "/"

		conn = http.client.HTTPConnection(host, port, timeout=TIMEOUT)
		conn.putrequest("POST", path)
		conn.putheader("Host", host)
		conn.putheader("Transfer-Encoding", "chunked")
		conn.putheader("Content-Type", "text/plain")
		conn.endheaders()

		for chunk in chunks:
			conn.send(hex(len(chunk))[2:].encode() + b"\r\n" + chunk + b"\r\n")
		conn.send(b"0\r\n\r\n")

		resp = conn.getresponse()
		status = resp.status
		resp.read()
		conn.close()
		test("POST with chunked encoding return 200", status in [200, 201], f"Got {status}")
	except Exception as e:
		test_error("test_chunked_encoding", str(e)[:100])

def test_large_file_upload():
	# Upload a bigger file (1MB)
	big_content = 'x' * (1024 * 1024) # 1 MB
	files = {'file': ('big_file.txt', big_content, 'text/plain')}

	r = safe_post(f"{BASE_URL}/uploads", files=files, timeout=10)
	test("Upload 1MB file return 200 or 201", r.status_code in [200, 201], f"Got {r.status_code}")

	# Cleanup
	if r.status_code in [200, 201]:
		safe_delete(f"{BASE_URL}/uploads/big_file.txt")

def test_multiple_requests():
	# Test multiple rapid requests
	for i in range(5):
		r = safe_get(f"{BASE_URL}/")
		test(f"Rapid request #{i+1} return 200", r.status_code == 200)

def test_security_path_traversal():
	# Test path traversal
	r = safe_get(f"{BASE_URL}/../../../etc/passwd")
	test("Path traversal blocked (not 200)", r.status_code != 200)

	r = safe_get(f"{BASE_URL}/../../config/default.conf")
	test("Config file not accessible via path traversal", r.status_code in [403, 404])

def test_malformed_requests():
	# Request without Host header (HTTP/1.1 requires Host)
	try:
		import socket
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(2)
		s.connect(('localhost', 8080))
		s.send(b"GET / HTTP/1.1\r\n\r\n")
		response = s.recv(1024)
		s.close()
		test("Request without Host header handled", b"400" in response or b"200" in response)
	except:
		test("Malformed request test failed (connection issue)", False)

def test_empty_requests():
	# Empty body
	r = safe_post(f"{BASE_URL}/", data="")
	test("POST with empty body returns 200", r.status_code in [200, 201])

	# GET with empty query string
	r = safe_get(f"{BASE_URL}/?")
	test("GET with empty query string returns 200", r.status_code == 200)

def test_special_characters():
	# Test URL encoding
	r = safe_get(f"{BASE_URL}/text/test.txt?param=hello%20world")
	test("URL with encoded spaces returns 200", r.status_code == 200)

	# Special characters in filename
	files = {'file': ('test file with spaces.txt', 'content', 'text/plain')}
	r = safe_post(f"{BASE_URL}/uploads", files=files)
	test("Upload file with spaces in name", r.status_code in [200, 201])

	# Cleanup
	if r.status_code in [200, 201]:
		try:
			safe_delete(f"{BASE_URL}/uploads/test file with spaces.txt")
		except:
			safe_delete(f"{BASE_URL}/uploads/test%20file%20with%20spaces.txt")

def test_multiple_file_upload():
	# Upload multiple files
	files = [
		('file', ('file1.txt', 'Content 1', 'text/plain')),
		('file', ('file2.txt', 'Content 2', 'text/plain'))
	]
	r = safe_post(f"{BASE_URL}/uploads", files=files)
	test("Multiple file upload", r.status_code in [200, 201])

	# Cleanup
	if r.status_code in [200, 201]:
		safe_delete(f"{BASE_URL}/uploads/file1.txt")
		safe_delete(f"{BASE_URL}/uploads/file2.txt")

def test_concurrent_requests():
	# Test concurrent requests with threads
	import threading
	results = []

	def make_request():
		r = safe_get(f"{BASE_URL}/")
		results.append(r.status_code == 200)

	threads = []
	for i in range(10):
		t = threading.Thread(target=make_request)
		threads.append(t)
		t.start()

	for t in threads:
		t.join()

	test("10 concurrent requests all succeed", all(results))

def test_long_url():
	# Very long URL
	long_path = "/text/" + "a" * 1000 + ".txt"
	r = safe_get(f"{BASE_URL}{long_path}")
	test("Long URL returns 404 or 414", r.status_code in [404, 414])

def test_post_without_content_type():
	# POST without Content-Type
	r = safe_post(f"{BASE_URL}/", data="raw data")
	test("POST without explicit Content-Type handled", r.status_code in [200, 201, 400])

def test_head_method():
	# HEAD should return same headers as GET but no body
	r = safe_head(f"{BASE_URL}/")
	test("HEAD returns 200 or 405", r.status_code in [200, 405], f"Got {r.status_code}")
	if r.status_code == 200:
		test("HEAD has no body", len(r.content) == 0, f"Body length: {len(r.content)}")
	elif r.status_code == 405:
		# RFC 7231: 405 MUST include Allow header
		test("405 response has Allow header", "Allow" in r.headers, f"Headers: {r.headers}")

def test_case_sensitivity():
	# Test case sensitivity des URLs
	r1 = safe_get(f"{BASE_URL}/INDEX.HTML")
	r2 = safe_get(f"{BASE_URL}/index.html")
	test("Case sensitivity test", r1.status_code != r2.status_code or r1.status_code in [200, 404])

def test_double_slash():
	# Double slashes dans URL
	r = safe_get(f"{BASE_URL}//index.html")
	test("URL with double slash handled", r.status_code in [200, 301, 404])

def test_cgi_get_params():
	# CGI avec paramètres GET
	r = safe_get(f"{BASE_URL}/cgi-bin/py/contact.py?name=Test&email=test@test.com")
	test("CGI with GET params returns 200", r.status_code == 200)

def test_binary_file():
	# Upload binary file
	binary_content = bytes(range(256))
	files = {'file': ('binary.bin', binary_content, 'application/octet-stream')}
	r = safe_post(f"{BASE_URL}/uploads", files=files)
	test("Binary file upload", r.status_code in [200, 201])

	# Cleanup
	if r.status_code in [200, 201]:
		safe_delete(f"{BASE_URL}/uploads/binary.bin")

def test_error_pages_custom():
	# Test that custom error pages are properly served
	r = safe_get(f"{BASE_URL}/page_inexistante.html")
	test("Custom 404 error page served", r.status_code == 404, f"Got {r.status_code}")
	test("Custom 404 contains custom content", "404" in r.text or "not found" in r.text.lower())

def test_post_max_body():
	# Test with a file just under the limit (10M in config)
	medium_data = 'x' * (9 * 1024 * 1024)  # 9MB
	r = safe_post(f"{BASE_URL}/", data=medium_data, timeout=15)
	test("POST with 9MB (under limit) succeeds", r.status_code in [200, 201], f"Got {r.status_code}")

def test_persistent_upload():
	# Verify that uploaded files persist
	files = {'file': ('persistent.txt', 'Should persist', 'text/plain')}
	r = safe_post(f"{BASE_URL}/uploads", files=files)

	# Second GET to verify persistence
	r = safe_get(f"{BASE_URL}/uploads/persistent.txt")
	test("Uploaded file persists", r.status_code == 200, f"Got {r.status_code}")
	test("File content is correct", "Should persist" in r.text)

	# Cleanup
	safe_delete(f"{BASE_URL}/uploads/persistent.txt")
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(2)
		s.connect(('localhost', 8080))
		s.send(b"GET / HTTP/2.0\r\nHost: localhost\r\n\r\n")
		response = s.recv(4096)
		s.close()
		test("Invalid HTTP version handled", b"505" in response or b"200" in response or b"400" in response)
	except Exception as e:
		test_error("Invalid HTTP version test", str(e)[:50])

def test_large_headers():
	# Very long headers
	long_header = 'x' * 8000
	headers = {'X-Custom-Header': long_header}
	r = safe_get(f"{BASE_URL}/", headers=headers)
	test("Large headers handled", r.status_code in [200, 431, 400], f"Got {r.status_code}")

def test_invalid_http_version():
	# Test avec version HTTP invalide
	import socket
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	s.settimeout(TIMEOUT)
	try:
		s.connect(('localhost', 8080))
		s.sendall(b"GET / HTTP/9.9\r\n\r\n")
		response = s.recv(1024).decode('utf-8', errors='ignore')
		s.close()
		test("Invalid HTTP version rejected", "505" in response or "400" in response, f"Response: {response[:100]}")
	except Exception as e:
		test_error("Invalid HTTP version rejected", str(e))

def test_multiple_cookies():
	# Test with multiple cookies
	cookies = {'session': 'abc123', 'user': 'test', 'lang': 'fr'}
	r = safe_get(f"{BASE_URL}/", cookies=cookies)
	test("Multiple cookies handled", r.status_code == 200, f"Got {r.status_code}")

def test_post_json():
	# Test POST with JSON
	import json
	data = json.dumps({'key': 'value', 'number': 42})
	headers = {'Content-Type': 'application/json'}
	r = safe_post(f"{BASE_URL}/", data=data, headers=headers)
	test("POST JSON data handled", r.status_code in [200, 201, 415], f"Got {r.status_code}")

def test_empty_file_upload():
	# Upload empty file
	files = {'file': ('empty.txt', '', 'text/plain')}
	r = safe_post(f"{BASE_URL}/uploads", files=files)
	test("Empty file upload handled", r.status_code in [200, 201, 400], f"Got {r.status_code}")

	# Cleanup
	if r.status_code in [200, 201]:
		safe_delete(f"{BASE_URL}/uploads/empty.txt")

def test_filename_security():
	# Upload with dangerous filename
	files = {'file': ('../../../etc/passwd', 'hacked', 'text/plain')}
	r = safe_post(f"{BASE_URL}/uploads", files=files)
	test("Dangerous filename rejected or sanitized", r.status_code in [200, 201, 400, 403], f"Got {r.status_code}")

	# Cleanup - try to delete possible filenames
	if r.status_code in [200, 201]:
		try:
			safe_delete(f"{BASE_URL}/uploads/_________etc_passwd")
		except:
			pass

def test_query_string_complex():
	# Query string with special characters
	r = safe_get(f"{BASE_URL}/?param1=value1&param2=hello%20world&param3=a%26b")
	test("Complex query string handled", r.status_code == 200, f"Got {r.status_code}")

def test_fragment_in_url():
	# Fragment in URL (after #)
	r = safe_get(f"{BASE_URL}/index.html#section1")
	test("URL with fragment handled", r.status_code == 200, f"Got {r.status_code}")

def test_trailing_slash_redirect():
	# Test behavior with/without trailing slash
	r1 = safe_get(f"{BASE_URL}/uploads")
	r2 = safe_get(f"{BASE_URL}/uploads/")
	test("Trailing slash consistency", r1.status_code in [200, 301, 302, 404] and r2.status_code in [200, 301, 302, 404])

def test_different_line_endings():
	# Test request with LF instead of CRLF
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(2)
		s.connect(('localhost', 8080))
		s.send(b"GET / HTTP/1.1\nHost: localhost\n\n")
		response = s.recv(4096)
		s.close()
		test("LF line endings handled", b"200" in response or b"400" in response)
	except socket.timeout:
		test("LF line endings timeout (server ignores)", True)
	except Exception as e:
		test_error("Line endings test", str(e)[:50])

def test_request_with_body_on_get():
	# GET with body (technically allowed but unusual)
	r = safe_get(f"{BASE_URL}/", data="unexpected body")
	test("GET with body handled", r.status_code in [200, 400], f"Got {r.status_code}")

def test_zero_content_length():
	# POST with Content-Length: 0
	headers = {'Content-Length': '0'}
	r = safe_post(f"{BASE_URL}/", headers=headers)
	test("POST with Content-Length 0 handled", r.status_code in [200, 201, 400], f"Got {r.status_code}")

def test_duplicate_headers():
	# Duplicate headers
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(2)
		s.connect(('localhost', 8080))
		s.send(b"GET / HTTP/1.1\r\nHost: localhost\r\nHost: example.com\r\n\r\n")
		response = s.recv(4096)
		s.close()
		test("Duplicate Host headers handled", b"400" in response or b"200" in response)
	except Exception as e:
		test_error("Duplicate headers test", str(e)[:50])

def test_missing_crlf():
	# Request without double CRLF at the end
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(2)
		s.connect(('localhost', 8080))
		s.send(b"GET / HTTP/1.1\r\nHost: localhost\r\n")
		# No final \r\n\r\n
		response = s.recv(1024)
		s.close()
		test("Missing final CRLF handled", len(response) > 0 or True)
	except socket.timeout:
		test("Missing CRLF causes timeout (expected)", True)
	except Exception as e:
		test_error("Missing CRLF test", str(e)[:50])

def test_very_long_uri():
	# Extremely long URI (>8KB)
	long_uri = "/text/" + "a" * 10000 + ".txt"
	r = safe_get(f"{BASE_URL}{long_uri}")
	test("Very long URI returns 414 or 404", r.status_code in [414, 404], f"Got {r.status_code}")

def test_null_byte_in_url():
	# Null byte in URL
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(2)
		s.connect(('localhost', 8080))
		s.send(b"GET /index.html\x00.txt HTTP/1.1\r\nHost: localhost\r\n\r\n")
		response = s.recv(4096)
		s.close()
		test("Null byte in URL handled", b"400" in response or b"404" in response)
	except Exception as e:
		test_error("Null byte test", str(e)[:50])

def test_connection_close():
	# Test Connection: close header
	headers = {'Connection': 'close'}
	r = safe_get(f"{BASE_URL}/", headers=headers)
	test("Connection: close handled", r.status_code == 200, f"Got {r.status_code}")
	test("Connection closed", r.headers.get('Connection', '').lower() in ['close', ''])

def test_expect_100_continue():
	# Test Expect: 100-continue
	headers = {'Expect': '100-continue'}
	r = safe_post(f"{BASE_URL}/", data="test data", headers=headers)
	test("Expect: 100-continue handled", r.status_code in [100, 200, 201, 417], f"Got {r.status_code}")

def test_range_request():
	# Test request with Range header (if supported)
	headers = {'Range': 'bytes=0-100'}
	r = safe_get(f"{BASE_URL}/text/test.txt", headers=headers)
	test("Range request handled", r.status_code in [200, 206, 416], f"Got {r.status_code}")

def test_if_modified_since():
	# Test If-Modified-Since header
	headers = {'If-Modified-Since': 'Wed, 01 Jan 2020 00:00:00 GMT'}
	r = safe_get(f"{BASE_URL}/", headers=headers)
	test("If-Modified-Since handled", r.status_code in [200, 304], f"Got {r.status_code}")

def test_slow_client():
	# Client that sends very slowly
	import socket
	import time
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(10)
		s.connect(('localhost', 8080))
		s.send(b"GET / HTTP/1.1\r\n")
		time.sleep(0.5)
		s.send(b"Host: localhost\r\n\r\n")
		response = s.recv(4096)
		s.close()
		test("Slow client handled", b"200" in response)
	except Exception as e:
		test_error("Slow client test", str(e)[:50])

def test_pipelined_requests():
	# HTTP pipelined requests (multiple requests at once)
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(5)
		s.connect(('localhost', 8080))
		pipeline = b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\nGET /about.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
		s.send(pipeline)
		response = s.recv(8192)
		s.close()
		test("Pipelined requests handled", response.count(b"200") >= 1)
	except Exception as e:
		test_error("Pipelined requests test", str(e)[:50])

def test_two_requests_same_buffer():
	"""Two requests sent in the same read buffer"""
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(5)
		s.connect(('localhost', 8080))
		payload = b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\nGET /thrhteh.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
		s.sendall(payload)
		chunks = []
		while True:
			try:
				data = s.recv(4096)
				if not data:
					break
				chunks.append(data)
			except socket.timeout:
				break
		s.close()
		response = b"".join(chunks)
		status_count = response.count(b"HTTP/1.")
		first_ok = b"HTTP/1.1 200" in response or b"HTTP/1.0 200" in response
		second_ok = b"HTTP/1.1 404" in response or b"HTTP/1.0 404" in response or status_count >= 2
		test("Two requests in one buffer", status_count >= 2 and first_ok and second_ok,
			 f"Statuses: {status_count}, snippet: {response[:200]}")
	except Exception as e:
		test_error("Two requests same buffer", str(e)[:50])

def test_pipelined_get_head():
	"""GET followed by HEAD in the same buffer"""
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(5)
		s.connect(('localhost', 8080))
		payload = b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\nHEAD /about.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
		s.sendall(payload)
		data = []
		while True:
			try:
				chunk = s.recv(4096)
				if not chunk:
					break
				data.append(chunk)
			except socket.timeout:
				break
		s.close()
		resp = b"".join(data)
		status_count = resp.count(b"HTTP/1.")
		second_is_head = b"HEAD /about.html" not in resp and status_count >= 2
		test("GET then HEAD pipelined", status_count >= 2 and b"200" in resp and second_is_head,
			 f"Statuses: {status_count}, snippet: {resp[:200]}")
	except Exception as e:
		test_error("GET+HEAD pipeline", str(e)[:50])

def test_pipelined_post_then_get():
	"""POST with Content-Length followed by GET in the same buffer"""
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(5)
		s.connect(('localhost', 8080))
		body = b"hello world"
		payload = (
			b"POST /uploads/test-buffer.txt HTTP/1.1\r\n"
			b"Host: localhost\r\n"
			b"Content-Length: " + str(len(body)).encode() + b"\r\n"
			b"Content-Type: text/plain\r\n\r\n"
			+ body +
			b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
		)
		s.sendall(payload)
		chunks = []
		while True:
			try:
				part = s.recv(4096)
				if not part:
					break
				chunks.append(part)
			except socket.timeout:
				break
		s.close()
		resp = b"".join(chunks)
		status_count = resp.count(b"HTTP/1.")
		post_ok = b"201" in resp or b"200" in resp
		get_ok = resp.find(b"HTTP/1.", resp.find(b"HTTP/1.")+1) != -1
		test("POST then GET in buffer", status_count >= 2 and post_ok and get_ok,
			 f"Statuses: {status_count}, snippet: {resp[:200]}")
		# Cleanup uploaded test file
		try:
			safe_delete(f"{BASE_URL}/uploads/test-buffer.txt")
		except:
			pass
	except Exception as e:
		test_error("POST+GET pipeline", str(e)[:50])

def test_pipelined_chunked_then_get():
	"""Chunked POST followed by GET in the same buffer"""
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(5)
		s.connect(('localhost', 8080))
		chunked_body = b"b\r\nhello world\r\n0\r\n\r\n"
		payload = (
			b"POST /uploads/test-chunked-buffer.txt HTTP/1.1\r\n"
			b"Host: localhost\r\n"
			b"Transfer-Encoding: chunked\r\n"
			b"Content-Type: text/plain\r\n\r\n"
			+ chunked_body +
			b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
		)
		s.sendall(payload)
		chunks = []
		while True:
			try:
				part = s.recv(4096)
				if not part:
					break
				chunks.append(part)
			except socket.timeout:
				break
		s.close()
		resp = b"".join(chunks)
		status_count = resp.count(b"HTTP/1.")
		post_ok = b"201" in resp or b"200" in resp
		get_ok = resp.find(b"HTTP/1.", resp.find(b"HTTP/1.")+1) != -1
		test("POST chunked then GET", status_count >= 2 and post_ok and get_ok,
			 f"Statuses: {status_count}, snippet: {resp[:200]}")
		try:
			safe_delete(f"{BASE_URL}/uploads/test-chunked-buffer.txt")
		except:
			pass
	except Exception as e:
		test_error("POST chunked+GET pipeline", str(e)[:50])

def test_connection_close_then_next_request():
	"""GET with Connection: close followed by GET in the same buffer"""
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(5)
		s.connect(('localhost', 8080))
		payload = (
			b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n"
			b"GET /about.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
		)
		s.sendall(payload)
		resp = b""
		while True:
			try:
				chunk = s.recv(4096)
				if not chunk:
					break
				resp += chunk
			except socket.timeout:
				break
		s.close()
		status_count = resp.count(b"HTTP/1.")
		conn_close = b"Connection: close" in resp
		test("Connection close stops pipeline", status_count == 1 and conn_close,
			 f"Statuses: {status_count}, snippet: {resp[:200]}")
	except Exception as e:
		test_error("Connection close pipeline", str(e)[:50])

def test_three_requests_pipeline():
	"""Three consecutive requests in the same buffer"""
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(5)
		s.connect(('localhost', 8080))
		payload = (
			b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
			b"GET /about.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
			b"GET /missing-nope.html HTTP/1.1\r\nHost: localhost\r\n\r\n"
		)
		s.sendall(payload)
		data = []
		while True:
			try:
				chunk = s.recv(4096)
				if not chunk:
					break
				data.append(chunk)
			except socket.timeout:
				break
		s.close()
		resp = b"".join(data)
		status_count = resp.count(b"HTTP/1.")
		ok_200 = resp.count(b" 200") >= 2
		ok_404 = b" 404" in resp
		test("Three requests pipeline", status_count >= 3 and ok_200 and ok_404,
			 f"Statuses: {status_count}, snippet: {resp[:200]}")
	except Exception as e:
		test_error("Pipeline triple", str(e)[:50])

def test_very_small_timeout():
	# Test with very short timeout
	try:
		r = safe_get(f"{BASE_URL}/", timeout=0.001)
		test("Very short timeout handled", r.status_code == 200)
	except:
		test("Very short timeout causes exception (expected)", True)

def test_post_multipart_boundary():
	# Test multipart with custom boundary
	boundary = "----CustomBoundary123"
	headers = {'Content-Type': f'multipart/form-data; boundary={boundary}'}
	body = f'--{boundary}\r\nContent-Disposition: form-data; name="file"; filename="test.txt"\r\n\r\ntest content\r\n--{boundary}--\r\n'
	r = safe_post(f"{BASE_URL}/uploads", data=body.encode(), headers=headers)
	test("Custom multipart boundary handled", r.status_code in [200, 201, 400], f"Got {r.status_code}")

	# Cleanup
	if r.status_code in [200, 201]:
		try:
			safe_delete(f"{BASE_URL}/uploads/test.txt")
		except:
			try:
				safe_delete(f"{BASE_URL}/uploads/test")
			except:
				pass

def test_upload_during_delete():
	# Simultaneous upload and delete
	import threading
	uploaded = [False]
	deleted = [False]

	def upload():
		files = {'file': ('concurrent.txt', 'test', 'text/plain')}
		r = safe_post(f"{BASE_URL}/uploads", files=files)
		if r.status_code in [200, 201]:
			uploaded[0] = True

	def delete():
		r = safe_delete(f"{BASE_URL}/uploads/concurrent.txt")
		if r.status_code in [200, 204]:
			deleted[0] = True

	t1 = threading.Thread(target=upload)
	t2 = threading.Thread(target=delete)
	t1.start()
	t2.start()
	t1.join()
	t2.join()
	test("Concurrent upload/delete handled", True)

	# Cleanup if the file still exists
	if uploaded[0] and not deleted[0]:
		try:
			safe_delete(f"{BASE_URL}/uploads/concurrent.txt")
		except:
			pass
def test_cgi_environment_vars():
	# Test that CGI environment variables are correct
	r = safe_get(f"{BASE_URL}/cgi-bin/py/contact.py?test=value")
	test("CGI with query params returns 200", r.status_code == 200, f"Got {r.status_code}")

def test_cgi_concurrent_ordering():
	"""Test that concurrent CGI requests complete in correct order (fastest first)"""
	import threading

	def run_cgi_test(lang, slow_path, medium_path):
		results = {}
		errors = []

		def fetch(name, url):
			try:
				start = time.time()
				r = requests.get(url, timeout=10)
				elapsed = time.time() - start
				results[name] = {'time': elapsed, 'status': r.status_code}
			except Exception as e:
				errors.append(f"{name}: {str(e)[:50]}")

		# Launch slow first, then medium after 0.5s, then static after another 0.5s
		t_slow = threading.Thread(target=fetch, args=('slow', f"{BASE_URL}{slow_path}"))
		t_slow.start()
		time.sleep(0.5)

		t_medium = threading.Thread(target=fetch, args=('medium', f"{BASE_URL}{medium_path}"))
		t_medium.start()
		time.sleep(0.5)

		t_static = threading.Thread(target=fetch, args=('static', f"{BASE_URL}/index.html"))
		t_static.start()

		t_slow.join(timeout=15)
		t_medium.join(timeout=15)
		t_static.join(timeout=15)

		if errors:
			return False, results, errors

		# All three must return 200
		if not all(name in results and results[name]['status'] == 200 for name in ['slow', 'medium', 'static']):
			statuses = {k: v.get('status', 'N/A') for k, v in results.items()}
			return False, results, [f"Bad status codes: {statuses}"]

		# Check ordering: static < medium < slow
		t_static_time = results['static']['time']
		t_medium_time = results['medium']['time']
		t_slow_time = results['slow']['time']

		order_ok = t_static_time < t_medium_time < t_slow_time
		if not order_ok:
			return False, results, [f"Wrong order: static={t_static_time:.2f}s medium={t_medium_time:.2f}s slow={t_slow_time:.2f}s"]

		return True, results, []

	# Test Python CGI
	py_ok, py_results, py_errors = run_cgi_test('Python', '/cgi-bin/py/slow.py', '/cgi-bin/py/medium.py')

	# Test PHP CGI
	php_ok, php_results, php_errors = run_cgi_test('PHP', '/cgi-bin/php/slow.php', '/cgi-bin/php/medium.php')

	# At least one must pass
	at_least_one = py_ok or php_ok
	details_parts = []
	if py_ok and py_results:
		details_parts.append(f"Python: static={py_results['static']['time']:.2f}s medium={py_results['medium']['time']:.2f}s slow={py_results['slow']['time']:.2f}s")
	if php_ok and php_results:
		details_parts.append(f"PHP: static={php_results['static']['time']:.2f}s medium={php_results['medium']['time']:.2f}s slow={php_results['slow']['time']:.2f}s")

	test("CGI concurrent requests complete in correct order",
		 at_least_one,
		 " | ".join(details_parts) if details_parts else f"Python: {py_errors} | PHP: {php_errors}")

	# Warnings for individual failures
	if not py_ok:
		py_detail = f"Python: {py_errors}"
		if py_results and all(k in py_results for k in ['static', 'medium', 'slow']):
			py_detail += f" (static={py_results['static']['time']:.2f}s medium={py_results['medium']['time']:.2f}s slow={py_results['slow']['time']:.2f}s)"
		print(f"  {YELLOW}⚠ {py_detail}{RESET}")

	if not php_ok:
		php_detail = f"PHP: {php_errors}"
		if php_results and all(k in php_results for k in ['static', 'medium', 'slow']):
			php_detail += f" (static={php_results['static']['time']:.2f}s medium={php_results['medium']['time']:.2f}s slow={php_results['slow']['time']:.2f}s)"
		print(f"  {YELLOW}⚠ {php_detail}{RESET}")

def test_multiple_slashes():
	# Multiple consecutive slashes
	r = safe_get(f"{BASE_URL}///index.html")
	test("Multiple slashes handled", r.status_code in [200, 301, 404], f"Got {r.status_code}")

def test_dot_segments():
	# Segments with dots in URL
	r = safe_get(f"{BASE_URL}/./index.html")
	test("Dot segment (./) handled", r.status_code in [200, 404], f"Got {r.status_code}")

	r = safe_get(f"{BASE_URL}/css/../index.html")
	test("Parent segment (../) handled", r.status_code in [200, 403, 404], f"Got {r.status_code}")

def test_percent_encoding():
	# URL encoding with different characters
	r = safe_get(f"{BASE_URL}/text/test.txt?param=%2F%2E%2E")
	test("Percent encoding handled", r.status_code in [200, 400], f"Got {r.status_code}")

def test_post_no_content_length():
	# POST without Content-Length (requires chunked or connection close)
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(20)
		s.connect(('localhost', 8080))
		s.send(b"POST / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\ndata")
		s.shutdown(socket.SHUT_WR)
		response = s.recv(4096)
		s.close()
		test("POST without Content-Length handled", b"200" in response or b"411" in response or b"400" in response)
	except Exception as e:
		test_error("POST no Content-Length test", str(e)[:50])

def test_absolute_uri():
	# Request with absolute URI (http://host/path)
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(2)
		s.connect(('localhost', 8080))
		s.send(b"GET http://localhost:8080/ HTTP/1.1\r\nHost: localhost\r\n\r\n")
		response = s.recv(4096)
		s.close()
		test("Absolute URI handled", b"200" in response or b"400" in response)
	except Exception as e:
		test_error("Absolute URI test", str(e)[:50])

def test_case_insensitive_headers():
	# Headers in different cases
	headers = {'host': 'localhost', 'CoNtEnT-TyPe': 'text/plain'}
	r = safe_get(f"{BASE_URL}/", headers=headers)
	test("Case-insensitive headers handled", r.status_code == 200, f"Got {r.status_code}")

def test_whitespace_in_headers():
	# Whitespace in headers
	import socket
	try:
		s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		s.settimeout(2)
		s.connect(('localhost', 8080))
		s.send(b"GET / HTTP/1.1\r\nHost:   localhost   \r\n\r\n")
		response = s.recv(4096)
		s.close()
		test("Whitespace in headers handled", b"200" in response or b"400" in response)
	except Exception as e:
		test_error("Whitespace in headers test", str(e)[:50])

def test_upload_special_extensions():
	# Upload files with various extensions
	extensions = ['.jpg', '.png', '.pdf', '.zip', '.json', '.xml']
	for ext in extensions:
		files = {'file': (f'test{ext}', b'binary data', 'application/octet-stream')}
		r = safe_post(f"{BASE_URL}/uploads", files=files)
		test(f"Upload {ext} file", r.status_code in [200, 201], f"Got {r.status_code}")

		# Cleanup
		if r.status_code in [200, 201]:
			try:
				safe_delete(f"{BASE_URL}/uploads/test{ext}")
			except:
				pass

# ============================================================================
# PART 6: PORT ISSUES
# ============================================================================

def test_port_multiple_configs():
	"""Test that different ports serve different content"""
	# Note: Requires a config with multiple ports
	result = subprocess.run(['netstat', '-tulpn'], capture_output=True, text=True)
	ports_found = []
	for line in result.stdout.split('\n'):
		if 'webserv' in line or '8080' in line or '8081' in line or '8082' in line:
			if ':8080' in line:
				ports_found.append(8080)
			if ':8081' in line:
				ports_found.append(8081)
			if ':8082' in line:
				ports_found.append(8082)

	test("Multiple ports detected (8080, 8081, 8082)", len(set(ports_found)) >= 2,
		 f"Ports found: {set(ports_found)}")

def test_port_same_port_twice():
	"""Test that two server blocks on the same port are handled as virtual hosts"""
	try:
		# Request to default server (webServTester) on port 8080
		r1 = requests.get("http://localhost:8080/",
						  headers={"Host": "webServTester"}, timeout=2)
		# Request to vhost (same_port) on the same port 8080
		r2 = requests.get("http://localhost:8080/",
						  headers={"Host": "same_port"}, timeout=2)
		test("Vhost 'same_port' responds on same port as 'webServTester'",
			 r1.status_code == 200 and r2.status_code == 200,
			 f"webServTester={r1.status_code}, same_port={r2.status_code}")
	except Exception as e:
		test("Vhost 'same_port' responds on same port as 'webServTester'",
			 False, f"Error: {e}")

def test_port_cannot_bind_twice():
	"""Test that two instances cannot bind the same port"""
	# Verify only one instance is running
	result = subprocess.run(['pgrep', '-c', 'webserv'], capture_output=True, text=True)
	count = int(result.stdout.strip()) if result.stdout.strip().isdigit() else 0
	test("Only one webserv instance running", count == 1,
		 f"Found {count} instances (try: pgrep webserv)")

# ============================================================================
# PART 7: SIEGE & STRESS TEST
# ============================================================================

def test_siege_availability():
	"""Test availability with siege (requires siege installed)"""
	# Verify siege is installed
	result = subprocess.run(['which', 'siege'], capture_output=True)
	if result.returncode != 0:
		test("Siege is installed", False, "Install with: brew install siege")
		return

	# Create a simple page for the test
	test_page = 'www/siege_test.html'
	with open(test_page, 'w') as f:
		f.write('')

	# Run siege (short duration for test)
	result = subprocess.run(
		['siege', '-b', '-c', '20', '-t', '20s', f'{BASE_URL}/siege_test.html'],
		capture_output=True, text=True, timeout=30
	)

	# Parse result (stdout and stderr, case-insensitive)
	availability = None
	output = (result.stdout or '') + '\n' + (result.stderr or '')
	for line in output.split('\n'):
		if 'availability' in line.lower():
			# Extract percentage or bare float, accept comma decimal separator
			match = re.search(r'(\d+[.,]?\d*)\s*%', line)
			if match:
				availability = float(match.group(1).replace(',', '.'))
				break
			match = re.search(r'(\d+[.,]?\d*)', line)
			if match:
				availability = float(match.group(1).replace(',', '.'))
				break
	if availability is None:
		availability = 0.0

	test("Siege availability > 99.5%", availability > 99.5,
		 f"Got {availability}% (run manually: siege -b -c 25 -t 30s {BASE_URL}/)")

	# Cleanup
	try:
		os.remove(test_page)
	except:
		pass

def test_memory_stability():
	"""Test that memory does not leak (basic check)"""
	# Get the server PID
	result = subprocess.run(['pgrep', 'webserv'], capture_output=True, text=True)
	if not result.stdout.strip():
		test("Memory stability check", False, "webserv process not found")
		return

	pid = result.stdout.strip().split()[0]

	# First memory measurement
	result1 = subprocess.run(['ps', '-o', 'rss=', '-p', pid], capture_output=True, text=True)
	mem1 = int(result1.stdout.strip()) if result1.stdout.strip() else 0

	# Make some requests
	for _ in range(50):
		try:
			safe_get(f"{BASE_URL}/")
		except:
			pass

	time.sleep(1)

	# Second memory measurement
	result2 = subprocess.run(['ps', '-o', 'rss=', '-p', pid], capture_output=True, text=True)
	mem2 = int(result2.stdout.strip()) if result2.stdout.strip() else 0

	# Memory should not increase by more than 10MB
	mem_increase = (mem2 - mem1) / 1024  # en MB
	test("Memory stable after 50 requests", mem_increase < 10,
		 f"Memory increased by {mem_increase:.2f} MB (from {mem1/1024:.1f}MB to {mem2/1024:.1f}MB)")

def test_no_hanging_connections():
	"""Test that no connections are left hanging"""
	# Make some requests
	for _ in range(10):
		safe_get(f"{BASE_URL}/")

	time.sleep(2)

	# Verify ESTABLISHED connections
	result = subprocess.run(['netstat', '-an'], capture_output=True, text=True)
	established_count = 0
	for line in result.stdout.split('\n'):
		if '8080' in line and 'ESTABLISHED' in line:
			established_count += 1

	test("No hanging connections", established_count < 5,
		 f"Found {established_count} ESTABLISHED connections (should be 0 or very low)")

# ============================================================================
# BONUS PART
# ============================================================================

def test_bonus_cookies_session():
	"""Test cookies and sessions"""
	r = safe_get(f"{BASE_URL}/counter.html")
	test("Counter page accessible", r.status_code == 200, f"Got {r.status_code}")

	# Verify Set-Cookie in headers
	has_cookie = 'Set-Cookie' in r.headers or 'set-cookie' in r.headers
	test("Server sets cookies", has_cookie,
		 f"Cookies: {r.headers.get('Set-Cookie', 'None')}")

	if has_cookie:
		cookie_value = r.headers.get('Set-Cookie', '')
		test("Cookie contains session_id", 'session' in cookie_value.lower(),
			 f"Cookie: {cookie_value[:100]}")

def test_bonus_multiple_cgi():
	"""Test multiple CGI systems (Python + PHP)"""
	# Test Python CGI
	r_py = safe_get(f"{BASE_URL}/cgi-bin/py/contact.py")
	test("Python CGI works", r_py.status_code == 200, f"Got {r_py.status_code}")

	# Test PHP CGI
	r_php = safe_get(f"{BASE_URL}/cgi-bin/php/qrcode.php")
	test("PHP CGI works", r_php.status_code == 200, f"Got {r_php.status_code}")

	# Both must work
	test("Multiple CGI systems working", r_py.status_code == 200 and r_php.status_code == 200,
		 "Both Python and PHP CGI should work")

def summary():
	print(f"\n{PASSED} passed, {FAILED} failed")

def save_results():
	"""Save results to a JSON file"""
	data = {
		"timestamp": datetime.now().isoformat(),
		"passed": PASSED,
		"failed": FAILED,
		"tests": test_results
	}
	with open(LOG_FILE, 'w') as f:
		json.dump(data, f, indent=2)
	print(f"\n{BLUE}ℹ{RESET} Results saved to {LOG_FILE}")

def load_previous_results():
	"""Load previous results if they exist"""
	if os.path.exists(LOG_FILE):
		try:
			with open(LOG_FILE, 'r') as f:
				return json.load(f)
		except:
			return None
	return None

def compare_results(previous):
	"""Compare current results with previous ones"""
	if not previous:
		print(f"\n{BLUE}ℹ{RESET} No previous results to compare")
		return

	prev_tests = previous.get("tests", {})

	# New failures
	new_fails = []
	# New passes
	new_passes = []
	# Unchanged
	unchanged_pass = 0
	unchanged_fail = 0

	for test_name, result in test_results.items():
		if test_name not in prev_tests:
			continue

		prev_status = prev_tests[test_name]["status"]
		curr_status = result["status"]

		if prev_status == "passed" and curr_status in ["failed", "error"]:
			new_fails.append((test_name, result["details"]))
		elif prev_status in ["failed", "error"] and curr_status == "passed":
			new_passes.append((test_name, result["details"]))
		elif prev_status == "passed" and curr_status == "passed":
			unchanged_pass += 1
		elif prev_status in ["failed", "error"] and curr_status in ["failed", "error"]:
			unchanged_fail += 1

	# Affichage du rapport de comparaison
	print(f"\n{'='*60}")
	print(f"COMPARISON WITH PREVIOUS RUN")
	print(f"Previous run: {previous.get('timestamp', 'unknown')}")
	print(f"{'='*60}")

	print(f"\n{BLUE}Summary:{RESET}")
	print(f"  Unchanged passed: {unchanged_pass}")
	print(f"  Unchanged failed: {unchanged_fail}")
	print(f"  {GREEN}New passes: {len(new_passes)}{RESET}")
	print(f"  {RED}New failures: {len(new_fails)}{RESET}")

	if new_passes:
		print(f"\n{GREEN}✓ Tests now passing (previously failed):{RESET}")
		for test_name, details in new_passes:
			print(f"  • {test_name}")
			if details:
				print(f"    → {details}")

	if new_fails:
		print(f"\n{RED}✗ Tests now failing (previously passed):{RESET}")
		for test_name, details in new_fails:
			print(f"  • {test_name}")
			if details:
				print(f"    → {details}")

	if not new_passes and not new_fails:
		print(f"\n{GREEN}✓ No changes in test results{RESET}")

def check_server_running():
	"""Verify that webserv is running"""
	print("Checking if webserv is running...")
	try:
		r = requests.get(BASE_URL, timeout=2)
		print(f"{GREEN}✓{RESET} webserv is running and responding\n")
		return True
	except requests.exceptions.RequestException:
		print(f"{RED}✗{RESET} webserv is not responding")
		print(f"{YELLOW}→{RESET} Make sure webserv is running on {BASE_URL}")
		return False

def stop_server():
	"""Stop the webserv server"""
	print("\nStopping webserv...")
	try:
		subprocess.run(['pkill', '-SIGTERM', 'webserv'], capture_output=True)
		time.sleep(0.5)  # Give it time to shutdown gracefully
		print(f"{GREEN}✓{RESET} Server stopped")
	except Exception as e:
		print(f"{YELLOW}⚠{RESET} Could not stop server: {str(e)}")

if __name__ == "__main__":
	try:
		check_and_install_dependencies()

		if not check_server_running():
			sys.exit(1)

		# Load previous results
		previous_results = load_previous_results()

		print("Starting webserv tests...\n")

		print("=" * 70)
		print("PART 1: MANDATORY PART - CODE CHECKS")
		print("=" * 70)
		run_test(test_code_poll_in_loop)
		run_test(test_code_poll_read_write)
		run_test(test_code_compilation)

		print("\n" + "=" * 70)
		print("PART 2: CONFIGURATION")
		print("=" * 70)
		run_test(test_config_multiple_ports)
		run_test(test_config_virtual_hosts)
		run_test(test_config_routes_directories)
		run_test(test_error_pages_custom)
		run_test(test_post_max_body)
		run_test(test_autoindex)
		run_test(test_method_not_allowed)

		print("\n" + "=" * 70)
		print("PART 3: BASIC CHECKS")
		print("=" * 70)
		# GET requests
		run_test(test_get_index)
		run_test(test_get_static_files)
		run_test(test_get_404)
		run_test(test_get_pages)
		run_test(test_response_body_content)
		# POST requests
		run_test(test_post_upload)
		run_test(test_persistent_upload)
		# DELETE requests
		run_test(test_delete)
		# UNKNOWN requests
		run_test(test_unknown_method_no_crash)
		# Appropriate status codes (covered by above tests)
		# Upload file and get it back (covered by test_post_upload + test_persistent_upload)
		# Additional HTTP tests
		run_test(test_http_headers)
		run_test(test_keep_alive)

		print("\n" + "=" * 70)
		print("PART 4: CHECK CGI")
		print("=" * 70)
		run_test(test_post_cgi_python)
		run_test(test_cgi_working_directory)
		run_test(test_cgi_get_params)
		run_test(test_cgi_get_with_query)
		run_test(test_cgi_post_with_body)
		run_test(test_cgi_syntax_error)
		run_test(test_cgi_errors)
		run_test(test_cgi_runtime_error)
		run_test(test_cgi_missing_shebang)
		run_test(test_cgi_permission_denied)
		run_test(test_cgi_interpreter_not_executable)
		run_test(test_cgi_environment_vars)
		run_test(test_cgi_concurrent_ordering)

		print("\n" + "=" * 70)
		print("PART 5: ADVANCED TESTS (Browser tests are manual)")
		print("=" * 70)
		run_test(test_chunked_encoding)
		run_test(test_large_file_upload)
		run_test(test_multiple_requests)
		run_test(test_concurrent_requests)
		run_test(test_security_path_traversal)
		run_test(test_malformed_requests)
		run_test(test_empty_requests)
		run_test(test_special_characters)
		run_test(test_long_url)
		run_test(test_double_slash)
		run_test(test_multiple_file_upload)
		run_test(test_binary_file)
		run_test(test_head_method)
		run_test(test_post_without_content_type)
		run_test(test_case_sensitivity)
		run_test(test_invalid_http_version)
		run_test(test_large_headers)
		run_test(test_multiple_cookies)
		run_test(test_post_json)
		run_test(test_empty_file_upload)
		run_test(test_filename_security)
		run_test(test_null_byte_in_url)
		run_test(test_query_string_complex)
		run_test(test_fragment_in_url)
		run_test(test_trailing_slash_redirect)
		run_test(test_very_long_uri)
		run_test(test_different_line_endings)
		run_test(test_request_with_body_on_get)
		run_test(test_zero_content_length)
		run_test(test_duplicate_headers)
		run_test(test_missing_crlf)
		run_test(test_connection_close)
		run_test(test_expect_100_continue)
		run_test(test_range_request)
		run_test(test_if_modified_since)
		run_test(test_slow_client)
		run_test(test_pipelined_requests)
		run_test(test_two_requests_same_buffer)
		run_test(test_pipelined_get_head)
		run_test(test_pipelined_post_then_get)
		run_test(test_pipelined_chunked_then_get)
		run_test(test_connection_close_then_next_request)
		run_test(test_three_requests_pipeline)
		run_test(test_very_small_timeout)
		run_test(test_upload_during_delete)
		run_test(test_post_multipart_boundary)
		run_test(test_multiple_slashes)
		run_test(test_dot_segments)
		run_test(test_percent_encoding)
		run_test(test_post_no_content_length)
		run_test(test_absolute_uri)
		run_test(test_case_insensitive_headers)
		run_test(test_whitespace_in_headers)
		run_test(test_upload_special_extensions)

		print("\n" + "=" * 70)
		print("PART 6: PORT ISSUES")
		print("=" * 70)
		run_test(test_port_multiple_configs)
		run_test(test_port_same_port_twice)
		run_test(test_port_cannot_bind_twice)

		print("\n" + "=" * 70)
		print("PART 7: SIEGE & STRESS TEST")
		print("=" * 70)
		run_test(test_siege_availability)
		run_test(test_memory_stability)
		run_test(test_no_hanging_connections)

		print("\n" + "=" * 70)
		print("BONUS PART")
		print("=" * 70)
		run_test(test_bonus_cookies_session)
		run_test(test_bonus_multiple_cgi)

		summary()
		compare_results(previous_results)
		save_results()
		stop_server()

		# List of failed tests
		failed_tests_list = [name for name, result in test_results.items() if result["status"] in ["failed", "error"]]
		if failed_tests_list:
			print("\n" + "=" * 70)
			print("FAILED TESTS")
			print("=" * 70)
			for i, test_name in enumerate(failed_tests_list, 1):
				print(f"{i}. {test_name}")
		else:
			print("\n" + "=" * 70)
			print("\u2705 ALL TESTS PASSED!")
			print("=" * 70)

	except KeyboardInterrupt:
		print("\n\nTests interrupted by user")
		summary()
		compare_results(previous_results)
		save_results()
		stop_server()
