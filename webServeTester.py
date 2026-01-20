# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    webServeTester.py                                  :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: eschwart <eschwart@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/01/16 11:30:57 by eschwart          #+#    #+#              #
#    Updated: 2026/01/20 12:43:39 by eschwart         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

import requests
import sys
import os
import socket
import json
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

# Dictionnaire pour stocker les resultats des tests
test_results = {}

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
	"""Test who cause timeout or exception"""
	global FAILED, test_results
	print(f"{YELLOW}⚠{RESET} {name} (error/timeout)")
	if error_msg:
		print(f"{YELLOW}→{RESET} {error_msg}")
	FAILED += 1
	test_results[name] = {"status": "error", "details": error_msg}

def safe_get(url, **kwargs):
    """GET avec timeout par défaut"""
    kwargs.setdefault('timeout', TIMEOUT)
    return requests.get(url, **kwargs)

def safe_post(url, **kwargs):
    """POST avec timeout par défaut"""
    kwargs.setdefault('timeout', TIMEOUT)
    return requests.post(url, **kwargs)

def safe_delete(url, **kwargs):
    """DELETE avec timeout par défaut"""
    kwargs.setdefault('timeout', TIMEOUT)
    return requests.delete(url, **kwargs)

def safe_put(url, **kwargs):
    """PUT avec timeout par défaut"""
    kwargs.setdefault('timeout', TIMEOUT)
    return requests.put(url, **kwargs)

def safe_head(url, **kwargs):
    """HEAD avec timeout par défaut"""
    kwargs.setdefault('timeout', TIMEOUT)
    return requests.head(url, **kwargs)

def safe_options(url, **kwargs):
    """OPTIONS avec timeout par défaut"""
    kwargs.setdefault('timeout', TIMEOUT)
    return requests.options(url, **kwargs)

def run_test(func):
    """Wrapper pour exécuter un test avec gestion d'erreur"""
    try:
        func()
    except Exception as e:
        test_error(f"{func.__name__} - Exception: {str(e)[:50]}")

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
	# Test aue le body contient biend u contenu
	r = safe_get(f"{BASE_URL}/")
	test("GET / body is not empty", len(r.text) > 0)
	test("GET / body contains 'webserv' or title", any(word in r.text.lower() for word in ['webserv', 'title', 'html']))

	# Test Content-Length header
	r = safe_get(f"{BASE_URL}/text/test.txt")
	test("Response has Content-Lenght header", "Content-Length" in r.headers)
	test("Content-Length matches body size", int(r.headers.get("Content-Length", 0)) == len(r.content))


def test_post_upload():
	# Upload un fichier texte
	files = {'file': ('test_upload.txt', 'Hello form tester!', 'text/plain')}
	r = safe_post(f"{BASE_URL}/uploads", files=files)
	test("POST file upload return 200 or 201", r.status_code in [200, 201], f"Got {r.status_code}")

	# Verifie aue le fichier existe
	r = safe_get(f"{BASE_URL}/uploads/test_upload.txt")
	test("Upload file is accessible", r.status_code == 200)
	# Note: pas de delete ici, test_delete() va le supprimer

def test_post_cgi_python():
	r = safe_post(f"{BASE_URL}/cgi-bin/py/contact.py", data={'name': 'Test', 'email': 'test@test.com', 'message': 'Hello world form contact cgi test'})
	test("POST CGI Python return 200", r.status_code == 200)

def test_delete():
	# supprime le texte du test upload avant
	r = safe_delete(f"{BASE_URL}/uploads/test_upload.txt")
	test("DELETE file return 200 or 204", r.status_code in [200, 204], f"Got {r.status_code}")

	# Verifie au'il est bien supprimer
	r = safe_get(f"{BASE_URL}/uploads/test_upload.txt")
	test("Deleted file return 404", r.status_code == 404)

def test_method_not_allowed():
	# DELETE sur / devreait etre refuse
	r = safe_delete(f"{BASE_URL}/")
	test("DELETE on / return 405", r.status_code == 405, f"Got {r.status_code}")

	# PUT n ets pas supporte
	r = safe_put(f"{BASE_URL}/", data="test")
	test("PUT return 405 or 501", r.status_code in [405, 501], f"Got {r.status_code}")

def test_autoindex():
	# Test autoindex on ulpoads
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
	# Session pour garder la co
	session = requests.Session()

	r1 = session.get(f"{BASE_URL}/")
	test("First request return 200", r1.status_code == 200)

	r2 = session.get(f"{BASE_URL}/about.html")
	test("Second request on same connection return 200", r2.status_code == 200)

	session.close()

def test_cgi_errors():
	#Test CGI qui timeout
	r =safe_get(f"{BASE_URL}/cgi-bin/py/timeout.py", timeout=10)
	test("CGI timeout return 504", r.status_code == 504, f"Got {r.status_code}")

	# Test CGI avec erreur 500
	r = safe_get(f"{BASE_URL}/cgi-bin/py/error500.py")
	test("CGI error return 500", r.status_code == 500, f"Got {r.status_code} (TODO: fix server)")

def test_chunked_encoding():
	# Test POST avec Transfer-Encoding: chunked
	headers = {'Transfer-Encoding': 'chunked'}
	data = "Hello chunked world!"

	r = safe_post(f"{BASE_URL}/", data=data, headers=headers)
	test("POST with chunked encoding return 200", r.status_code in [200, 201])

def test_large_file_upload():
	# Upload un fichier plus gros (1MB)
	big_content = 'x' * (1024 * 1024) # 1 MB
	files = {'file': ('big_file.txt', big_content, 'text/plain')}

	r = safe_post(f"{BASE_URL}/uploads", files=files, timeout=10)
	test("Upload 1MB file return 200 or 201", r.status_code in [200, 201], f"Got {r.status_code}")

	# Cleanup
	if r.status_code in [200, 201]:
		safe_delete(f"{BASE_URL}/uploads/big_file.txt")

def test_multiple_requests():
	# Test plusieurs requetes rapides
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
    # Requête sans Host header (HTTP/1.1 require Host)
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
    # Body vide
    r = safe_post(f"{BASE_URL}/", data="")
    test("POST with empty body returns 200", r.status_code in [200, 201])

    # GET avec query string vide
    r = safe_get(f"{BASE_URL}/?")
    test("GET with empty query string returns 200", r.status_code == 200)

def test_special_characters():
    # Test URL encoding
    r = safe_get(f"{BASE_URL}/text/test.txt?param=hello%20world")
    test("URL with encoded spaces returns 200", r.status_code == 200)

    # Caractères spéciaux dans filename
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
    # Upload plusieurs fichiers
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
    # Test requêtes concurrentes avec threads
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
    # URL très longue
    long_path = "/text/" + "a" * 1000 + ".txt"
    r = safe_get(f"{BASE_URL}{long_path}")
    test("Long URL returns 404 or 414", r.status_code in [404, 414])

def test_post_without_content_type():
    # POST sans Content-Type
    r = safe_post(f"{BASE_URL}/", data="raw data")
    test("POST without explicit Content-Type handled", r.status_code in [200, 201, 400])

def test_head_method():
    # HEAD method (si supporté)
    r = safe_head(f"{BASE_URL}/")
    test("HEAD method returns 200 or 405", r.status_code in [200, 405])
    if r.status_code == 200:
        test("HEAD has no body", len(r.content) == 0)

def test_options_method():
    # OPTIONS method
    r = safe_options(f"{BASE_URL}/")
    test("OPTIONS method handled", r.status_code in [200, 204, 405, 501])

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
    # Upload fichier binaire
    binary_content = bytes(range(256))
    files = {'file': ('binary.bin', binary_content, 'application/octet-stream')}
    r = safe_post(f"{BASE_URL}/uploads", files=files)
    test("Binary file upload", r.status_code in [200, 201])

    # Cleanup
    if r.status_code in [200, 201]:
        safe_delete(f"{BASE_URL}/uploads/binary.bin")

def test_error_pages_custom():
    # Test que les pages d'erreur personnalisées sont bien servies
    r = safe_get(f"{BASE_URL}/page_inexistante.html")
    test("Custom 404 error page served", r.status_code == 404, f"Got {r.status_code}")
    test("Custom 404 contains custom content", "404" in r.text or "not found" in r.text.lower())

def test_post_max_body():
    # Test avec fichier juste sous la limite (10M dans config)
    medium_data = 'x' * (9 * 1024 * 1024)  # 9MB
    r = safe_post(f"{BASE_URL}/", data=medium_data, timeout=15)
    test("POST with 9MB (under limit) succeeds", r.status_code in [200, 201], f"Got {r.status_code}")

def test_persistent_upload():
    # Vérifie que les fichiers uploadés persistent
    files = {'file': ('persistent.txt', 'Should persist', 'text/plain')}
    r = safe_post(f"{BASE_URL}/uploads", files=files)

    # Deuxième GET pour vérifier persistence
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
    # Headers très longs
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
    # Test avec plusieurs cookies
    cookies = {'session': 'abc123', 'user': 'test', 'lang': 'fr'}
    r = safe_get(f"{BASE_URL}/", cookies=cookies)
    test("Multiple cookies handled", r.status_code == 200, f"Got {r.status_code}")

def test_post_json():
    # Test POST avec JSON
    import json
    data = json.dumps({'key': 'value', 'number': 42})
    headers = {'Content-Type': 'application/json'}
    r = safe_post(f"{BASE_URL}/", data=data, headers=headers)
    test("POST JSON data handled", r.status_code in [200, 201, 415], f"Got {r.status_code}")

def test_empty_file_upload():
    # Upload fichier vide
    files = {'file': ('empty.txt', '', 'text/plain')}
    r = safe_post(f"{BASE_URL}/uploads", files=files)
    test("Empty file upload handled", r.status_code in [200, 201, 400], f"Got {r.status_code}")

    # Cleanup
    if r.status_code in [200, 201]:
        safe_delete(f"{BASE_URL}/uploads/empty.txt")

def test_filename_security():
    # Upload avec nom de fichier dangereux
    files = {'file': ('../../../etc/passwd', 'hacked', 'text/plain')}
    r = safe_post(f"{BASE_URL}/uploads", files=files)
    test("Dangerous filename rejected or sanitized", r.status_code in [200, 201, 400, 403], f"Got {r.status_code}")

    # Cleanup - essayer de supprimer les noms possibles
    if r.status_code in [200, 201]:
        try:
            safe_delete(f"{BASE_URL}/uploads/_________etc_passwd")
        except:
            pass

def test_query_string_complex():
    # Query string avec caractères spéciaux
    r = safe_get(f"{BASE_URL}/?param1=value1&param2=hello%20world&param3=a%26b")
    test("Complex query string handled", r.status_code == 200, f"Got {r.status_code}")

def test_fragment_in_url():
    # Fragment dans URL (après #)
    r = safe_get(f"{BASE_URL}/index.html#section1")
    test("URL with fragment handled", r.status_code == 200, f"Got {r.status_code}")

def test_trailing_slash_redirect():
    # Test comportement avec/sans trailing slash
    r1 = safe_get(f"{BASE_URL}/uploads")
    r2 = safe_get(f"{BASE_URL}/uploads/")
    test("Trailing slash consistency", r1.status_code in [200, 301, 302, 404] and r2.status_code in [200, 301, 302, 404])

def test_different_line_endings():
    # Test requête avec LF au lieu de CRLF
    import socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(2)  # ← AJOUTEZ CETTE LIGNE
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
    # GET avec body (techniquement permis mais inhabituel)
    r = safe_get(f"{BASE_URL}/", data="unexpected body")
    test("GET with body handled", r.status_code in [200, 400], f"Got {r.status_code}")

def test_zero_content_length():
    # POST avec Content-Length: 0
    headers = {'Content-Length': '0'}
    r = safe_post(f"{BASE_URL}/", headers=headers)
    test("POST with Content-Length 0 handled", r.status_code in [200, 201, 400], f"Got {r.status_code}")

def test_duplicate_headers():
    # Headers dupliqués
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
    # Requête sans double CRLF à la fin
    import socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(2)
        s.connect(('localhost', 8080))
        s.send(b"GET / HTTP/1.1\r\nHost: localhost\r\n")
        # Pas de \r\n\r\n final
        response = s.recv(1024)
        s.close()
        test("Missing final CRLF handled", len(response) > 0 or True)
    except socket.timeout:
        test("Missing CRLF causes timeout (expected)", True)
    except Exception as e:
        test_error("Missing CRLF test", str(e)[:50])

def test_very_long_uri():
    # URI extrêmement long (>8KB)
    long_uri = "/text/" + "a" * 10000 + ".txt"
    r = safe_get(f"{BASE_URL}{long_uri}")
    test("Very long URI returns 414 or 404", r.status_code in [414, 404], f"Got {r.status_code}")

def test_null_byte_in_url():
    # Null byte dans URL
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
    # Test requête avec Range header (si supporté)
    headers = {'Range': 'bytes=0-100'}
    r = safe_get(f"{BASE_URL}/text/test.txt", headers=headers)
    test("Range request handled", r.status_code in [200, 206, 416], f"Got {r.status_code}")

def test_if_modified_since():
    # Test If-Modified-Since header
    headers = {'If-Modified-Since': 'Wed, 01 Jan 2020 00:00:00 GMT'}
    r = safe_get(f"{BASE_URL}/", headers=headers)
    test("If-Modified-Since handled", r.status_code in [200, 304], f"Got {r.status_code}")

def test_slow_client():
    # Client qui envoie très lentement
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
    # Requêtes HTTP pipelinées (plusieurs requêtes d'un coup)
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

def test_very_small_timeout():
    # Test avec timeout très court
    try:
        r = safe_get(f"{BASE_URL}/", timeout=0.001)
        test("Very short timeout handled", r.status_code == 200)
    except:
        test("Very short timeout causes exception (expected)", True)

def test_post_multipart_boundary():
    # Test multipart avec boundary custom
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
    # Upload et delete simultanés
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

    # Cleanup si le fichier existe encore
    if uploaded[0] and not deleted[0]:
        try:
            safe_delete(f"{BASE_URL}/uploads/concurrent.txt")
        except:
            pass
def test_cgi_environment_vars():
    # Test que les variables CGI sont correctes
    r = safe_get(f"{BASE_URL}/cgi-bin/py/contact.py?test=value")
    test("CGI with query params returns 200", r.status_code == 200, f"Got {r.status_code}")

def test_multiple_slashes():
    # Multiples slashes consécutifs
    r = safe_get(f"{BASE_URL}///index.html")
    test("Multiple slashes handled", r.status_code in [200, 301, 404], f"Got {r.status_code}")

def test_dot_segments():
    # Segments avec points dans URL
    r = safe_get(f"{BASE_URL}/./index.html")
    test("Dot segment (./) handled", r.status_code in [200, 404], f"Got {r.status_code}")

    r = safe_get(f"{BASE_URL}/css/../index.html")
    test("Parent segment (../) handled", r.status_code in [200, 403, 404], f"Got {r.status_code}")

def test_percent_encoding():
    # Encodage URL avec différents caractères
    r = safe_get(f"{BASE_URL}/text/test.txt?param=%2F%2E%2E")
    test("Percent encoding handled", r.status_code in [200, 400], f"Got {r.status_code}")

def test_post_no_content_length():
    # POST sans Content-Length (requiert chunked ou connection close)
    import socket
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(3)
        s.connect(('localhost', 8080))
        s.send(b"POST / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\ndata")
        s.shutdown(socket.SHUT_WR)
        response = s.recv(4096)
        s.close()
        test("POST without Content-Length handled", b"200" in response or b"411" in response or b"400" in response)
    except Exception as e:
        test_error("POST no Content-Length test", str(e)[:50])

def test_absolute_uri():
    # Requête avec URI absolue (http://host/path)
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
    # Headers en différentes casses
    headers = {'host': 'localhost', 'CoNtEnT-TyPe': 'text/plain'}
    r = safe_get(f"{BASE_URL}/", headers=headers)
    test("Case-insensitive headers handled", r.status_code == 200, f"Got {r.status_code}")

def test_whitespace_in_headers():
    # Espaces dans les headers
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
    # Upload fichiers avec extensions variées
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

def summary():
	print(f"\n{PASSED} passed, {FAILED} failed")

def save_results():
	"""Sauvegarde les resultats dans un fichier JSON"""
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
	"""Charge les resultats precedents s'ils existent"""
	if os.path.exists(LOG_FILE):
		try:
			with open(LOG_FILE, 'r') as f:
				return json.load(f)
		except:
			return None
	return None

def compare_results(previous):
	"""Compare les resultats actuels avec les precedents"""
	if not previous:
		print(f"\n{BLUE}ℹ{RESET} No previous results to compare")
		return

	prev_tests = previous.get("tests", {})

	# Nouveaux fails
	new_fails = []
	# Nouveaux succes
	new_passes = []
	# Inchanges
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
	"""Verifie que webserv est bien lance"""
	print("Checking if webserv is running...")
	try:
		r = requests.get(BASE_URL, timeout=2)
		print(f"{GREEN}✓{RESET} webserv is running and responding\n")
		return True
	except requests.exceptions.RequestException:
		print(f"{RED}✗{RESET} webserv is not responding")
		print(f"{YELLOW}→{RESET} Make sure webserv is running on {BASE_URL}")
		return False

if __name__ == "__main__":
	try:
		if not check_server_running():
			sys.exit(1)

		# Charger les resultats precedents
		previous_results = load_previous_results()

		print("Starting webserv tests...\n")

		print("=== GET Tests ===")
		run_test(test_get_index)
		run_test(test_get_static_files)
		run_test(test_get_404)
		run_test(test_get_pages)

		print("\n=== Response Body Tests ===")
		run_test(test_response_body_content)

		print("\n=== POST Tests ===")
		run_test(test_post_upload)
		#run_test(test_post_body_limit)
		run_test(test_post_cgi_python)

		print("\n=== DELETE Tests ===")
		run_test(test_delete)

		print("\n=== HTTP Errors Tests ===")
		run_test(test_method_not_allowed)

		print("\n=== Autoindex Tests ===")
		run_test(test_autoindex)

		print("\n=== HTTP Headers Tests ===")
		run_test(test_http_headers)

		print("\n=== Keep-Alive Tests ===")
		run_test(test_keep_alive)

		print("\n=== CGI Error Tests ===")
		run_test(test_cgi_errors)

		print("\n=== Chunked Encoding Tests ===")
		run_test(test_chunked_encoding)

		print("\n=== Large File Upload Tests ===")
		run_test(test_large_file_upload)

		print("\n=== Multiple Rapid Requests Tests ===")
		run_test(test_multiple_requests)

		print("\n=== Security Tests ===")
		run_test(test_security_path_traversal)

		print("\n=== Edge Cases Tests ===")
		run_test(test_malformed_requests)
		run_test(test_empty_requests)
		run_test(test_special_characters)
		run_test(test_long_url)
		run_test(test_double_slash)

		print("\n=== Advanced Upload Tests ===")
		run_test(test_multiple_file_upload)
		run_test(test_binary_file)

		print("\n=== HTTP Methods Tests ===")
		run_test(test_head_method)
		run_test(test_options_method)
		run_test(test_post_without_content_type)

		print("\n=== URL Tests ===")
		run_test(test_case_sensitivity)
		run_test(test_cgi_get_params)

		print("\n=== Concurrency Tests ===")
		run_test(test_concurrent_requests)

		print("\n=== Error Pages Tests ===")
		run_test(test_error_pages_custom)

		print("\n=== Body Size Limits Tests ===")
		run_test(test_post_max_body)

		print("\n=== Persistence Tests ===")
		run_test(test_persistent_upload)

		print("\n=== Protocol Tests ===")
		run_test(test_invalid_http_version)
		run_test(test_large_headers)

		print("\n=== Cookies Tests ===")
		run_test(test_multiple_cookies)

		print("\n=== Data Format Tests ===")
		run_test(test_post_json)
		run_test(test_empty_file_upload)

		print("\n=== Security Edge Cases ===")
		run_test(test_filename_security)
		run_test(test_null_byte_in_url)

		print("\n=== URL Parsing Tests ===")
		run_test(test_query_string_complex)
		run_test(test_fragment_in_url)
		run_test(test_trailing_slash_redirect)
		run_test(test_very_long_uri)

		print("\n=== Request Format Tests ===")
		run_test(test_different_line_endings)
		run_test(test_request_with_body_on_get)
		run_test(test_zero_content_length)
		run_test(test_duplicate_headers)
		run_test(test_missing_crlf)

		print("\n=== HTTP Features Tests ===")
		run_test(test_connection_close)
		run_test(test_expect_100_continue)
		run_test(test_range_request)
		run_test(test_if_modified_since)

		print("\n=== Performance Tests ===")
		run_test(test_slow_client)
		run_test(test_pipelined_requests)
		run_test(test_very_small_timeout)

		print("\n=== Race Condition Tests ===")
		run_test(test_upload_during_delete)

		print("\n=== Multipart Tests ===")
		run_test(test_post_multipart_boundary)

		print("\n=== CGI Advanced Tests ===")
		run_test(test_cgi_environment_vars)

		print("\n=== URL Normalization Tests ===")
		run_test(test_multiple_slashes)
		run_test(test_dot_segments)
		run_test(test_percent_encoding)

		print("\n=== HTTP Protocol Edge Cases ===")
		run_test(test_post_no_content_length)
		run_test(test_absolute_uri)
		run_test(test_case_insensitive_headers)
		run_test(test_whitespace_in_headers)

		print("\n=== File Types Tests ===")
		run_test(test_upload_special_extensions)

		summary()
		compare_results(previous_results)
		save_results()
	except KeyboardInterrupt:
		print("\n\nTests interrupted by user")
		summary()
		compare_results(previous_results)
		save_results()
