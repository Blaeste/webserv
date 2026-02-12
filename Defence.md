# 🛡️ WebServ - Defense Evaluation Guide

> **Note**: Ce guide est basé sur le scale officiel 42 et notre implémentation réelle.
>
> **Implémentation**: GET, POST, DELETE, HEAD, OPTIONS | CGI 90s timeout | No keep-alive | Virtual Hosts (bonus)

---

## 📋 Table des Matières

1. [Mandatory Part - Code Review](#mandatory-part---code-review)
2. [Configuration Tests](#configuration-tests)
3. [Basic Checks](#basic-checks)
4. [CGI Tests](#cgi-tests)
5. [Browser Tests](#browser-tests)
6. [Port Issues](#port-issues)
7. [Siege & Stress Tests](#siege--stress-tests)
8. [Bonus Part](#bonus-part)
9. [Quick Reference](#quick-reference)

---

## Mandatory Part - Code Review

### 1. Installation & Prerequisites

```bash
# Install siege (on macOS)
brew install siege

# Verify compilation
make re
# Should compile without warnings or re-linking issues
```

### 2. HTTP Server Basics - Questions & Answers

**Q: Qu'est-ce qu'un serveur HTTP ?**
- Programme qui écoute sur un port TCP et répond aux requêtes HTTP/1.1
- Parse les requêtes (request line, headers, body)
- Génère des réponses avec status codes, headers, body
- Sert des fichiers statiques et exécute des scripts CGI

**Q: Quelle fonction utilisez-vous pour l'I/O Multiplexing ?**
- **Réponse**: `poll()`
- **Localisation**: `Server.cpp` ligne 77
- **Alternatives possibles**: select(), kqueue(), epoll()

**Q: Comment fonctionne poll() ?**
```cpp
// 1. Créer un tableau de struct pollfd
std::vector<struct pollfd> _pollFds;

// 2. Ajouter les fd à surveiller
struct pollfd pfd;
pfd.fd = socket_fd;
pfd.events = POLLIN | POLLOUT;  // Surveiller lecture ET écriture
_pollFds.push_back(pfd);

// 3. Appeler poll()
int ret = poll(&_pollFds[0], _pollFds.size(), timeout_ms);

// 4. Vérifier les événements
if (_pollFds[i].revents & POLLIN)   // Données disponibles en lecture
if (_pollFds[i].revents & POLLOUT)  // Prêt pour écriture
```

**Q: Un seul poll() pour tout ?**
- ✅ **OUI** - Un seul poll() dans la boucle principale
- ✅ Surveille SIMULTANÉMENT lecture ET écriture sur tous les fd
- ✅ Gère listen sockets, client sockets, et CGI pipes

### 3. ✅ CRITICAL CHECKS (Note: 0 si échoué)

#### Check #1: poll() vérifie READ et WRITE simultanément
```cpp
// Server.cpp - Boucle principale
int ret = poll(&_pollFds[0], _pollFds.size(), 1000);

// Pour chaque fd, on vérifie POLLIN et POLLOUT
if (revents & POLLIN)   handleClientRead(i);
if (revents & POLLOUT)  handleClientWrite(i);
```
✅ **VALIDÉ** - Un seul poll() surveille les deux événements

#### Check #2: Un seul read/write par client par poll()
```cpp
// Server.cpp - handleClientRead()
ssize_t bytesRead = recv(fd, buffer, sizeof(buffer), 0);
// ← UN SEUL recv() par itération de poll()

// Server.cpp - handleClientWrite()
ssize_t bytesSent = send(clientFd, data, size, 0);
// ← UN SEUL send() par itération de poll()
```
✅ **VALIDÉ** - Pas de boucle read/write, une seule opération I/O

#### Check #3: Gestion des erreurs I/O et suppression du client
```cpp
// Exemple: Server.cpp - handleClientRead()
ssize_t bytesRead = recv(fd, buffer, sizeof(buffer), 0);
if (bytesRead <= 0) {
    removeClient(clientFd, pollIndex);  // ✅ Client supprimé
    return;
}
```
✅ **VALIDÉ** - Vérification de `-1` ET `0`, client supprimé

#### Check #4: errno N'est PAS vérifié après I/O
```bash
# Chercher errno dans le code
grep -n "errno" srcs/server/Server.cpp
grep -n "errno" srcs/server/Client.cpp
# Résultat: Aucune vérification d'errno après read/recv/write/send
```
✅ **VALIDÉ** - Pas de vérification d'errno (interdit par le sujet)

#### Check #5: Aucun read/write sans poll()
```cpp
// Tous les I/O socket passent par poll():
// - accept() appelé si POLLIN sur listen socket
// - recv() appelé si POLLIN sur client socket
// - send() appelé si POLLOUT sur client socket

// ✅ Exception autorisée: Fichiers disque réguliers
// (read/write sur fichiers normaux ne nécessitent pas poll)
```
✅ **VALIDÉ** - Tous les I/O réseau passent par poll()

### 4. Code Flow: poll() → read/write

**Flux pour READ:**
```
1. poll() détecte POLLIN sur fd client
2. handleClientRead(pollIndex) appelé
3. ONE recv() sur le socket
4. Données ajoutées au buffer client
5. Si requête complète → parsing
6. Retour à poll()
```

**Flux pour WRITE:**
```
1. poll() détecte POLLOUT sur fd client
2. handleClientWrite(pollIndex) appelé
3. ONE send() avec les données de réponse
4. Si envoi incomplet → garder POLLOUT actif
5. Si envoi complet → removeClient() (Connection: close)
6. Retour à poll()
```

### 5. Checklist de Validation ✅

- ✅ Un seul poll() dans la boucle principale
- ✅ poll() surveille POLLIN et POLLOUT simultanément
- ✅ Un seul read/write par client par poll()
- ✅ Gestion correcte des erreurs (check -1 ET 0)
- ✅ Client supprimé en cas d'erreur I/O
- ✅ Pas de vérification d'errno
- ✅ Tous les I/O socket passent par poll()
- ✅ Compilation sans re-link (Makefile correct)

---

## Configuration Tests

### 1. ✅ HTTP Status Codes

**Notre implémentation:**
- 200 OK
- 201 Created (upload)
- 204 No Content (DELETE)
- 301/302 Redirections
- 400 Bad Request
- 403 Forbidden
- 404 Not Found
- 405 Method Not Allowed
- 413 Payload Too Large
- 500 Internal Server Error
- 501 Not Implemented
- 504 Gateway Timeout (CGI 90s)

**Test:**
```bash
# 404 Not Found
curl -v http://localhost:8080/nonexistent

# 405 Method Not Allowed (POST non autorisé sur /)
curl -X POST http://localhost:8080/

# 413 Payload Too Large
dd if=/dev/zero bs=1M count=20 | curl -X POST --data-binary @- http://localhost:8080/uploads
```

### 2. ✅ Multiple Servers with Different Ports

**Configuration:**
```nginx
server {
    listen 8080;
    server_name webserv;
    root ./www;
}

server {
    listen 8081;
    server_name webserv2;
    root ./www2;
}
```

**Test:**
```bash
curl http://localhost:8080/  # Serveur 1
curl http://localhost:8081/  # Serveur 2
```

### 3. ✅ Multiple Servers with Different Hostnames (Virtual Hosts - Bonus)

**Configuration:**
```nginx
server {
    listen 8080;
    server_name example.com;
    root ./www_example;
}

server {
    listen 8080;
    server_name test.com;
    root ./www_test;
}
```

**Test:**
```bash
# Virtual host matching basé sur Host header
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/
curl --resolve test.com:8080:127.0.0.1 http://test.com:8080/

# Ou avec header Host manuel
curl -H "Host: example.com" http://localhost:8080/
curl -H "Host: test.com" http://localhost:8080/
```

### 4. ✅ Setup Default Error Pages

**Configuration:**
```nginx
server {
    listen 8080;
    error_page 404 /error_pages/404.html;
    error_page 403 /error_pages/403.html;
    error_page 500 /error_pages/500.html;
}
```

**Test:**
```bash
# Vérifier page 404 custom
curl http://localhost:8080/nonexistent
# Devrait afficher le contenu de www/error_pages/404.html

# Vérifier page 403
curl http://localhost:8080/forbidden_dir
```

### 5. ✅ Limit Client Body Size

**Configuration:**
```nginx
server {
    listen 8080;
    client_max_body_size 10M;  # Limite globale

    location /uploads {
        client_max_body_size 5M;  # Limite spécifique à la location
    }
}
```

**Test:**
```bash
# Body < limite → 201 Created
curl -X POST -H "Content-Type: text/plain" \
  --data "Small body" http://localhost:8080/uploads

# Body > limite → 413 Payload Too Large
dd if=/dev/zero bs=1M count=20 | \
  curl -X POST -H "Content-Type: text/plain" \
  --data-binary @- http://localhost:8080/uploads
```

### 6. ✅ Setup Routes to Different Directories

**Configuration:**
```nginx
server {
    listen 8080;

    location / {
        root ./www;
    }

    location /uploads {
        root ./uploads;
    }

    location /cgi-bin/php {
        root ./cgi-bin/php;
    }
}
```

**Test:**
```bash
curl http://localhost:8080/index.html        # Sert ./www/index.html
curl http://localhost:8080/uploads/file.txt  # Sert ./uploads/file.txt
```

### 7. ✅ Default File for Directories (index)

**Configuration:**
```nginx
location / {
    root ./www;
    index index.html index.htm default.html;
}
```

**Test:**
```bash
# Requête sur / cherche index.html, puis index.htm, puis default.html
curl http://localhost:8080/
```

### 8. ✅ List of Accepted Methods per Route

**Configuration:**
```nginx
location / {
    allowed_methods GET HEAD OPTIONS;
}

location /uploads {
    allowed_methods GET POST DELETE;
}
```

**Test:**
```bash
# GET autorisé sur / → 200
curl http://localhost:8080/

# POST non autorisé sur / → 405
curl -X POST http://localhost:8080/

# DELETE autorisé sur /uploads → 204
curl -X DELETE http://localhost:8080/uploads/test.txt
```

---

## Basic Checks

### ✅ GET, POST, DELETE, HEAD, OPTIONS

```bash
# GET
curl http://localhost:8080/index.html

# POST
curl -X POST -F "file=@test.txt" http://localhost:8080/uploads

# DELETE
curl -X DELETE http://localhost:8080/uploads/test.txt

# HEAD (pas de body dans la réponse)
curl -I http://localhost:8080/index.html

# OPTIONS (retourne Allow header)
curl -X OPTIONS -v http://localhost:8080/
# Devrait afficher: Allow: GET, POST, DELETE, HEAD, OPTIONS
```

### ✅ UNKNOWN Requests

```bash
# Méthode inconnue → 501 Not Implemented (pas de crash)
curl -X TRACE http://localhost:8080/
curl -X CUSTOM http://localhost:8080/
```

### ✅ Appropriate Status Codes

Vérifier que chaque opération retourne le bon code:
- 200 pour GET réussi
- 201 pour upload réussi
- 204 pour DELETE réussi
- 404 pour fichier inexistant
- 405 pour méthode non autorisée

### ✅ Upload and Retrieve File

```bash
# 1. Upload
curl -X POST -F "file=@photo.jpg" http://localhost:8080/uploads
# Status: 201 Created

# 2. Vérifier présence (avec autoindex)
curl http://localhost:8080/uploads/
# Devrait lister photo.jpg

# 3. Download
curl -o downloaded.jpg http://localhost:8080/uploads/photo.jpg
# Status: 200 OK

# 4. Vérifier intégrité
diff photo.jpg downloaded.jpg
# Pas de différence
```

---

## CGI Tests

### ✅ CGI Working with GET and POST

**Configuration:**
```nginx
location /cgi-bin/php {
    root ./cgi-bin/php;
    allowed_methods GET POST;
    cgi_extension .php;
    cgi_path /usr/bin/php-cgi;
}

location /cgi-bin/py {
    root ./cgi-bin/py;
    allowed_methods GET POST;
    cgi_extension .py;
    cgi_path /usr/bin/python3;
}
```

**Test GET:**
```bash
# Python CGI avec query string
curl "http://localhost:8080/cgi-bin/py/test.py?name=Alice&age=25"

# PHP CGI
curl "http://localhost:8080/cgi-bin/php/info.php"
```

**Test POST:**
```bash
# POST avec body
curl -X POST -d "name=Bob&email=bob@test.com" \
  http://localhost:8080/cgi-bin/py/post.py
```

### ✅ CGI Run in Correct Directory

**Vérification:**
```python
# cgi-bin/py/test_pwd.py
#!/usr/bin/python3
import os
print("Content-Type: text/plain\r\n\r\n")
print("Current dir:", os.getcwd())
```

```bash
curl http://localhost:8080/cgi-bin/py/test_pwd.py
# Devrait afficher: Current dir: /path/to/webserv/cgi-bin/py
```

**Notre implémentation**: CGI.cpp fait `chdir()` au répertoire du script

### ✅ CGI Error Handling

**Test: Script avec erreur de syntaxe**
```python
# cgi-bin/py/syntax_error.py
#!/usr/bin/python3
print("Content-Type: text/plain")
print()
invalid syntax here  # ← Erreur volontaire
```

```bash
curl http://localhost:8080/cgi-bin/py/syntax_error.py
# Status: 500 Internal Server Error
```

**Test: Script infini (timeout)**
```python
# cgi-bin/py/infinite.py
#!/usr/bin/python3
print("Content-Type: text/plain\r\n\r\n")
while True:
    pass  # Boucle infinie
```

```bash
curl http://localhost:8080/cgi-bin/py/infinite.py
# Attend 90 secondes puis:
# Status: 504 Gateway Timeout
```

**Test: Script qui crash**
```python
# cgi-bin/py/crash.py
#!/usr/bin/python3
import sys
sys.exit(1)  # Exit avec code erreur
```

```bash
curl http://localhost:8080/cgi-bin/py/crash.py
# Status: 500 Internal Server Error
```

### ✅ CGI Environment Variables

Le serveur doit passer les variables CGI standard (RFC 3875):
- `REQUEST_METHOD`
- `QUERY_STRING`
- `SCRIPT_NAME`
- `SCRIPT_FILENAME`
- `PATH_INFO`
- `SERVER_NAME`
- `SERVER_PORT`
- `SERVER_PROTOCOL`
- `CONTENT_TYPE`
- `CONTENT_LENGTH`
- `HTTP_*` (tous les headers)
- `REDIRECT_STATUS` (requis pour PHP-CGI)

**Test:**
```python
# cgi-bin/py/env.py
#!/usr/bin/python3
import os
print("Content-Type: text/plain\r\n\r\n")
print("REQUEST_METHOD:", os.environ.get('REQUEST_METHOD'))
print("QUERY_STRING:", os.environ.get('QUERY_STRING'))
print("SERVER_NAME:", os.environ.get('SERVER_NAME'))
```

---

## Browser Tests

### ✅ Open in Browser

```
http://localhost:8080/
```

**Vérifications:**
1. Ouvrir Developer Tools → Network tab
2. Recharger la page
3. Vérifier:
   - Request headers (Host, User-Agent, etc.)
   - Response headers (Content-Type, Content-Length, Connection: close)
   - Status codes corrects
   - Toutes les ressources chargées (CSS, images, JS)

### ✅ Serve Fully Static Website

- HTML, CSS, images, JS doivent tous se charger
- Navigation entre pages doit fonctionner
- Pas d'erreurs 404 dans la console

### ✅ Wrong URL

```
http://localhost:8080/nonexistent.html
```
→ Devrait afficher page 404 custom

### ✅ List Directory

```
http://localhost:8080/uploads/
```
→ Si `autoindex on` : affiche liste HTML des fichiers

### ✅ Redirected URL

**Configuration:**
```nginx
location /old-page {
    return https://example.com/new-page;
}
```

```
http://localhost:8080/old-page
```
→ Status 301/302, redirection visible dans browser

---

## Port Issues

### ✅ Multiple Ports with Different Websites

**Configuration:**
```nginx
server {
    listen 8080;
    root ./www;
    index index.html;
}

server {
    listen 8081;
    root ./www2;
    index other.html;
}
```

**Test:**
```bash
# Port 8080 → site 1
curl http://localhost:8080/

# Port 8081 → site 2
curl http://localhost:8081/
```

### ✅ Same Port Multiple Times in Config

**Test invalide:**
```nginx
server {
    listen 8080;
}

server {
    listen 8080;  # ← Même port dans le même fichier
}
```

**Résultat attendu:**
- ✅ Si virtual hosts (server_name différents) : OK
- ❌ Sinon: Configuration invalide ou comportement défini

### ✅ Multiple Server Instances

**Tester:**
```bash
# Lancer serveur 1
./webserv config1.conf &

# Lancer serveur 2 avec config différente mais ports communs
./webserv config2.conf &
# Devrait échouer: "Address already in use"
```

---

## Siege & Stress Tests

### ✅ Install Siege

```bash
# macOS
brew install siege

# Vérifier
siege --version
```

### ✅ Availability > 99.5%

**Test:**
```bash
# Stress test avec siege (batch mode)
siege -b -c 10 -r 100 http://localhost:8080/

# Résultats:
# - Transactions: 1000 (total requests)
# - Availability: > 99.50% ✅
# - Elapsed time: X seconds
# - Response time: < 0.5s
# - Failed transactions: 0
```

### ✅ No Memory Leaks

**Monitor pendant siege:**
```bash
# Terminal 1: Lancer siege
siege -b -t 60S http://localhost:8080/

# Terminal 2: Monitor mémoire
watch -n 1 'ps aux | grep webserv | grep -v grep'
```

**Vérification:**
- La mémoire (RSS/VSZ) ne doit PAS augmenter indéfiniment
- Après le test, la mémoire devrait revenir au niveau initial

**Valgrind (optionnel mais recommandé):**
```bash
valgrind --leak-check=full --show-leak-kinds=all ./webserv config/default.conf
# Lancer quelques requêtes
# Ctrl+C pour arrêter
# Vérifier: "All heap blocks were freed -- no leaks are possible"
```

### ✅ No Hanging Connections

```bash
# Vérifier connexions pendant siege
netstat -an | grep 8080 | wc -l

# Après siege, toutes les connexions devraient être fermées
# (Connection: close après chaque réponse)
```

### ✅ Siege Indefinitely

```bash
# Test de longue durée
siege -b -t 300S http://localhost:8080/
# 5 minutes sans restart nécessaire
```

---

## Bonus Part

**⚠️ N'évaluer que si la partie obligatoire est parfaite !**

### ✅ Cookies and Session Management

**Fonctionnalité:**
- Génération de session IDs
- Cookies envoyés dans les réponses
- Stockage de données de session
- Timeout de sessions

**Test:**
```bash
# Première requête
curl -v -c cookies.txt http://localhost:8080/session-test
# Vérifier Set-Cookie dans la réponse

# Deuxième requête avec cookie
curl -v -b cookies.txt http://localhost:8080/session-test
# Session devrait être maintenue
```

**Vérification dans le code:**
- `Server.cpp`: SessionData avec timeout
- `HttpResponse.cpp`: Set-Cookie header
- Session cleanup automatique

### ✅ Multiple CGI Types

**Notre implémentation:**
- ✅ Python (.py)
- ✅ PHP (.php)

**Configuration:**
```nginx
location /cgi-bin/php {
    cgi_extension .php;
    cgi_path /usr/bin/php-cgi;
}

location /cgi-bin/py {
    cgi_extension .py;
    cgi_path /usr/bin/python3;
}
```

**Test:**
```bash
curl http://localhost:8080/cgi-bin/php/test.php
curl http://localhost:8080/cgi-bin/py/test.py
```

---

## Quick Reference

### Key Files to Show

```
srcs/server/Server.cpp    → Boucle poll(), gestion I/O
srcs/server/Client.cpp    → États client, timeouts
srcs/http/HttpRequest.cpp → Parsing requêtes, chunked
srcs/http/HttpResponse.cpp → Construction réponses
srcs/cgi/CGI.cpp          → Exécution CGI, timeout 90s
srcs/config/Config.cpp    → Parsing configuration
config/default.conf       → Configuration par défaut
```

### Important Numbers

- **CGI Timeout**: 90 secondes
- **Méthodes**: GET, POST, DELETE, HEAD, OPTIONS
- **No Keep-Alive**: Connection: close après chaque réponse
- **Default Port**: 8080
- **Body Size Limit**: Configurable (défaut 10M)
- **Siege Availability**: > 99.5%

### Commands Cheat Sheet

```bash
# Compilation
make re

# Lancer serveur
./webserv config/default.conf

# Tests basiques
curl http://localhost:8080/
curl -X POST -F "file=@test.txt" http://localhost:8080/uploads
curl -X DELETE http://localhost:8080/uploads/test.txt

# Stress test
siege -b -c 10 -r 100 http://localhost:8080/

# Memory check
valgrind --leak-check=full ./webserv config/default.conf

# Check connections
netstat -an | grep 8080
```

### Critical Points

✅ **MUST PASS (sinon note 0):**
1. poll() vérifie READ et WRITE simultanément
2. Un seul read/write par client par poll()
3. Pas de vérification d'errno après I/O
4. Client supprimé en cas d'erreur I/O
5. Tous les I/O socket passent par poll()
6. Compilation sans re-link

✅ **Configuration complète testée**
✅ **GET, POST, DELETE fonctionnent**
✅ **CGI fonctionne avec GET et POST**
✅ **Upload/Download de fichiers**
✅ **Stress test > 99.5% availability**
✅ **Pas de memory leaks**

---

**Bonne défense ! 🚀**
