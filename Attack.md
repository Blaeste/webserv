# 🎯 WebServ - Guide d'Attaques et Tests de Sécurité

> **Note**: Ce document liste les vecteurs d'attaque potentiels pour identifier les faiblesses du serveur HTTP.
> Utilisez-le pour tester et renforcer votre implémentation avant l'évaluation.

---

## 📋 Table des Matières

1. [Attaques sur le Parsing HTTP](#1-attaques-sur-le-parsing-http)
2. [Attaques sur le Body](#2-attaques-sur-le-body)
3. [Attaques CGI](#3-attaques-cgi)
4. [Attaques Path Traversal](#4-attaques-path-traversal)
5. [Attaques Upload](#5-attaques-upload)
6. [Attaques DELETE](#6-attaques-delete)
7. [Race Conditions & Concurrency](#7-race-conditions--concurrency)
8. [Attaques Configuration](#8-attaques-configuration)
9. [Memory Leaks & Crashes](#9-memory-leaks--crashes)
10. [Tests de Conformité HTTP/1.1](#10-tests-de-conformité-http11)
11. [Checks Norme & Compilation](#11-checks-norme--compilation)
12. [Bonus Attacks (Virtual Hosts)](#12-bonus-attacks-virtual-hosts)
13. [Défense et Protection](#défense-et-protection)

---

## 🎯 **1. ATTAQUES SUR LE PARSING HTTP**

### 1.1 Requêtes Malformées

**Objectif**: Faire planter le parser ou obtenir un comportement indéfini

```bash
# Request line sans CRLF
echo -n "GET / HTTP/1.1" | nc localhost 8080

# Méthode invalide
echo -e "INVALID / HTTP/1.1\r\n\r\n" | nc localhost 8080

# Version HTTP invalide
echo -e "GET / HTTP/2.0\r\n\r\n" | nc localhost 8080
echo -e "GET / HTTP/1.2\r\n\r\n" | nc localhost 8080
echo -e "GET / HTTP/0.9\r\n\r\n" | nc localhost 8080

# URI trop longue (> MAX_URI_LENGTH)
python3 -c "print('GET /' + 'A'*10000 + ' HTTP/1.1\r\n\r\n')" | nc localhost 8080

# Headers sans CRLF (utilise LF seulement)
echo -e "GET / HTTP/1.1\nHost: localhost\n\n" | nc localhost 8080

# Request line incomplète
echo -e "GET /\r\n\r\n" | nc localhost 8080
echo -e "GET\r\n\r\n" | nc localhost 8080

# Espaces multiples dans request line
echo -e "GET    /    HTTP/1.1\r\n\r\n" | nc localhost 8080

# Tabs dans request line
echo -e "GET\t/\tHTTP/1.1\r\n\r\n" | nc localhost 8080
```

**Résultat attendu**: 400 Bad Request (pas de crash)

---

### 1.2 Header Injection & Smuggling

**Objectif**: Injecter des headers malveillants ou faire du HTTP Request Smuggling

```bash
# Header avec caractères nulls
printf "GET / HTTP/1.1\r\nHost: localhost\x00evil\r\n\r\n" | nc localhost 8080

# Headers dupliqués (Content-Length)
echo -e "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\nContent-Length: 10\r\n\r\n" | nc localhost 8080

# Transfer-Encoding + Content-Length (HTTP smuggling classique)
echo -e "POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\nContent-Length: 10\r\n\r\n" | nc localhost 8080

# Folded headers (obsolète en HTTP/1.1 mais certains serveurs l'acceptent)
echo -e "GET / HTTP/1.1\r\nHost: localhost\r\nX-Custom: value1\r\n value2\r\n\r\n" | nc localhost 8080

# Header name avec espaces
echo -e "GET / HTTP/1.1\r\nHost : localhost\r\n\r\n" | nc localhost 8080

# Header sans valeur
echo -e "GET / HTTP/1.1\r\nHost: localhost\r\nX-Empty:\r\n\r\n" | nc localhost 8080

# Header avec CRLF injection
echo -e "GET / HTTP/1.1\r\nHost: localhost\r\nX-Inject: value\r\nInjected: header\r\n\r\n" | nc localhost 8080

# Header name trop long
python3 -c "print('GET / HTTP/1.1\r\n' + 'A'*10000 + ': value\r\n\r\n')" | nc localhost 8080

# Header value trop long
python3 -c "print('GET / HTTP/1.1\r\nHost: localhost\r\nX-Long: ' + 'B'*100000 + '\r\n\r\n')" | nc localhost 8080
```

**Résultat attendu**: 400 Bad Request ou gestion sécurisée

---

## 🎯 **2. ATTAQUES SUR LE BODY**

### 2.1 Content-Length Attacks

**Objectif**: Exploiter le parsing du Content-Length pour des attaques DoS ou buffer overflow

```bash
# Content-Length > client_max_body_size (devrait retourner 413)
curl -X POST http://localhost:8080/ -H "Content-Length: 999999999" -d ""

# Content-Length négatif
echo -e "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: -1\r\n\r\n" | nc localhost 8080

# Content-Length avec caractères invalides
echo -e "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: ABC\r\n\r\n" | nc localhost 8080

# Content-Length avec overflow (> INT_MAX)
echo -e "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 9999999999999999999\r\n\r\n" | nc localhost 8080

# Content-Length présent mais corps absent
echo -e "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 100\r\n\r\n" | nc localhost 8080

# Content-Length plus petit que le corps réel
printf "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length: 5\r\n\r\nHello World" | nc localhost 8080

# Content-Length avec espaces
echo -e "POST / HTTP/1.1\r\nHost: localhost\r\nContent-Length:  10 \r\n\r\nHelloWorld" | nc localhost 8080
```

**Résultat attendu**: 400/413 selon le cas, pas de crash

---

### 2.2 Chunked Encoding Attacks

**Objectif**: Exploiter le parsing du Transfer-Encoding: chunked

```bash
# Chunk size invalide (caractères non-hex)
printf "POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\nZZZ\r\ndata\r\n0\r\n\r\n" | nc localhost 8080

# Chunk sans terminator CRLF
printf "POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello" | nc localhost 8080

# Chunk size trop grand (overflow)
printf "POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\nFFFFFFFFFFFFFFFF\r\n" | nc localhost 8080

# Chunk final manquant (pas de "0\r\n\r\n")
printf "POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n" | nc localhost 8080

# Multiple chunks sans final chunk
printf "POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello\r\n5\r\nworld\r\n" | nc localhost 8080

# Chunk size avec extensions (;param=value)
printf "POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5;ext=val\r\nhello\r\n0\r\n\r\n" | nc localhost 8080

# Chunk data plus long que size annoncée
printf "POST / HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\n\r\n5\r\nhello world\r\n0\r\n\r\n" | nc localhost 8080
```

**Résultat attendu**: 400 Bad Request ou gestion correcte du chunked

---

## 🎯 **3. ATTAQUES CGI**

### 3.1 CGI Timeout & Resource Exhaustion

**Objectif**: Épuiser les ressources serveur ou tester les timeouts

```bash
# Test du timeout CGI (90 secondes)
curl http://localhost:8080/cgi-bin/py/long_timeout.py
# Devrait timeout après 90s et retourner 504

# Lancer plusieurs CGI en parallèle pour épuiser les ressources
for i in {1..50}; do
    curl http://localhost:8080/cgi-bin/py/slow.py &
done
wait

# CGI qui consomme beaucoup de mémoire
cat > /tmp/memory_hog.py << 'EOF'
#!/usr/bin/python3
print("Content-Type: text/plain\r\n\r\n")
big_list = [0] * 100000000  # 100M integers
print("Done")
EOF
chmod +x /tmp/memory_hog.py

# CGI qui fork des processus enfants
cat > /tmp/fork_bomb.py << 'EOF'
#!/usr/bin/python3
import os
print("Content-Type: text/plain\r\n\r\n")
for i in range(100):
    os.fork()
print("Forked")
EOF
chmod +x /tmp/fork_bomb.py

# Test avec SIGKILL pendant l'exécution CGI
curl http://localhost:8080/cgi-bin/py/slow.py &
PID=$!
sleep 1
kill -9 $PID
```

**Résultat attendu**: Timeout à 90s, pas de zombie processes, cleanup correct

---

### 3.2 CGI Environment Injection

**Objectif**: Injecter des variables d'environnement malveillantes

```bash
# Injection via query string avec null byte
curl "http://localhost:8080/cgi-bin/py/contact.py?param=value%00injection"

# Injection via headers personnalisés
curl -H "X-Evil: $(printf '\x00\x0a')" http://localhost:8080/cgi-bin/py/contact.py

# Path traversal dans script CGI
curl "http://localhost:8080/cgi-bin/py/../../etc/passwd"
curl "http://localhost:8080/cgi-bin/py/../php/qrcode.php"

# Query string extrêmement longue
curl "http://localhost:8080/cgi-bin/py/contact.py?$(python3 -c 'print("a=b&"*10000)')"

# POST body géant vers CGI
dd if=/dev/zero bs=1M count=50 | curl -X POST --data-binary @- http://localhost:8080/cgi-bin/py/contact.py

# Caractères spéciaux dans QUERY_STRING
curl "http://localhost:8080/cgi-bin/py/contact.py?name=\$\(ls\)&cmd=\`whoami\`"

# Test SCRIPT_NAME et PATH_INFO
curl "http://localhost:8080/cgi-bin/py/contact.py/extra/path/info"
```

**Résultat attendu**: Pas d'injection, validation correcte, 404 pour path traversal

---

### 3.3 CGI Output Validation

**Objectif**: Tester la validation des outputs CGI

```bash
# CGI qui retourne headers invalides
cat > /tmp/bad_headers.py << 'EOF'
#!/usr/bin/python3
print("Invalid Header Format")
print("Content-Type: text/plain")
print()
print("Body")
EOF
chmod +x /tmp/bad_headers.py

# CGI qui ne retourne pas de Content-Type
cat > /tmp/no_ctype.py << 'EOF'
#!/usr/bin/python3
print()
print("No content type")
EOF
chmod +x /tmp/no_ctype.py

# CGI qui retourne une sortie binaire
cat > /tmp/binary.py << 'EOF'
#!/usr/bin/python3
import sys
sys.stdout.buffer.write(b"Content-Type: application/octet-stream\r\n\r\n")
sys.stdout.buffer.write(bytes(range(256)))
EOF
chmod +x /tmp/binary.py
```

**Résultat attendu**: 500 Internal Server Error pour CGI invalides

---

## 🎯 **4. ATTAQUES PATH TRAVERSAL**

**Objectif**: Accéder à des fichiers en dehors du root directory

```bash
# Path traversal classique
curl http://localhost:8080/../../../etc/passwd
curl http://localhost:8080/../../webserv
curl http://localhost:8080/../Makefile

# URL encoding
curl http://localhost:8080/..%2f..%2f..%2fetc%2fpasswd
curl http://localhost:8080/%2e%2e/%2e%2e/%2e%2e/etc/passwd

# Double encoding
curl http://localhost:8080/%252e%252e/%252e%252e/etc/passwd

# Null byte injection (si non protégé en C++)
curl "http://localhost:8080/../../../etc/passwd%00.html"

# Mixed encoding
curl "http://localhost:8080/..%2F..%2F..%2Fetc/passwd"

# Backslash (Windows-style, mais à tester)
curl "http://localhost:8080/..\\..\\..\\etc\\passwd"

# Absolute path
curl "http://localhost:8080//etc/passwd"

# Path avec ./ et ../
curl "http://localhost:8080/./././../../../etc/passwd"

# Unicode encoding
curl "http://localhost:8080/%c0%ae%c0%ae/%c0%ae%c0%ae/etc/passwd"

# Test sur location /uploads
curl http://localhost:8080/uploads/../../Makefile
curl http://localhost:8080/uploads/../../../etc/passwd
```

**Résultat attendu**: 404 Not Found ou 403 Forbidden (jamais d'accès aux fichiers système)

---

## 🎯 **5. ATTAQUES UPLOAD**

**Objectif**: Exploiter la fonctionnalité d'upload de fichiers

```bash
# Upload de fichier géant (> client_max_body_size = 10M)
dd if=/dev/zero of=/tmp/huge.bin bs=1M count=100
curl -X POST -F "file=@/tmp/huge.bin" http://localhost:8080/uploads

# Upload avec filename malveillant (path traversal)
curl -X POST -F "file=@test.txt;filename=../../etc/passwd" http://localhost:8080/uploads
curl -X POST -F "file=@test.txt;filename=../../../webserv" http://localhost:8080/uploads

# Upload sans boundary dans multipart
curl -X POST -H "Content-Type: multipart/form-data" -d "invalid" http://localhost:8080/uploads

# Upload avec boundary invalide
curl -X POST -H "Content-Type: multipart/form-data; boundary=invalid" -d "data" http://localhost:8080/uploads

# Upload sans Content-Type multipart (raw body)
curl -X POST --data-binary @test.txt http://localhost:8080/uploads

# Upload avec filename vide
curl -X POST -F "file=@test.txt;filename=" http://localhost:8080/uploads

# Upload avec caractères spéciaux dans filename
curl -X POST -F "file=@test.txt;filename=test\$\(whoami\).txt" http://localhost:8080/uploads
curl -X POST -F "file=@test.txt;filename=test;rm -rf /.txt" http://localhost:8080/uploads

# Upload de fichiers multiples
curl -X POST -F "file1=@test1.txt" -F "file2=@test2.txt" http://localhost:8080/uploads

# Upload avec headers malformés
curl -X POST -H "Content-Type: multipart/form-data; boundary=" -d "" http://localhost:8080/uploads

# Test race condition (upload puis delete simultanés)
curl -X POST -F "file=@test.txt" http://localhost:8080/uploads &
curl -X DELETE http://localhost:8080/uploads/test.txt &
wait
```

**Résultat attendu**:
- 413 pour fichiers trop gros
- Nom de fichier sanitized (pas de path traversal)
- Gestion correcte des multipart invalides

---

## 🎯 **6. ATTAQUES DELETE**

**Objectif**: Supprimer des fichiers critiques ou en dehors du répertoire autorisé

```bash
# DELETE sur des fichiers système
curl -X DELETE http://localhost:8080/../../../etc/passwd
curl -X DELETE http://localhost:8080/../../webserv
curl -X DELETE http://localhost:8080/../Makefile

# DELETE sur directory
curl -X DELETE http://localhost:8080/uploads/
curl -X DELETE http://localhost:8080/

# DELETE avec URL encoding
curl -X DELETE http://localhost:8080/uploads/..%2f..%2fMakefile

# DELETE sur fichier inexistant
curl -X DELETE http://localhost:8080/uploads/nonexistent.txt

# DELETE sur location non autorisée (GET POST seulement)
curl -X DELETE http://localhost:8080/index.html

# DELETE simultanés sur le même fichier
echo "test" > uploads/delete_me.txt
curl -X DELETE http://localhost:8080/uploads/delete_me.txt &
curl -X DELETE http://localhost:8080/uploads/delete_me.txt &
curl -X DELETE http://localhost:8080/uploads/delete_me.txt &
wait

# DELETE avec body (devrait être ignoré)
curl -X DELETE -d "body" http://localhost:8080/uploads/test.txt
```

**Résultat attendu**:
- 404 pour fichiers inexistants
- 403 ou 404 pour path traversal
- 405 pour locations non autorisées

---

## 🎯 **7. RACE CONDITIONS & CONCURRENCY**

**Objectif**: Tester la gestion de la concurrence et trouver des race conditions

```bash
# Stress test avec connexions simultanées
siege -c 255 -r 100 http://localhost:8080/

# Attaque Slowloris (ouvrir connexions sans envoyer de données)
for i in {1..1000}; do
    (echo -n "GET / HTTP/1.1\r\n" | nc localhost 8080 2>/dev/null) &
done
sleep 60
pkill nc

# Test de file descriptor exhaustion
ulimit -n 256  # Limiter les FDs disponibles
for i in {1..500}; do
    nc localhost 8080 &
done
wait
ulimit -n 1024  # Restaurer

# Requêtes partielles simultanées
for i in {1..100}; do
    (sleep 0.1 && echo -n "GET / HTTP/1.1\r\nHost: " && sleep 5 && echo "localhost\r\n\r\n") | nc localhost 8080 &
done

# Upload et GET simultanés sur le même fichier
while true; do
    curl -X POST -F "file=@test.txt" http://localhost:8080/uploads &
    curl http://localhost:8080/uploads/test.txt &
    sleep 0.01
done

# Multiples CGI simultanés
for i in {1..50}; do
    curl http://localhost:8080/cgi-bin/py/slow.py &
done
wait

# Test poll() avec événements multiples
ab -n 10000 -c 200 http://localhost:8080/

# Test de client timeout
(echo -n "GET / HTTP/1.1\r\nHost: localhost\r\n" && sleep 120 && echo "\r\n") | nc localhost 8080
```

**Résultat attendu**:
- Pas de crash
- Gestion correcte des timeouts
- Pas de deadlock
- Cleanup correct des connexions

---

## 🎯 **8. ATTAQUES CONFIGURATION**

### 8.1 Port déjà utilisé

**Objectif**: Tester le comportement quand le port est déjà pris

```bash
# Lancer un autre serveur sur le même port
python3 -m http.server 8080 &
PYTHON_PID=$!

# Tenter de lancer webserv (devrait échouer proprement)
./webserv config/default.conf

# Cleanup
kill $PYTHON_PID
```

**Résultat attendu**: Message d'erreur clair, exit propre (pas de crash)

---

### 8.2 Configuration invalide

**Objectif**: Tester le parser de configuration

```bash
# Fichier de config inexistant
./webserv /non/existent/file.conf

# Config vide
echo "" > /tmp/empty.conf
./webserv /tmp/empty.conf

# Config avec syntax errors
echo "server { listen; }" > /tmp/bad.conf
./webserv /tmp/bad.conf

# Config sans server block
echo "# Just a comment" > /tmp/no_server.conf
./webserv /tmp/no_server.conf

# Config avec accolades non matchées
echo "server { listen 8080;" > /tmp/unmatched.conf
./webserv /tmp/unmatched.conf

# Config avec directives invalides
echo "server { invalid_directive value; }" > /tmp/invalid.conf
./webserv /tmp/invalid.conf

# Config avec port invalide
echo "server { listen 99999; }" > /tmp/bad_port.conf
./webserv /tmp/bad_port.conf

# Config avec client_max_body_size négatif
echo "server { client_max_body_size -10M; }" > /tmp/negative.conf
./webserv /tmp/negative.conf

# Config avec paths inexistants
echo "server { location / { root /nonexistent; } }" > /tmp/bad_path.conf
./webserv /tmp/bad_path.conf

# Permissions insuffisantes
echo "server { listen 80; }" > /tmp/privileged.conf
./webserv /tmp/privileged.conf  # Sans sudo, devrait échouer
```

**Résultat attendu**: Messages d'erreur explicites, pas de crash

---

## 🎯 **9. MEMORY LEAKS & CRASHES**

**Objectif**: Détecter les fuites mémoire et comportements indéfinis

```bash
# Valgrind avec requêtes variées
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
         --log-file=valgrind.log ./webserv config/default.conf &
WEBSERV_PID=$!
sleep 2

# Séquence de tests
curl http://localhost:8080/
curl -X POST -d "data=test" http://localhost:8080/
curl http://localhost:8080/cgi-bin/py/contact.py
curl -X POST -F "file=@test.txt" http://localhost:8080/uploads
curl http://localhost:8080/nonexistent
curl -X DELETE http://localhost:8080/uploads/test.txt

# Arrêt propre
kill -TERM $WEBSERV_PID
wait
cat valgrind.log

# Test avec AddressSanitizer
make re CXXFLAGS="-fsanitize=address -g"
./webserv config/default.conf &
sleep 2
curl http://localhost:8080/
pkill webserv

# Test de SIGINT pendant une requête
./webserv config/default.conf &
WEBSERV_PID=$!
curl http://localhost:8080/cgi-bin/py/slow.py &
sleep 0.5
kill -INT $WEBSERV_PID
wait

# Test de multiples SIGTERM
./webserv config/default.conf &
WEBSERV_PID=$!
kill -TERM $WEBSERV_PID
kill -TERM $WEBSERV_PID
kill -TERM $WEBSERV_PID
wait

# Test avec requête incomplète puis SIGTERM
./webserv config/default.conf &
WEBSERV_PID=$!
(echo -n "GET / HTTP/1.1\r\n" && sleep 2) | nc localhost 8080 &
sleep 0.5
kill -TERM $WEBSERV_PID
wait

# Vérifier qu'il n'y a pas de processus zombie
ps aux | grep webserv
ps aux | grep defunct
```

**Résultat attendu**:
- 0 leaks avec Valgrind
- Pas d'erreurs AddressSanitizer
- Cleanup propre des CGI (pas de zombies)
- Signal handlers fonctionnels

---

## 🎯 **10. TESTS DE CONFORMITÉ HTTP/1.1**

**Objectif**: Vérifier la conformité avec RFC 2616/7230

```bash
# HEAD ne doit pas retourner de body
RESPONSE=$(curl -i -X HEAD http://localhost:8080/)
echo "$RESPONSE" | grep -A 100 "^$"  # Ne devrait rien afficher après headers

# OPTIONS doit retourner Allow header
curl -i -X OPTIONS http://localhost:8080/ | grep "Allow:"

# 405 si méthode non autorisée
curl -i -X PUT http://localhost:8080/ | grep "405"
curl -i -X PATCH http://localhost:8080/ | grep "405"

# Host header manquant en HTTP/1.1 (devrait retourner 400)
echo -e "GET / HTTP/1.1\r\n\r\n" | nc localhost 8080

# Host header présent en HTTP/1.1 (devrait marcher)
echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 8080

# Test case-insensitivity des headers
echo -e "GET / HTTP/1.1\r\nhOsT: localhost\r\n\r\n" | nc localhost 8080
echo -e "GET / HTTP/1.1\r\nHOST: localhost\r\n\r\n" | nc localhost 8080

# Test des status codes
curl -i http://localhost:8080/ | grep "200 OK"
curl -i http://localhost:8080/nonexistent | grep "404"
curl -i -X POST -d "$(dd if=/dev/zero bs=1M count=20 2>/dev/null)" http://localhost:8080/ | grep "413"
curl -i -X PUT http://localhost:8080/ | grep "405"
curl -i http://localhost:8080/cgi-bin/py/error500.py | grep "500"

# Test Connection: close
curl -i -H "Connection: close" http://localhost:8080/

# Test des redirections (si implémentées)
curl -i -L http://localhost:8080/redirect

# Test Accept-Encoding (gzip)
curl -i -H "Accept-Encoding: gzip" http://localhost:8080/

# Test User-Agent vide
curl -i -H "User-Agent:" http://localhost:8080/

# Test Referer avec caractères spéciaux
curl -i -H "Referer: http://example.com/<script>" http://localhost:8080/
```

**Résultat attendu**: Conformité RFC, status codes corrects

---

## 🎯 **11. CHECKS NORME & COMPILATION**

**Objectif**: Vérifier la qualité du code et conformité C++98

```bash
# Compilation avec tous les warnings
make fclean
make re CXXFLAGS="-Wall -Wextra -Werror -std=c++98 -pedantic"

# Vérifier qu'il n'y a pas de C++11/14/17 features
echo "=== Checking for C++11+ features ==="
grep -rn "auto " srcs/ --include="*.cpp" --include="*.hpp" | grep -v "// "
grep -rn "nullptr" srcs/ --include="*.cpp" --include="*.hpp"
grep -rn "std::unique_ptr" srcs/ --include="*.cpp" --include="*.hpp"
grep -rn "std::shared_ptr" srcs/ --include="*.cpp" --include="*.hpp"
grep -rn "std::move" srcs/ --include="*.cpp" --include="*.hpp"
grep -rn "override" srcs/ --include="*.cpp" --include="*.hpp"
grep -rn "final" srcs/ --include="*.cpp" --include="*.hpp"
grep -rn "constexpr" srcs/ --include="*.cpp" --include="*.hpp"
grep -rn "decltype" srcs/ --include="*.cpp" --include="*.hpp"
grep -rn "lambda" srcs/ --include="*.cpp" --include="*.hpp"
grep -rn "\\[\\](" srcs/ --include="*.cpp" --include="*.hpp"  # Lambda syntax

# Vérifier les includes interdits
echo "=== Checking for forbidden includes ==="
grep -rn "#include <thread>" srcs/
grep -rn "#include <mutex>" srcs/
grep -rn "#include <atomic>" srcs/
grep -rn "#include <future>" srcs/

# Vérifier les fonctions interdites
echo "=== Checking for forbidden functions ==="
grep -rn "system(" srcs/ --include="*.cpp"
grep -rn "popen(" srcs/ --include="*.cpp"
grep -rn "exec[vl]p\?(" srcs/ --include="*.cpp" | grep -v "execve"  # execve autorisé pour CGI

# Test de re-linking
echo "=== Checking re-linking ==="
make
touch srcs/main.cpp
make | grep -q "Nothing to be done" && echo "FAIL: Relink detected" || echo "OK: No relink"

# Check norminette (si applicable au projet)
# norminette srcs/

# Check des memory leaks sur tests complets
echo "=== Memory leak comprehensive test ==="
valgrind --leak-check=full --show-leak-kinds=all \
         --suppressions=/usr/share/valgrind/default.supp \
         --log-file=valgrind_full.log \
         ./webserv config/default.conf &
WEBSERV_PID=$!
sleep 2

# Suite de tests complète
for i in {1..100}; do
    curl -s http://localhost:8080/ > /dev/null
    curl -s -X POST -d "data" http://localhost:8080/ > /dev/null
    curl -s http://localhost:8080/cgi-bin/py/contact.py > /dev/null
done

kill -TERM $WEBSERV_PID
wait

echo "=== Valgrind Results ==="
grep "definitely lost" valgrind_full.log
grep "indirectly lost" valgrind_full.log
grep "possibly lost" valgrind_full.log
```

**Résultat attendu**:
- Compilation sans warnings
- Pas de features C++11+
- Pas de re-linking
- 0 leaks

---

## 🎯 **12. BONUS ATTACKS (Virtual Hosts)**

**Objectif**: Tester la fonctionnalité de virtual hosts (server_name)

```bash
# Test avec server_name correct
curl -H "Host: webserv" http://localhost:8080/

# Test avec server_name incorrect
curl -H "Host: wrong.server" http://localhost:8080/

# Test sans Host header (devrait utiliser le premier server block)
curl -H "Host:" http://localhost:8080/

# Test avec Host header vide
printf "GET / HTTP/1.1\r\nHost: \r\n\r\n" | nc localhost 8080

# Test avec Host header contenant port
curl -H "Host: webserv:8080" http://localhost:8080/

# Test avec Host header contenant IP
curl -H "Host: 127.0.0.1" http://localhost:8080/

# Test avec Host header contenant caractères invalides
curl -H "Host: web/../serv" http://localhost:8080/
curl -H "Host: web\x00serv" http://localhost:8080/

# Si plusieurs server blocks sur le même port
# Vérifier que le bon server_name est utilisé
curl -H "Host: server1.com" http://localhost:8080/
curl -H "Host: server2.com" http://localhost:8080/
```

**Résultat attendu**: Sélection correcte du server block basé sur Host header

---

## 🎯 **13. TESTS EDGE CASES SUPPLÉMENTAIRES**

```bash
# Requête avec body sur GET (devrait être ignoré)
curl -X GET -d "body" http://localhost:8080/

# Requête OPTIONS avec body
curl -X OPTIONS -d "body" http://localhost:8080/

# Requête HEAD avec body
curl -X HEAD -d "body" http://localhost:8080/

# POST sans Content-Type
curl -X POST -d "data" http://localhost:8080/

# URI avec fragment (#)
curl "http://localhost:8080/index.html#section"

# URI avec multiples query params
curl "http://localhost:8080/?a=1&b=2&c=3&d=4&e=5"

# Query string sans valeur
curl "http://localhost:8080/?key1&key2&key3"

# Query string avec caractères spéciaux
curl "http://localhost:8080/?data=<script>alert('xss')</script>"

# Test avec IP au lieu de hostname
curl http://127.0.0.1:8080/
curl http://0.0.0.0:8080/
curl http://localhost:8080/

# Connexion puis déconnexion immédiate (sans requête)
for i in {1..100}; do
    timeout 0.1 nc localhost 8080 &
done
wait

# Test avec contenu binaire dans body
dd if=/dev/urandom bs=1K count=10 | curl -X POST --data-binary @- http://localhost:8080/

# Test autoindex
curl http://localhost:8080/uploads/
curl http://localhost:8080/  # devrait retourner index.html, pas autoindex

# Test avec fichier sans extension
touch uploads/noextension
curl http://localhost:8080/uploads/noextension

# Test avec fichier caché (.hidden)
touch uploads/.hidden
curl http://localhost:8080/uploads/.hidden

# Test avec nom de fichier très long
LONGNAME=$(python3 -c "print('a'*255)")
touch "uploads/$LONGNAME.txt" 2>/dev/null
curl "http://localhost:8080/uploads/$LONGNAME.txt"

# Test cookies multiples
curl -b "session=abc123; user=john; token=xyz" http://localhost:8080/
```

---

## ✅ **DÉFENSE ET PROTECTION**

### Checklist de sécurité

Pour chaque attaque listée ci-dessus, vérifiez que votre code :

#### 1. **Validation des Inputs**
- [ ] Validation de la request line (méthode, URI, version HTTP)
- [ ] Validation de tous les headers (noms et valeurs)
- [ ] Validation du Content-Length (range, overflow)
- [ ] Validation du Transfer-Encoding: chunked
- [ ] Rejection des caractères nuls, CRLF injections
- [ ] Validation de la taille maximale de la requête

#### 2. **Gestion des Erreurs**
- [ ] Retourne les bons codes HTTP (400, 404, 405, 413, 500, 504)
- [ ] Pas de crash sur requêtes malformées
- [ ] Pas d'information leakage dans les erreurs
- [ ] Cleanup propre même en cas d'erreur

#### 3. **Limites et Timeouts**
- [ ] client_max_body_size respecté (10M par défaut)
- [ ] Timeout CGI à 90 secondes
- [ ] Timeout client pour connexions idle
- [ ] MAX_REQUEST_SIZE pour éviter memory exhaustion
- [ ] MAX_URI_LENGTH pour limiter la taille des URIs

#### 4. **Sécurité des Fichiers**
- [ ] Path canonicalization (résolution de .. et .)
- [ ] Vérification que le chemin reste dans root directory
- [ ] Sanitization des noms de fichiers uploadés
- [ ] Vérification des permissions avant DELETE

#### 5. **CGI Security**
- [ ] Variables d'environnement correctement échappées
- [ ] Timeout et cleanup des processus CGI
- [ ] Validation des scripts CGI (extension, path)
- [ ] Gestion des CGI qui crashent ou timeout
- [ ] Pas de zombie processes

#### 6. **Concurrence et Performance**
- [ ] poll() correctement implémenté
- [ ] Gestion des sockets non-bloquants
- [ ] Pas de race conditions
- [ ] Cleanup correct des connexions
- [ ] Gestion des file descriptors (pas d'exhaustion)

#### 7. **Memory Management**
- [ ] Pas de memory leaks (testé avec Valgrind)
- [ ] Pas de buffer overflows
- [ ] Pas de use-after-free
- [ ] Delete des objets CGIProcess en cas d'erreur

#### 8. **Signal Handling**
- [ ] SIGINT proprement géré (arrêt propre)
- [ ] SIGTERM proprement géré
- [ ] SIGPIPE ignoré ou géré
- [ ] Self-pipe pour signal handling dans poll()

#### 9. **Conformité HTTP/1.1**
- [ ] HEAD ne retourne pas de body
- [ ] OPTIONS retourne Allow header
- [ ] Host header obligatoire en HTTP/1.1
- [ ] Case-insensitive headers
- [ ] Status codes conformes RFC

#### 10. **Code Quality**
- [ ] C++98 strict (pas de features C++11+)
- [ ] Compilation avec -Wall -Wextra -Werror
- [ ] Pas de re-linking
- [ ] Code propre et lisible
- [ ] Gestion d'erreur cohérente

---

## 🧪 **Script de Test Automatique**

Voici un script bash pour automatiser tous les tests :

```bash
#!/bin/bash

echo "=== WebServ Security Test Suite ==="
echo ""

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

PASSED=0
FAILED=0

# Function to run test
run_test() {
    local name=$1
    local cmd=$2
    local expected=$3

    echo -n "Testing: $name... "
    result=$(eval $cmd 2>&1)

    if echo "$result" | grep -q "$expected"; then
        echo -e "${GREEN}PASS${NC}"
        ((PASSED++))
    else
        echo -e "${RED}FAIL${NC}"
        ((FAILED++))
        echo "  Expected: $expected"
        echo "  Got: $result"
    fi
}

# Start server
echo "Starting server..."
./webserv config/default.conf &
WEBSERV_PID=$!
sleep 2

echo ""
echo "=== Running Tests ==="
echo ""

# Test 1: Invalid method
run_test "Invalid HTTP method" \
    "echo -e 'INVALID / HTTP/1.1\r\n\r\n' | nc localhost 8080 | head -1" \
    "400\|501"

# Test 2: Path traversal
run_test "Path traversal prevention" \
    "curl -s -o /dev/null -w '%{http_code}' http://localhost:8080/../../../etc/passwd" \
    "404\|403"

# Test 3: File too large
run_test "File size limit (413)" \
    "curl -s -o /dev/null -w '%{http_code}' -X POST -H 'Content-Length: 999999999' http://localhost:8080/" \
    "413"

# Test 4: Method not allowed
run_test "Method not allowed (405)" \
    "curl -s -o /dev/null -w '%{http_code}' -X PUT http://localhost:8080/" \
    "405"

# Test 5: CGI timeout
run_test "CGI timeout (504)" \
    "timeout 95 curl -s -o /dev/null -w '%{http_code}' http://localhost:8080/cgi-bin/py/timeout.py" \
    "504"

# Test 6: HEAD request has no body
run_test "HEAD has no body" \
    "curl -s -I http://localhost:8080/ | grep -c '^$'" \
    "1"

# Test 7: OPTIONS returns Allow header
run_test "OPTIONS returns Allow" \
    "curl -s -i -X OPTIONS http://localhost:8080/ | grep -c 'Allow:'" \
    "1"

# Test 8: Missing Host header in HTTP/1.1
run_test "Missing Host header" \
    "echo -e 'GET / HTTP/1.1\r\n\r\n' | nc localhost 8080 | head -1" \
    "400"

# Add more tests...

echo ""
echo "=== Test Summary ==="
echo -e "Passed: ${GREEN}$PASSED${NC}"
echo -e "Failed: ${RED}$FAILED${NC}"
echo ""

# Cleanup
kill $WEBSERV_PID
wait $WEBSERV_PID 2>/dev/null

exit $FAILED
```

---

## 📚 **Ressources et Références**

- [RFC 7230 - HTTP/1.1: Message Syntax and Routing](https://tools.ietf.org/html/rfc7230)
- [RFC 7231 - HTTP/1.1: Semantics and Content](https://tools.ietf.org/html/rfc7231)
- [RFC 3875 - The Common Gateway Interface (CGI) Version 1.1](https://tools.ietf.org/html/rfc3875)
- [OWASP Testing Guide](https://owasp.org/www-project-web-security-testing-guide/)
- [HTTP Request Smuggling](https://portswigger.net/web-security/request-smuggling)

---

## 🎓 **Conclusion**

Ce document liste les principales attaques qu'un évaluateur pourrait tenter pour casser votre serveur web. Utilisez-le comme checklist pour :

1. **Tester votre implémentation** avant l'évaluation
2. **Identifier les faiblesses** et les corriger
3. **Comprendre les edge cases** et les gérer correctement
4. **Préparer votre défense** en anticipant les questions

Bonne chance pour votre évaluation ! 🚀

---

*Document créé le 12 février 2026*
*Pour le projet webserv de 42*
