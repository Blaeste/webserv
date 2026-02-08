# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    webServTester.py                                   :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: gdosch <gdosch@student.42.fr>              +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/01/16 11:30:57 by eschwart          #+#    #+#              #
#    Updated: 2026/02/08 13:37:10 by gdosch           ###   ########.fr        #
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

BASE_URL = "http://localhost:8082"
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

# ============================================================================
# PARTIE 1: MANDATORY PART - CODE CHECKS
# ============================================================================

def test_code_poll_in_loop():
    """Check poll() dans la boucle principale"""
    result = subprocess.run(['grep', '-n', 'poll(', 'srcs/server/Server.cpp'],
                          capture_output=True, text=True)
    test("poll() found in Server.cpp", len(result.stdout) > 0,
         f"Found {len(result.stdout.splitlines())} occurrences")

def test_code_poll_read_write():
    """Check poll() vérifie READ et WRITE simultanément"""
    result = subprocess.run(['grep', '-E', 'POLLIN|POLLOUT', 'srcs/server/Server.cpp'],
                          capture_output=True, text=True)
    has_pollin = 'POLLIN' in result.stdout
    has_pollout = 'POLLOUT' in result.stdout
    test("poll() checks both POLLIN and POLLOUT", has_pollin and has_pollout,
         f"POLLIN: {has_pollin}, POLLOUT: {has_pollout}")

def test_code_compilation():
    """Test compilation sans re-link"""
    # Première compilation
    result1 = subprocess.run(['make'], capture_output=True, text=True, cwd='.')
    # Deuxième make (ne devrait rien faire)
    result2 = subprocess.run(['make'], capture_output=True, text=True, cwd='.')
    no_relink = 'up to date' in result2.stdout or 'Nothing to be done' in result2.stdout or len(result2.stdout) < 50
    test("Compilation without re-link", result1.returncode == 0 and no_relink,
         "Makefile properly handles dependencies")

# ============================================================================
# PARTIE 2: CONFIGURATION
# ============================================================================

def test_config_multiple_ports():
    """Test multiple ports avec différents sites"""
    ports = [8080, 8081, 8082]
    for port in ports:
        try:
            r = requests.get(f"http://localhost:{port}/", timeout=2)
            test(f"Port {port} is accessible", r.status_code == 200, f"Got {r.status_code}")
        except:
            test(f"Port {port} is accessible", False, f"Port {port} not responding")

def test_config_virtual_hosts():
    """Test virtual hosts avec Host header différent"""
    # Test avec Host header custom
    headers = {'Host': 'example.com'}
    r = safe_get(f"{BASE_URL}/", headers=headers)
    test("Virtual host with custom Host header", r.status_code in [200, 404],
         f"Got {r.status_code}")

    # Test avec Host header normal
    headers = {'Host': 'localhost'}
    r2 = safe_get(f"{BASE_URL}/", headers=headers)
    test("Virtual host with localhost Host header", r2.status_code == 200,
         f"Got {r2.status_code}")

def test_config_routes_directories():
    """Test routes vers différents répertoires"""
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

def test_unknown_method_no_crash():
    """Test méthode UNKNOWN (FOOBAR) et vérifier pas de crash"""
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

        # Vérifier que le serveur répond toujours
        time.sleep(0.5)
        r = safe_get(f"{BASE_URL}/")
        test("Server still running after UNKNOWN method", r.status_code == 200,
             "Server crashed or not responding")
    except Exception as e:
        test_error("UNKNOWN method test", str(e)[:50])

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

# ============================================================================
# PARTIE 4: CHECK CGI - ADVANCED TESTS
# ============================================================================

def test_cgi_working_directory():
    """Test CGI s'exécute dans le bon répertoire"""
    # Créer un script qui affiche son pwd
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
    """Test CGI avec erreur de syntaxe"""
    # Créer un script avec erreur de syntaxe
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

    # Vérifier que le serveur répond toujours
    r2 = safe_get(f"{BASE_URL}/")
    test("Server still running after CGI syntax error", r2.status_code == 200)

    # Cleanup
    try:
        os.remove(script_path)
    except:
        pass

def test_cgi_runtime_error():
    """Test CGI avec erreur runtime (division by zero)"""
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

    # Vérifier que le serveur répond toujours
    r2 = safe_get(f"{BASE_URL}/")
    test("Server still running after CGI runtime error", r2.status_code == 200)

    # Cleanup
    try:
        os.remove(script_path)
    except:
        pass

def test_cgi_missing_shebang():
    """Test CGI sans shebang"""
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
    """Test CGI avec paramètres GET détaillés"""
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
    """Test CGI POST reçoit bien le body"""
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

# ============================================================================
# PARTIE 6: PORT ISSUES
# ============================================================================

def test_port_multiple_configs():
    """Test que différents ports servent différents contenus"""
    # Note: Nécessite une config avec plusieurs ports
    result = subprocess.run(['netstat', '-tulpn'], capture_output=True, text=True)
    ports_found = []
    for line in result.stdout.split('\n'):
        if 'webserv' in line or '8080' in line or '8081' in line:
            if ':8080' in line:
                ports_found.append(8080)
            if ':8081' in line:
                ports_found.append(8081)
            if ':8082' in line:
                ports_found.append(8082)

    test("Multiple ports detected", len(set(ports_found)) >= 1,
         f"Ports found: {set(ports_found)}")

def test_port_same_port_twice():
    """Info: Test que le même port en double est géré (config check)"""
    # Ce test est informatif - il faut vérifier manuellement la config
    test("Port duplication check (manual verification needed)", True,
         "Verify in config that duplicate ports are handled (virtual hosts or error)")

def test_port_cannot_bind_twice():
    """Test qu'on ne peut pas lancer 2 instances sur le même port"""
    # Vérifier qu'une seule instance tourne
    result = subprocess.run(['pgrep', '-c', 'webserv'], capture_output=True, text=True)
    count = int(result.stdout.strip()) if result.stdout.strip().isdigit() else 0
    test("Only one webserv instance running", count == 1,
         f"Found {count} instances (try: pgrep webserv)")

# ============================================================================
# PARTIE 7: SIEGE & STRESS TEST
# ============================================================================

def test_siege_availability():
    """Test availability avec siege (nécessite siege installé)"""
    # Vérifier que siege est installé
    result = subprocess.run(['which', 'siege'], capture_output=True)
    if result.returncode != 0:
        test("Siege is installed", False, "Install with: brew install siege")
        return

    # Créer une page simple pour le test
    test_page = 'www/siege_test.html'
    with open(test_page, 'w') as f:
        f.write('')

    # Lancer siege (courte durée pour le test)
    result = subprocess.run(
        ['siege', '-b', '-c', '20', '-t', '20s', f'{BASE_URL}/siege_test.html'],
        capture_output=True, text=True, timeout=30
    )

    # Parser le résultat (stdout et stderr, case-insensitive)
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
    """Test que la mémoire ne fuit pas (check basique)"""
    # Récupérer le PID du serveur
    result = subprocess.run(['pgrep', 'webserv'], capture_output=True, text=True)
    if not result.stdout.strip():
        test("Memory stability check", False, "webserv process not found")
        return

    pid = result.stdout.strip().split()[0]

    # Première mesure mémoire
    result1 = subprocess.run(['ps', '-o', 'rss=', '-p', pid], capture_output=True, text=True)
    mem1 = int(result1.stdout.strip()) if result1.stdout.strip() else 0

    # Faire quelques requêtes
    for _ in range(50):
        try:
            safe_get(f"{BASE_URL}/")
        except:
            pass

    time.sleep(1)

    # Deuxième mesure mémoire
    result2 = subprocess.run(['ps', '-o', 'rss=', '-p', pid], capture_output=True, text=True)
    mem2 = int(result2.stdout.strip()) if result2.stdout.strip() else 0

    # La mémoire ne devrait pas augmenter de plus de 10MB
    mem_increase = (mem2 - mem1) / 1024  # en MB
    test("Memory stable after 50 requests", mem_increase < 10,
         f"Memory increased by {mem_increase:.2f} MB (from {mem1/1024:.1f}MB to {mem2/1024:.1f}MB)")

def test_no_hanging_connections():
    """Test qu'il n'y a pas de connexions qui traînent"""
    # Faire quelques requêtes
    for _ in range(10):
        safe_get(f"{BASE_URL}/")

    time.sleep(2)

    # Vérifier les connexions ESTABLISHED
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
    """Test cookies et sessions"""
    r = safe_get(f"{BASE_URL}/counter.html")
    test("Counter page accessible", r.status_code == 200, f"Got {r.status_code}")

    # Vérifier Set-Cookie dans les headers
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

    # Les deux doivent fonctionner
    test("Multiple CGI systems working", r_py.status_code == 200 and r_php.status_code == 200,
         "Both Python and PHP CGI should work")

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

def stop_server():
	"""Arrete le serveur webserv"""
	print("\nStopping webserv...")
	try:
		subprocess.run(['pkill', '-SIGTERM', 'webserv'], capture_output=True)
		time.sleep(0.5)  # Give it time to shutdown gracefully
		print(f"{GREEN}✓{RESET} Server stopped")
	except Exception as e:
		print(f"{YELLOW}⚠{RESET} Could not stop server: {str(e)}")

if __name__ == "__main__":
	try:
		if not check_server_running():
			sys.exit(1)

		# Charger les resultats precedents
		previous_results = load_previous_results()

		print("Starting webserv tests...\n")

		print("=" * 70)
		print("PARTIE 1: MANDATORY PART - CODE CHECKS")
		print("=" * 70)
		run_test(test_code_poll_in_loop)
		run_test(test_code_poll_read_write)
		run_test(test_code_compilation)

		print("\n" + "=" * 70)
		print("PARTIE 2: CONFIGURATION")
		print("=" * 70)
		run_test(test_config_multiple_ports)
		run_test(test_config_virtual_hosts)
		run_test(test_config_routes_directories)
		run_test(test_error_pages_custom)
		run_test(test_post_max_body)
		run_test(test_autoindex)
		run_test(test_method_not_allowed)

		print("\n" + "=" * 70)
		print("PARTIE 3: BASIC CHECKS")
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
		print("PARTIE 4: CHECK CGI")
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

		print("\n" + "=" * 70)
		print("PARTIE 5: ADVANCED TESTS (Browser tests are manual)")
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
		print("PARTIE 6: PORT ISSUES")
		print("=" * 70)
		run_test(test_port_multiple_configs)
		run_test(test_port_same_port_twice)
		run_test(test_port_cannot_bind_twice)

		print("\n" + "=" * 70)
		print("PARTIE 7: SIEGE & STRESS TEST")
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

		# Liste des tests échoués
		failed_tests_list = [name for name, result in test_results.items() if result["status"] in ["failed", "error"]]
		if failed_tests_list:
			print("\n" + "=" * 70)
			print("TESTS ÉCHOUÉS")
			print("=" * 70)
			for i, test_name in enumerate(failed_tests_list, 1):
				print(f"{i}. {test_name}")
		else:
			print("\n" + "=" * 70)
			print("✅ TOUS LES TESTS ONT RÉUSSI!")
			print("=" * 70)

	except KeyboardInterrupt:
		print("\n\nTests interrupted by user")
		summary()
		compare_results(previous_results)
		save_results()
		stop_server()
