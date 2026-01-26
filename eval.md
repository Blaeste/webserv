# WebServ - Evaluation Quick Guide

## Partie 1: Mandatory Part

### 0. Installation & Questions Théoriques

**Installation de Siege:**
```bash
# macOS
brew install siege

# Linux (Debian/Ubuntu)
sudo apt-get install siege

# Vérifier
siege --version
```

**Questions à poser au groupe:**
- [ ] Expliquer les bases d'un serveur HTTP
- [ ] Quelle fonction avez-vous utilisée pour I/O Multiplexing ? (poll, select, epoll, kqueue)
- [ ] Comment fonctionne poll() / select() ?
- [ ] Utilisez-vous un seul poll() / select() ?
- [ ] Comment gérez-vous accept() du serveur ET read/write des clients ?

---

### 1. Check the code
- [ ] poll() dans la boucle principale → `grep -n "poll(" srcs/server/Server.cpp`
- [ ] poll() vérifie READ et WRITE simultanément → `grep "POLLIN\|POLLOUT" srcs/server/Server.cpp`
- [ ] Un seul read/write par client → Vérifier `handleClientRead()` et `handleClientWrite()`
- [ ] Erreurs read/recv/write/send vérifiées → `grep -A5 "recv\|send" srcs/server/Client.cpp`
- [ ] Client supprimé en cas d'erreur → `grep "removeClient" srcs/server/Server.cpp`
- [ ] Pas de vérification errno après I/O → `grep "errno" srcs/server/Server.cpp srcs/server/Client.cpp`
- [ ] Tous les I/O passent par poll() → Vérifier tous les `_pollFds.push_back()`
- [ ] Compilation sans re-link → `make && make`

---

## Partie 2: Configuration

### 1. HTTP Status Codes
```bash
curl -v http://localhost:8080/                    # 200 OK
curl -v http://localhost:8080/notfound            # 404 Not Found
curl -v -X DELETE http://localhost:8080/          # 405 Method Not Allowed
```

### 2. Multiple Ports
```bash
curl http://localhost:8080/
curl http://localhost:8081/
netstat -tulpn | grep webserv
```

### 3. Virtual Hosts (Different Hostnames)
```bash
curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/
curl -H "Host: example.com" http://localhost:8080/
```

### 4. Default Error Pages
```bash
curl http://localhost:8080/notfound.html  # Voir page 404 personnalisée
cat www/error_pages/404.html
```

### 5. Limit Client Body
```bash
# Petit body (OK)
curl -X POST -d "small data" http://localhost:8080/upload

# Grand body (413 Payload Too Large)
curl -X POST --data "$(python3 -c 'print("A"*15000000)')" http://localhost:8080/upload
```

### 6. Routes to Different Directories
```bash
curl http://localhost:8080/              # www/
curl http://localhost:8080/uploads/      # uploads/
curl http://localhost:8080/images/       # www/image/
```

### 7. Default Index Files
```bash
curl http://localhost:8080/              # Retourne index.html
curl http://localhost:8080/uploads/      # Liste ou index
```

### 8. Allowed Methods per Route
```bash
# GET autorisé partout
curl -X GET http://localhost:8080/

# POST sur / (autorisé)
curl -X POST -d "test" http://localhost:8080/

# DELETE sur / (405 Method Not Allowed)
curl -v -X DELETE http://localhost:8080/

# DELETE sur /uploads (autorisé)
echo "test" > uploads/test.txt
curl -X DELETE http://localhost:8080/uploads/test.txt
```

---

## Partie 3: Basic Checks

### 1. GET Request
```bash
# Avec curl
curl http://localhost:8080/
curl http://localhost:8080/about.html
curl -v http://localhost:8080/index.html  # Vérifier status 200

# Avec telnet
telnet localhost 8080
GET / HTTP/1.1
Host: localhost

# (Appuyer sur Enter 2 fois)
```

### 2. POST Request
```bash
# Avec curl
curl -X POST -d "name=John&email=test@example.com" http://localhost:8080/
curl -v -X POST -d "test data" http://localhost:8080/upload

# Avec telnet
telnet localhost 8080
POST /upload HTTP/1.1
Host: localhost
Content-Type: application/x-www-form-urlencoded
Content-Length: 9

test=data
```

### 3. DELETE Request
```bash
# Créer un fichier d'abord
echo "test content" > uploads/test_delete.txt

# DELETE avec curl
curl -v -X DELETE http://localhost:8080/uploads/test_delete.txt

# Vérifier que le fichier est supprimé
ls uploads/test_delete.txt  # Should not exist

# Avec telnet
telnet localhost 8080
DELETE /uploads/test_delete.txt HTTP/1.1
Host: localhost

```

### 4. UNKNOWN Request (should NOT crash)
```bash
# Requête avec méthode inconnue
curl -v -X INVALID http://localhost:8080/

# Avec telnet
telnet localhost 8080
FOOBAR / HTTP/1.1
Host: localhost


# Le serveur doit répondre 501 Not Implemented
# ET ne doit PAS crasher !

# Vérifier que le serveur tourne toujours
ps aux | grep webserv
curl http://localhost:8080/  # Doit encore fonctionner
```

### 5. Appropriate Status Codes
```bash
# 200 OK
curl -w "\nStatus: %{http_code}\n" http://localhost:8080/

# 404 Not Found
curl -w "\nStatus: %{http_code}\n" http://localhost:8080/notfound.html

# 405 Method Not Allowed
curl -w "\nStatus: %{http_code}\n" -X DELETE http://localhost:8080/

# 413 Payload Too Large
curl -w "\nStatus: %{http_code}\n" -X POST --data "$(python3 -c 'print("A"*15000000)')" http://localhost:8080/upload

# 501 Not Implemented
curl -w "\nStatus: %{http_code}\n" -X FOOBAR http://localhost:8080/
```

### 6. Upload File and Get It Back
```bash
# Créer un fichier test
echo "Hello from uploaded file!" > test_upload.txt

# Upload le fichier
curl -X POST -F "file=@test_upload.txt" http://localhost:8080/upload

# Vérifier qu'il est dans uploads/
ls -la uploads/test_upload.txt

# Récupérer le fichier
curl http://localhost:8080/uploads/test_upload.txt

# Ou avec wget
wget http://localhost:8080/uploads/test_upload.txt -O downloaded.txt

# Comparer les fichiers
diff test_upload.txt downloaded.txt  # Should be identical

# Tester avec un fichier binaire (image)
curl -X POST -F "file=@image.png" http://localhost:8080/upload
curl http://localhost:8080/uploads/image.png --output downloaded.png
```

---

## Partie 4: Check CGI

### 1. CGI Works Properly
```bash
# Python CGI - GET
curl http://localhost:8080/cgi-bin/py/contact.py

# Python CGI - POST
curl -X POST -d "name=John&email=john@test.com&message=Hello" \
  http://localhost:8080/cgi-bin/py/contact.py

# PHP CGI - GET
curl http://localhost:8080/cgi-bin/php/qrcode.php

# Vérifier la réponse HTTP
curl -v http://localhost:8080/cgi-bin/py/contact.py
```

### 2. CGI Running in Correct Directory
```bash
# Créer un CGI qui affiche son répertoire de travail
cat > cgi-bin/py/test_pwd.py << 'EOF'
#!/usr/bin/env python3
import os
print("Content-Type: text/plain\r")
print("\r")
print(f"Current Directory: {os.getcwd()}")
print(f"Script Location: {os.path.abspath(__file__)}")
EOF

chmod +x cgi-bin/py/test_pwd.py

# Tester
curl http://localhost:8080/cgi-bin/py/test_pwd.py

# Doit afficher le répertoire cgi-bin/py ou équivalent
```

### 3. CGI with GET Method
```bash
# Simple GET
curl http://localhost:8080/cgi-bin/py/contact.py

# GET avec query string
curl "http://localhost:8080/cgi-bin/py/contact.py?name=Alice&age=25"

# Vérifier que QUERY_STRING est passé
cat > cgi-bin/py/test_get.py << 'EOF'
#!/usr/bin/env python3
import os
print("Content-Type: text/plain\r")
print("\r")
print(f"QUERY_STRING: {os.environ.get('QUERY_STRING', 'None')}")
print(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD', 'None')}")
EOF

chmod +x cgi-bin/py/test_get.py
curl "http://localhost:8080/cgi-bin/py/test_get.py?test=123&foo=bar"
```

### 4. CGI with POST Method
```bash
# POST avec données
curl -X POST -d "username=John&password=secret" \
  http://localhost:8080/cgi-bin/py/contact.py

# POST avec Content-Type
curl -X POST \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -d "field1=value1&field2=value2" \
  http://localhost:8080/cgi-bin/py/contact.py

# Vérifier que le body est passé au CGI
cat > cgi-bin/py/test_post.py << 'EOF'
#!/usr/bin/env python3
import sys
import os
print("Content-Type: text/plain\r")
print("\r")
print(f"REQUEST_METHOD: {os.environ.get('REQUEST_METHOD')}")
print(f"CONTENT_LENGTH: {os.environ.get('CONTENT_LENGTH')}")
print(f"Body received: {sys.stdin.read()}")
EOF

chmod +x cgi-bin/py/test_post.py
curl -X POST -d "test=data" http://localhost:8080/cgi-bin/py/test_post.py
```

### 5. CGI with Errors - Script with Syntax Error
```bash
# Créer un script Python avec erreur de syntaxe
cat > cgi-bin/py/error_syntax.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/plain\r")
print("\r")
print("Before error")
this is invalid python syntax!!!
print("After error")
EOF

chmod +x cgi-bin/py/error_syntax.py

# Tester
curl -v http://localhost:8080/cgi-bin/py/error_syntax.py

# Le serveur doit:
# - Retourner 500 Internal Server Error
# - NE PAS crasher
# - Continuer à fonctionner

# Vérifier que le serveur tourne toujours
curl http://localhost:8080/
```

### 6. CGI with Errors - Infinite Loop
```bash
# Créer un script avec boucle infinie
cat > cgi-bin/py/infinite_loop.py << 'EOF'
#!/usr/bin/env python3
import time
print("Content-Type: text/plain\r")
print("\r")
print("Starting infinite loop...")
while True:
    time.sleep(1)
EOF

chmod +x cgi-bin/py/infinite_loop.py

# Tester (le serveur doit timeout après 5 secondes)
time curl http://localhost:8080/cgi-bin/py/infinite_loop.py

# Doit retourner 504 Gateway Timeout après ~5 secondes
# Le serveur NE DOIT PAS crasher

# Vérifier que le serveur fonctionne toujours
curl http://localhost:8080/

# Vérifier qu'il n'y a pas de processus zombie
ps aux | grep python | grep infinite_loop  # Devrait être vide
```

### 7. CGI with Errors - Runtime Error
```bash
# Créer un script qui crash au runtime
cat > cgi-bin/py/error_runtime.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/plain\r")
print("\r")
print("Before crash")
x = 1 / 0  # Division by zero
print("After crash")
EOF

chmod +x cgi-bin/py/error_runtime.py

# Tester
curl -v http://localhost:8080/cgi-bin/py/error_runtime.py

# Doit retourner 500 Internal Server Error
# Le serveur NE DOIT PAS crasher
curl http://localhost:8080/
```

### 8. CGI with Errors - Missing Shebang
```bash
# Créer un script sans shebang
cat > cgi-bin/py/no_shebang.py << 'EOF'
print("Content-Type: text/plain\r")
print("\r")
print("This script has no shebang")
EOF

chmod +x cgi-bin/py/no_shebang.py

# Tester
curl -v http://localhost:8080/cgi-bin/py/no_shebang.py

# Peut retourner 500 ou fonctionner selon l'implémentation
# Le serveur NE DOIT PAS crasher
```

### 9. CGI with Errors - Permission Denied
```bash
# Créer un script sans permission d'exécution
cat > cgi-bin/py/no_exec.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/plain\r")
print("\r")
print("This should not work")
EOF

chmod 644 cgi-bin/py/no_exec.py  # Pas de permission +x

# Tester
curl -v http://localhost:8080/cgi-bin/py/no_exec.py

# Doit retourner 500 Internal Server Error
# Le serveur NE DOIT PAS crasher
```

### 10. Verify Server Never Crashes
```bash
# Après tous les tests, vérifier que le serveur fonctionne
ps aux | grep webserv
curl http://localhost:8080/

# Tester une requête normale
curl http://localhost:8080/about.html

# Le serveur doit toujours répondre correctement
```

### 11. Check Error Visibility
```bash
# Dans le terminal où tourne le serveur, vérifier les logs
# Les erreurs CGI doivent être visibles:
# - "[CGI] Timeout: killing process XXX"
# - "[CGI] Failed to execute"
# - etc.

# Relancer le serveur et observer les logs
./webserv config/default.conf

# Dans un autre terminal, déclencher une erreur
curl http://localhost:8080/cgi-bin/py/error_syntax.py

# Les logs doivent montrer l'erreur clairement
```

---

## Partie 5: Check with a Browser

### 1. Open Browser and DevTools
```bash
# Lancer le serveur
./webserv config/default.conf

# Ouvrir le navigateur de référence
firefox http://localhost:8080/
# ou
google-chrome http://localhost:8080/
```

**Dans le navigateur:**
- Appuyer sur `F12` pour ouvrir les DevTools
- Aller dans l'onglet **Network** (Réseau)
- Recharger la page `Ctrl+R`

### 2. Check Request and Response Headers

**Headers à vérifier dans la requête:**
```
GET / HTTP/1.1
Host: localhost:8080
User-Agent: Mozilla/5.0 ...
Accept: text/html,application/xhtml+xml...
Accept-Language: en-US,en;q=0.9
Accept-Encoding: gzip, deflate
Connection: keep-alive
```

**Headers à vérifier dans la réponse:**
```
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 1234
Date: ...
Server: webserv/1.0
Connection: close
```

**Points à vérifier:**
- [ ] Status code correct (200 OK)
- [ ] Content-Type approprié (text/html pour HTML)
- [ ] Content-Length présent et correct
- [ ] Pas d'erreurs dans la console

### 3. Serve Fully Static Website

**Test de navigation:**
```
http://localhost:8080/              → index.html
http://localhost:8080/about.html    → About page
http://localhost:8080/contact.html  → Contact page
```

**Dans le navigateur, vérifier:**
- [ ] La page HTML s'affiche correctement
- [ ] Les images se chargent (`www/image/`)
- [ ] Les CSS sont appliqués (`www/css/style.css`)
- [ ] Les JS fonctionnent (`www/js/`)
- [ ] Navigation entre pages fonctionne
- [ ] Pas d'erreurs 404 dans les ressources

**Dans DevTools → Network:**
- [ ] Toutes les requêtes sont en status 200
- [ ] Content-Type corrects:
  - `.html` → `text/html`
  - `.css` → `text/css`
  - `.js` → `application/javascript`
  - `.png` → `image/png`
  - `.jpg` → `image/jpeg`

### 4. Try Wrong URL (404 Error)
```
http://localhost:8080/page_inexistante.html
http://localhost:8080/wrong/path/file.html
http://localhost:8080/notfound
```

**Vérifier:**
- [ ] Status 404 Not Found dans Network tab
- [ ] Page d'erreur 404 personnalisée s'affiche
- [ ] Le serveur ne crash pas
- [ ] La navigation continue de fonctionner après

### 5. Try to List a Directory (Autoindex)
```
http://localhost:8080/uploads/
http://localhost:8080/images/
```

**Si autoindex est ON:**
- [ ] Liste des fichiers s'affiche en HTML
- [ ] Les liens vers les fichiers fonctionnent
- [ ] Cliquer sur un fichier le télécharge/affiche

**Si autoindex est OFF:**
- [ ] Status 403 Forbidden
- [ ] Page d'erreur 403 s'affiche

### 6. Try Redirected URL
```bash
# Configurer une redirection dans default.conf
# location /old-page {
#     return 301 /new-page.html;
# }

# Tester dans le navigateur
http://localhost:8080/old-page
```

**Vérifier dans DevTools:**
- [ ] Status 301 Moved Permanently ou 302 Found
- [ ] Header `Location: /new-page.html`
- [ ] Le navigateur suit la redirection automatiquement
- [ ] URL finale dans la barre d'adresse est correcte

### 7. Additional Browser Tests

**Test 1: Multiple Tabs**
```
# Ouvrir plusieurs onglets simultanément
Tab 1: http://localhost:8080/
Tab 2: http://localhost:8080/about.html
Tab 3: http://localhost:8080/contact.html
```
- [ ] Toutes les pages se chargent correctement
- [ ] Pas de ralentissement
- [ ] Pas de crash serveur

**Test 2: Refresh Rapides**
```
# Sur une page, appuyer plusieurs fois sur F5 rapidement
```
- [ ] Toutes les requêtes reçoivent une réponse
- [ ] Pas de timeout
- [ ] Serveur stable

**Test 3: Back/Forward Navigation**
```
# Naviguer: Home → About → Contact
# Cliquer sur Back (←) plusieurs fois
# Cliquer sur Forward (→)
```
- [ ] Navigation fonctionne correctement
- [ ] Pages du cache navigateur s'affichent

**Test 4: Download File**
```
http://localhost:8080/uploads/test.txt
http://localhost:8080/image/photo.jpg
```
- [ ] Fichier se télécharge correctement
- [ ] Taille du fichier correcte
- [ ] Type MIME correct

**Test 5: POST from HTML Form**
```html
<!-- Tester un formulaire HTML -->
http://localhost:8080/contact.html
```
- [ ] Remplir le formulaire
- [ ] Submit
- [ ] Vérifier dans Network tab: méthode POST
- [ ] Vérifier la réponse

**Test 6: Large Page**
```
http://localhost:8080/large-page.html
```
- [ ] Page se charge complètement
- [ ] Images lourdes se chargent
- [ ] Pas de timeout

**Test 7: Special Characters in URL**
```
http://localhost:8080/page%20with%20spaces.html
http://localhost:8080/accents-éà.html
```
- [ ] Gestion correcte de l'URL encoding
- [ ] 200 si le fichier existe, 404 sinon

**Test 8: Session/Cookies**
```
http://localhost:8080/counter.html
```
- [ ] Vérifier dans DevTools → Application → Cookies
- [ ] Cookie `session_id` est présent
- [ ] Le compteur de visites fonctionne
- [ ] Refresh la page → compteur augmente

### 8. DevTools Console Check
```
# Ouvrir Console tab dans DevTools
```
**Vérifier:**
- [ ] Pas d'erreurs JavaScript
- [ ] Pas d'erreurs de chargement de ressources
- [ ] Pas de warnings CORS (sauf si attendu)

### 9. Performance Check (DevTools)
```
# Onglet Network → regarder le timing
```
**Vérifier:**
- [ ] Temps de réponse raisonnable (< 100ms pour fichiers statiques)
- [ ] Pas de requêtes qui pendent indéfiniment
- [ ] Waterfall cohérent (HTML d'abord, puis CSS/JS/images)

### 10. Mobile View (Responsive)
```
# Dans DevTools, cliquer sur l'icône mobile (Ctrl+Shift+M)
# Ou F12 → Toggle device toolbar
```
- [ ] Tester en iPhone/iPad view
- [ ] Tester en Android view
- [ ] Site s'affiche correctement

---

## Partie 6: Port Issues

### 1. Multiple Ports with Different Websites

**Créer un fichier de configuration avec plusieurs ports:**

```nginx
# config/multi_port.conf

server {
    listen 8080;
    server_name localhost;

    location / {
        root ./www;
        index index.html;
    }
}

server {
    listen 8081;
    server_name site2;

    location / {
        root ./www2;
        index index.html;
    }
}

server {
    listen 8082;
    server_name site3;

    location / {
        root ./www3;
        index index.html;
    }
}
```

**Préparer les sites:**
```bash
# Créer des répertoires distincts
mkdir -p www2 www3

# Site 1 (port 8080)
echo "<h1>Site 1 - Port 8080</h1>" > www/index.html

# Site 2 (port 8081)
echo "<h1>Site 2 - Port 8081</h1>" > www2/index.html

# Site 3 (port 8082)
echo "<h1>Site 3 - Port 8082</h1>" > www3/index.html
```

**Lancer le serveur:**
```bash
./webserv config/multi_port.conf
```

**Tester dans le navigateur:**
```
http://localhost:8080/  → Doit afficher "Site 1 - Port 8080"
http://localhost:8081/  → Doit afficher "Site 2 - Port 8081"
http://localhost:8082/  → Doit afficher "Site 3 - Port 8082"
```

**Vérifier que les 3 ports écoutent:**
```bash
netstat -tulpn | grep webserv
# ou
lsof -i :8080,8081,8082

# Doit montrer:
# *:8080 (LISTEN)
# *:8081 (LISTEN)
# *:8082 (LISTEN)
```

**Avec curl:**
```bash
curl http://localhost:8080/
curl http://localhost:8081/
curl http://localhost:8082/
```

✅ **Points à vérifier:**
- [ ] Chaque port sert un site différent
- [ ] Les contenus sont distincts
- [ ] Pas de mélange entre les sites
- [ ] Tous les ports fonctionnent simultanément

---

### 2. Same Port Multiple Times (Should NOT Work)

**Créer un fichier config avec le même port en double:**

```nginx
# config/duplicate_port.conf

server {
    listen 8080;
    server_name site1;

    location / {
        root ./www;
        index index.html;
    }
}

server {
    listen 8080;  # ← MÊME PORT !
    server_name site2;

    location / {
        root ./www2;
        index index.html;
    }
}
```

**Tester:**
```bash
./webserv config/duplicate_port.conf
```

**Comportement attendu:**

**Option A - Virtual Hosts (ACCEPTABLE):**
- Le serveur démarre correctement
- Les deux serveurs partagent le port 8080
- La sélection se fait via le Host header
```bash
curl -H "Host: site1" http://localhost:8080/  # → www/
curl -H "Host: site2" http://localhost:8080/  # → www2/
```

**Option B - Erreur (ACCEPTABLE AUSSI):**
- Le serveur refuse de démarrer
- Message d'erreur: "Port 8080 already in use"
- Le programme se termine proprement

✅ **Points à vérifier:**
- [ ] Le comportement est cohérent et documenté
- [ ] Si virtual hosts: la sélection fonctionne
- [ ] Si erreur: message clair et pas de crash
- [ ] Le fichier de config est validé au parsing

---

### 3. Multiple Server Instances on Same Port

**Test 1: Lancer deux instances du même serveur**

```bash
# Terminal 1
./webserv config/default.conf

# Terminal 2 (essayer de lancer une 2ème instance)
./webserv config/default.conf
```

**Comportement attendu:**
```
Error: bind() failed: Address already in use
```

✅ **Points à vérifier:**
- [ ] La 2ème instance NE DOIT PAS démarrer
- [ ] Message d'erreur clair: "Address already in use"
- [ ] La 2ème instance se termine proprement (pas de crash)
- [ ] La 1ère instance continue de fonctionner normalement

**Test avec curl:**
```bash
# La 1ère instance doit toujours répondre
curl http://localhost:8080/
```

---

**Test 2: Deux configs différentes avec port commun**

```bash
# config/server1.conf
server {
    listen 8080;
    server_name server1;
    location / { root ./www; }
}

# config/server2.conf
server {
    listen 8080;  # ← Même port
    server_name server2;
    location / { root ./www2; }
}
```

```bash
# Terminal 1
./webserv config/server1.conf

# Terminal 2 (essayer de lancer avec l'autre config)
./webserv config/server2.conf
```

**Résultat attendu:**
```
Error: bind() failed: Address already in use
```

✅ **Points à vérifier:**
- [ ] Impossible de lancer les deux en même temps
- [ ] Le deuxième serveur échoue proprement
- [ ] Message d'erreur explicite
- [ ] Pas de corruption de la 1ère instance

---

**Test 3: Configs avec ports partiellement communs**

```bash
# config/overlap1.conf
server {
    listen 8080;
    listen 8081;
}

# config/overlap2.conf
server {
    listen 8081;  # ← Overlap sur 8081
    listen 8082;
}
```

```bash
# Terminal 1
./webserv config/overlap1.conf
# → Écoute sur 8080 et 8081

# Terminal 2
./webserv config/overlap2.conf
# → DOIT ÉCHOUER (8081 déjà pris)
```

**Vérifier:**
```bash
# Après avoir lancé overlap1.conf
netstat -tulpn | grep webserv
# Devrait montrer: 8080, 8081

# Essayer overlap2.conf
./webserv config/overlap2.conf
# Devrait échouer avec "Address already in use"
```

✅ **Points à vérifier:**
- [ ] Détection du conflit de port
- [ ] Échec propre au bind()
- [ ] Message d'erreur précis (quel port pose problème)
- [ ] Rollback si nécessaire (ne pas laisser de socket orphelin)

---

### 4. Questions pour l'évaluation

**Q: Pourquoi le serveur devrait-il fonctionner si une des configurations n'est pas fonctionnelle?**

**R: Deux cas possibles selon l'implémentation:**

**Cas A - Virtual Hosts (ACCEPTABLE):**
Si le serveur utilise des virtual hosts, plusieurs server blocks PEUVENT partager le même port.
- Le serveur démarre correctement
- La sélection se fait via le Host header
- C'est une fonctionnalité, pas un bug

**Cas B - Erreur stricte (ACCEPTABLE AUSSI):**
Si un serveur tente de bind() un port déjà utilisé:
1. **bind() retourne -1** avec errno = EADDRINUSE
2. Le serveur **doit échouer gracieusement**
3. Message d'erreur clair: "Port 8080 already in use"
4. Le programme **ne doit pas démarrer partiellement**

**Important:** Si ça fonctionne (virtual hosts), demander au groupe d'expliquer POURQUOI ça fonctionne. Puis continuer l'évaluation.

**Comportement correct:**
```cpp
int listenFd = socket(AF_INET, SOCK_STREAM, 0);
if (bind(listenFd, ...) < 0) {
    if (errno == EADDRINUSE)
        throw std::runtime_error("Port already in use");
    // Cleanup et exit
}
```

**Vérification dans le code:**
```bash
grep -A10 "bind(" srcs/server/Server.cpp
```

Le code devrait vérifier la valeur de retour de bind() et gérer l'erreur EADDRINUSE.

---

### 5. Test Complet Port Issues

```bash
# Test séquence complète

# 1. Lancer avec multi-ports (OK)
./webserv config/multi_port.conf &
PID1=$!
sleep 1
curl http://localhost:8080/
curl http://localhost:8081/
curl http://localhost:8082/

# 2. Essayer de lancer une autre instance (DOIT ÉCHOUER)
./webserv config/multi_port.conf
# → "Address already in use"

# 3. Tuer la première instance
kill $PID1
sleep 1

# 4. Vérifier que les ports sont libérés
lsof -i :8080,8081,8082
# → Devrait être vide

# 5. Relancer (DOIT FONCTIONNER maintenant)
./webserv config/multi_port.conf
```

✅ **Checklist finale:**
- [ ] Multiple ports fonctionnent simultanément
- [ ] Chaque port sert le bon contenu
- [ ] Impossible de bind le même port deux fois
- [ ] Message d'erreur clair pour EADDRINUSE
- [ ] Pas de crash, échec gracieux
- [ ] Les ports sont libérés proprement à l'arrêt

---

## Partie 7: Siege & Stress Test

*(Siege déjà installé dans Partie 1)*

### 1. Availability Test (> 99.5%)

**Créer une page vide/simple pour le test:**
```bash
echo "<html><body><h1>Test</h1></body></html>" > www/empty.html
```

**Lancer le serveur:**
```bash
./webserv config/default.conf
```

**Test de disponibilité avec siege -b:**
```bash
# -b : benchmark mode (pas de délai entre requêtes)
# -c : concurrent users
# -t : time duration
# -v : verbose

siege -b -c 10 -t 30s http://localhost:8080/empty.html
```

**Résultat attendu:**
```
Transactions:               12450 hits
Availability:              99.95 %    ← DOIT ÊTRE > 99.5%
Elapsed time:              29.87 secs
Data transferred:           1.23 MB
Response time:              0.02 secs
Transaction rate:         416.83 trans/sec
Throughput:                 0.04 MB/sec
Concurrency:                9.87
Successful transactions:   12450
Failed transactions:           6       ← Doit être très faible
```

✅ **Points à vérifier:**
- [ ] **Availability > 99.5%** (minimum requis)
- [ ] Failed transactions très faible (< 0.5%)
- [ ] Pas de crash du serveur
- [ ] Response time raisonnable (< 0.1s)

**Test plus intensif:**
```bash
# Plus de connexions simultanées
siege -b -c 50 -t 30s http://localhost:8080/empty.html
siege -b -c 100 -t 30s http://localhost:8080/empty.html

# Plus long
siege -b -c 25 -t 60s http://localhost:8080/empty.html
```

---

### 2. Memory Leak Check

**Terminal 1 - Lancer le serveur:**
```bash
./webserv config/default.conf
```

**Terminal 2 - Monitorer la mémoire:**
```bash
# Obtenir le PID
PID=$(pgrep webserv)

# Monitorer en continu
watch -n 1 "ps aux | grep webserv | grep -v grep"

# Ou avec plus de détails
watch -n 1 "ps -o pid,vsz,rss,comm -p $PID"
```

**Terminal 3 - Lancer siege:**
```bash
# Test prolongé
siege -b -c 25 -t 120s http://localhost:8080/

# Ou en boucle
for i in {1..10}; do
    echo "=== Round $i ==="
    siege -b -c 25 -t 30s http://localhost:8080/
    sleep 5
done
```

**Surveiller dans Terminal 2:**
```
PID    VSZ     RSS    COMMAND
12345  15420   3560   webserv    ← Au début
12345  15420   3580   webserv    ← Après 30s (OK: légère augmentation)
12345  15420   3590   webserv    ← Après 60s (OK: stable)
12345  15420   3595   webserv    ← Après 90s (OK: stable)
```

✅ **Points à vérifier:**
- [ ] RSS (Resident Set Size) ne doit PAS augmenter indéfiniment
- [ ] Légère augmentation initiale OK (caches, buffers)
- [ ] Doit se stabiliser après quelques requêtes
- [ ] Pas d'augmentation linéaire continue

**Memory leak détecté = FAIL:**
```
PID    VSZ     RSS    COMMAND
12345  15420   3560   webserv    ← Début
12345  16820   5230   webserv    ← Après 30s (augmentation)
12345  18940   7850   webserv    ← Après 60s (continue!)
12345  22450  11230   webserv    ← Après 90s (LEAK!)
```

**Alternative avec valgrind (plus précis):**
```bash
# Compiler en debug
make clean && make

# Lancer avec valgrind
valgrind --leak-check=full --show-leak-kinds=all \
  --track-origins=yes --log-file=valgrind.log \
  ./webserv config/default.conf &

# Lancer des requêtes
siege -b -c 10 -t 30s http://localhost:8080/

# Arrêter le serveur (Ctrl+C)

# Analyser le rapport
cat valgrind.log | grep "definitely lost"
cat valgrind.log | grep "LEAK SUMMARY"
```

---

### 3. Hanging Connections Check

**Test 1: Vérifier les connexions actives**
```bash
# Terminal 1: Lancer serveur
./webserv config/default.conf

# Terminal 2: Monitorer connexions
watch -n 1 "netstat -an | grep 8080 | grep ESTABLISHED | wc -l"

# Terminal 3: Lancer siege
siege -b -c 50 -t 30s http://localhost:8080/
```

**Pendant le test:**
- Nombre de connexions ESTABLISHED augmente (normal)

**Après le test (important!):**
- Nombre doit retourner à 0 ou très faible
- Pas de connexions qui restent en ESTABLISHED

**Vérifier les états de connexion:**
```bash
# Pendant le test
netstat -an | grep 8080

# Résultat OK:
tcp4  0  0  *.8080     *.*     LISTEN
tcp4  0  0  127.0.0.1.8080  127.0.0.1.54321  ESTABLISHED
tcp4  0  0  127.0.0.1.8080  127.0.0.1.54322  ESTABLISHED
... (plusieurs ESTABLISHED pendant le test)

# Après le test (attendre 5 secondes):
tcp4  0  0  *.8080     *.*     LISTEN
# Plus de ESTABLISHED = OK
```

**Test des TIME_WAIT:**
```bash
netstat -an | grep 8080 | grep TIME_WAIT | wc -l
```
- TIME_WAIT est normal après fermeture de connexion
- Doit diminuer progressivement

**Check des connexions "zombies":**
```bash
# Connexions en CLOSE_WAIT (mauvais signe)
netstat -an | grep 8080 | grep CLOSE_WAIT

# Doit être vide!
# Si des connexions restent en CLOSE_WAIT = problème
```

✅ **Points à vérifier:**
- [ ] Pas de connexions qui restent ESTABLISHED après le test
- [ ] Pas de CLOSE_WAIT (signe de fd non fermé)
- [ ] TIME_WAIT est OK et diminue naturellement
- [ ] Le serveur libère les ressources proprement

**Test avec lsof:**
```bash
# Avant siege
lsof -i :8080 | wc -l

# Pendant siege
lsof -i :8080 | wc -l  # Augmente

# Après siege (attendre 10s)
lsof -i :8080 | wc -l  # Devrait revenir au niveau initial
```

---

### 4. Siege Indefinitely (Long-term Stability)

**Test de stabilité longue durée:**

```bash
# Option 1: Très long test
siege -b -c 25 -t 300s http://localhost:8080/
# 5 minutes continu

# Option 2: Boucle infinie (Ctrl+C pour arrêter)
while true; do
    echo "=== $(date) ==="
    siege -b -c 25 -t 60s http://localhost:8080/
    sleep 5
done

# Option 3: Nombre fixe de rounds
for i in {1..20}; do
    echo "=== Round $i/20 ==="
    siege -b -c 25 -t 30s http://localhost:8080/
    echo "Waiting 5s..."
    sleep 5
done
```

**Pendant le test, monitorer:**

```bash
# Terminal séparé
while true; do
    clear
    echo "=== Server Monitoring ==="
    echo "Time: $(date)"
    echo ""

    # Memory
    echo "Memory Usage:"
    ps aux | grep webserv | grep -v grep
    echo ""

    # Connections
    echo "Active connections:"
    netstat -an | grep 8080 | grep ESTABLISHED | wc -l
    echo ""

    # File descriptors
    echo "Open FDs:"
    lsof -p $(pgrep webserv) | wc -l
    echo ""

    sleep 2
done
```

✅ **Points à vérifier:**
- [ ] Le serveur ne doit JAMAIS crasher
- [ ] Mémoire stable (pas d'augmentation continue)
- [ ] Connexions se ferment proprement entre chaque round
- [ ] Response time reste stable
- [ ] Availability reste > 99.5%
- [ ] Nombre de fd ouverts ne grandit pas indéfiniment

**Résultats attendus après 20 rounds:**
```
Round 1:  Availability 99.98%, Response 0.02s, Memory 3.5MB
Round 5:  Availability 99.97%, Response 0.02s, Memory 3.6MB
Round 10: Availability 99.96%, Response 0.02s, Memory 3.6MB
Round 15: Availability 99.97%, Response 0.02s, Memory 3.6MB
Round 20: Availability 99.95%, Response 0.02s, Memory 3.6MB
```
→ Performance stable, mémoire stable = ✅

---

### 5. Stress Test avec Différents Scénarios

**Test A: Pages statiques**
```bash
siege -b -c 50 -t 60s http://localhost:8080/index.html
```

**Test B: Fichiers de tailles différentes**
```bash
# Créer des fichiers test
echo "small" > www/small.txt
head -c 1M /dev/urandom > www/medium.bin
head -c 10M /dev/urandom > www/large.bin

# Test
siege -b -c 25 -t 30s -f urls.txt

# urls.txt:
# http://localhost:8080/small.txt
# http://localhost:8080/medium.bin
# http://localhost:8080/large.bin
```

**Test C: Avec CGI (plus stressant)**
```bash
siege -b -c 10 -t 30s http://localhost:8080/cgi-bin/py/contact.py
# Availability peut être plus faible (CGI = plus lent)
# Mais doit rester > 95%
```

**Test D: Mix GET/POST**
```bash
# Créer un fichier urls.txt avec POST
cat > urls.txt << EOF
http://localhost:8080/ GET
http://localhost:8080/upload POST test=data
http://localhost:8080/about.html GET
EOF

siege -b -c 25 -t 30s -f urls.txt
```

---

### 6. Checklist Complète Stress Test

```bash
# 1. Availability test
siege -b -c 25 -t 60s http://localhost:8080/
# → Availability > 99.5% ✅

# 2. Memory check avant
ps aux | grep webserv | grep -v grep

# 3. Long stress test
siege -b -c 50 -t 300s http://localhost:8080/

# 4. Memory check après
ps aux | grep webserv | grep -v grep
# → Mémoire stable ✅

# 5. Connexions check
netstat -an | grep 8080 | grep ESTABLISHED
# → Devrait être vide ou minimal ✅

# 6. Serveur toujours up
curl http://localhost:8080/
# → 200 OK ✅

# 7. Repeat test
siege -b -c 25 -t 60s http://localhost:8080/
# → Availability toujours > 99.5% ✅
```

✅ **Critères de réussite:**
- [ ] Availability > 99.5% sur page simple
- [ ] Pas de memory leak (RSS stable)
- [ ] Pas de hanging connections (ESTABLISHED = 0 après test)
- [ ] Serveur stable indéfiniment (pas de crash)
- [ ] Performance ne se dégrade pas avec le temps

❌ **Échec si:**
- Availability < 99.5%
- Mémoire augmente continuellement
- Connexions restent ouvertes
- Serveur crash
- Performance se dégrade

---

## BONUS PART

⚠️ **ATTENTION:** N'évaluer les bonus QUE SI la partie obligatoire est entièrement et parfaitement réalisée, ET que la gestion d'erreurs gère les usages inattendus ou incorrects. Si tous les points obligatoires ne sont pas validés, les bonus doivent être totalement ignorés.

---

## Bonus 1: Cookies and Session Management

### Vérification du système de sessions

**Test 1: Création de session**
```bash
# Première requête - créer une session
curl -v http://localhost:8080/ 2>&1 | grep -i "set-cookie"

# Doit afficher quelque chose comme:
# Set-Cookie: session_id=abc123def456; Path=/; HttpOnly
```

**Test 2: Compteur de visites**
```bash
# Ouvrir dans le navigateur
firefox http://localhost:8080/counter.html

# DevTools → Application → Cookies
# Vérifier:
# - Cookie "session_id" présent
# - Valeur unique (ex: abc123def456)
# - Path: /
# - HttpOnly: true
```

**Recharger la page plusieurs fois:**
- [ ] Le compteur augmente à chaque visite
- [ ] Le session_id reste le même
- [ ] Les visites sont comptées correctement

**Test 3: Persistance de session**
```bash
# Sauvegarder le cookie
SESSION_ID=$(curl -v http://localhost:8080/ 2>&1 | grep -i "set-cookie" | sed -n 's/.*session_id=\([^;]*\).*/\1/p')

echo "Session ID: $SESSION_ID"

# Requête 1 avec le cookie
curl -b "session_id=$SESSION_ID" http://localhost:8080/counter.html

# Requête 2 avec le même cookie
curl -b "session_id=$SESSION_ID" http://localhost:8080/counter.html

# Le compteur doit augmenter entre les deux requêtes
```

**Test 4: Différentes sessions**
```bash
# Navigateur 1: Firefox
firefox http://localhost:8080/counter.html
# Compteur: 1, 2, 3...

# Navigateur 2: Chrome (mode normal)
google-chrome http://localhost:8080/counter.html
# Compteur: 1, 2, 3... (session séparée)

# Les deux compteurs sont indépendants
```

**Test 5: Expiration de session**
```bash
# Vérifier le timeout de session (généralement 30 min)
# Attendre ou modifier le timeout dans le code pour tester

# Après expiration:
# - Cookie invalide
# - Nouvelle session créée
# - Compteur repart à 1
```

**Vérifier dans le code:**
```bash
grep -r "session" srcs/ --include="*.cpp" --include="*.hpp"
grep -r "Set-Cookie" srcs/ --include="*.cpp"
```

✅ **Points à valider:**
- [ ] Cookie session_id généré automatiquement
- [ ] HttpOnly flag présent (sécurité)
- [ ] Path correct
- [ ] Session persistante entre requêtes
- [ ] Données de session stockées côté serveur
- [ ] Expiration de session implémentée
- [ ] Compteur de visites fonctionne
- [ ] Sessions indépendantes par client

**Structure de session dans le code:**
```cpp
struct SessionData {
    time_t lastActive;
    std::string username;
    int visitCount;
};

std::map<std::string, SessionData> _sessions;
```

---

## Bonus 2: Multiple CGI Systems

⚠️ **Critère du bonus:** Il doit y avoir **plus d'un système CGI** (more than one CGI system).

### Vérification des CGI supportés

**Implémentation typique:**
1. **Python CGI** (via /usr/bin/python3)
2. **PHP CGI** (via /usr/bin/php-cgi)
3. **Optionnel:** Perl, Ruby, ou autre

**Test 1: Python CGI**
```bash
# GET
curl http://localhost:8080/cgi-bin/py/contact.py

# POST
curl -X POST -d "name=John&email=test@test.com" \
  http://localhost:8080/cgi-bin/py/contact.py
```

**Vérifier la configuration:**
```nginx
location /cgi-bin/py {
    root ./cgi-bin/py;
    allowed_methods GET POST;
    cgi_extension .py;
    cgi_path /usr/bin/python3;
}
```

**Test 2: PHP CGI**
```bash
# GET
curl http://localhost:8080/cgi-bin/php/qrcode.php

# POST
curl -X POST -d "text=HelloWorld" \
  http://localhost:8080/cgi-bin/php/qrcode.php
```

**Vérifier la configuration:**
```nginx
location /cgi-bin/php {
    root ./cgi-bin/php;
    allowed_methods GET POST;
    cgi_extension .php;
    cgi_path /usr/bin/php-cgi;
}
```

**Test 3: Créer un nouveau CGI (Perl - optionnel)**
```bash
# Installer perl si nécessaire
which perl

# Créer un script Perl
cat > cgi-bin/perl/test.pl << 'EOF'
#!/usr/bin/perl
print "Content-Type: text/plain\r\n";
print "\r\n";
print "Hello from Perl CGI!\n";
print "Time: " . localtime() . "\n";
EOF

chmod +x cgi-bin/perl/test.pl

# Ajouter dans la config
# location /cgi-bin/perl {
#     root ./cgi-bin/perl;
#     cgi_extension .pl;
#     cgi_path /usr/bin/perl;
# }

# Tester
curl http://localhost:8080/cgi-bin/perl/test.pl
```

**Test 4: Variables d'environnement CGI**
```bash
# Créer un script qui affiche toutes les variables
cat > cgi-bin/py/env.py << 'EOF'
#!/usr/bin/env python3
import os
print("Content-Type: text/plain\r")
print("\r")
print("CGI Environment Variables:\n")
for key, value in sorted(os.environ.items()):
    if key.startswith(('REQUEST_', 'QUERY_', 'CONTENT_', 'HTTP_', 'SERVER_', 'GATEWAY_')):
        print(f"{key}={value}")
EOF

chmod +x cgi-bin/py/env.py
curl "http://localhost:8080/cgi-bin/py/env.py?test=123"
```

**Variables attendues:**
- REQUEST_METHOD
- QUERY_STRING
- CONTENT_LENGTH
- CONTENT_TYPE
- SERVER_NAME
- SERVER_PORT
- GATEWAY_INTERFACE (CGI/1.1)
- HTTP_* (headers)

**Test 5: CGI avec différents Content-Types**
```bash
# HTML
cat > cgi-bin/py/html.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/html\r")
print("\r")
print("<html><body><h1>HTML from CGI</h1></body></html>")
EOF

# JSON
cat > cgi-bin/py/json.py << 'EOF'
#!/usr/bin/env python3
import json
print("Content-Type: application/json\r")
print("\r")
print(json.dumps({"status": "success", "message": "Hello from CGI"}))
EOF

chmod +x cgi-bin/py/*.py

curl http://localhost:8080/cgi-bin/py/html.py
curl http://localhost:8080/cgi-bin/py/json.py
```

**Vérifier dans le code:**
```bash
# Router détecte le CGI
grep -A20 "isCGI" srcs/server/Router.cpp

# CGI.cpp execute les scripts
grep -A30 "startAsync" srcs/cgi/CGI.cpp

# Gestion des différents interpréteurs
grep "cgi_path" srcs/config/Location.cpp
```

✅ **Points à valider:**
- [ ] Au moins 2 CGI différents (Python + PHP minimum)
- [ ] Configuration par extension (.py, .php)
- [ ] cgi_path configurable
- [ ] Variables d'environnement CGI correctes
- [ ] GET et POST fonctionnent pour chaque CGI
- [ ] Gestion des erreurs par CGI
- [ ] Timeout appliqué à tous les CGI
- [ ] Support de différents Content-Type en sortie

**Architecture CGI dans le code:**
```cpp
struct CGIProcess {
    pid_t pid;
    int pipeIn;
    int pipeOut;
    time_t startTime;
    std::string output;
    bool inputWritten;
};

// Méthode qui détecte et lance le bon CGI
CGIProcess* CGI::startAsync(const RouteMatch& match,
                            const HttpRequest& request);
```

---

## Checklist Finale Bonus

### Cookies & Sessions ✅
- [ ] Cookie session_id généré automatiquement
- [ ] Attributs corrects (HttpOnly, Path)
- [ ] Sessions persistantes
- [ ] Expiration de session
- [ ] Compteur de visites fonctionnel
- [ ] Code propre et maintenable

### Multiple CGI ✅
- [ ] **AU MOINS 2 systèmes CGI différents** (Python + PHP minimum)
- [ ] Chaque CGI fonctionne correctement
- [ ] Configuration flexible (cgi_path, cgi_extension)
- [ ] Variables d'environnement CGI correctes
- [ ] GET et POST fonctionnent pour chaque CGI
- [ ] Gestion d'erreurs robuste pour tous les CGI
- [ ] Architecture extensible à d'autres langages

---

## Quick Checks

### Lancer le serveur
```bash
make
./webserv config/default.conf
```

### Vérifier les fd ouverts
```bash
lsof -p $(pgrep webserv)
```

### Vérifier les sockets
```bash
netstat -tulpn | grep webserv
# ou
ss -tulpn | grep webserv
```

### Logs en temps réel
```bash
# Terminal 1
./webserv config/default.conf

# Terminal 2
curl http://localhost:8080/
```

---

## Points clés à mentionner

✅ **poll()** - Un seul poll() pour tous les fd
✅ **Non-blocking I/O** - Tous les sockets en mode non-bloquant
✅ **Gestion CGI** - Asynchrone avec timeout 5s
✅ **Pas d'errno** - On vérifie juste la valeur de retour
✅ **Virtual hosts** - Selection via Host header
✅ **Routing** - Routes vers différents répertoires
✅ **Upload** - Avec limite de taille configurable
✅ **Sessions** - Cookies pour le compteur de visites
