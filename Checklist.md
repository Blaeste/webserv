# ✅ Checklist de Tests - Webserv

## 🎯 Tests à chaque itération

### ✔️ Compilation & Démarrage
- [ ] `make` compile sans erreurs ni warnings
- [ ] `make re` fonctionne correctement
- [ ] Le serveur démarre avec `./webserv config/default.conf`
- [ ] Le serveur démarre sans configuration (config par défaut)
- [ ] Le serveur refuse les configs invalides avec un message d'erreur clair
- [ ] Le serveur se ferme proprement avec Ctrl+C (SIGINT)
- [ ] Pas de fuites mémoire (vérifier avec `valgrind`)

### ✔️ Parsing de Configuration
- [ ] Parsing de plusieurs blocs `server`
- [ ] Parsing de plusieurs blocs `location` par serveur
- [ ] `listen` : ports valides (8080, 8081, etc.)
- [ ] `server_name` : noms multiples acceptés
- [ ] `client_max_body_size` : avec unités (M, K)
- [ ] `error_page` : mapping code → fichier
- [ ] `root` et `index` : chemins relatifs/absolus
- [ ] `allowed_methods` : GET, POST, DELETE
- [ ] `autoindex` : on/off
- [ ] `upload_path` : répertoire d'upload
- [ ] `cgi_extension` et `cgi_path` : config CGI
- [ ] Gestion des erreurs de syntaxe dans le fichier de config

---

## 🌐 Tests HTTP Basiques

### ✔️ Méthode GET
- [ ] GET d'une page statique existante (`/index.html`)
- [ ] GET d'un fichier CSS (`/css/style.css`)
- [ ] GET d'une image (`/images/test.png`)
- [ ] GET d'un fichier texte (`/text/test.txt`)
- [ ] GET de la racine `/` (redirige vers `index.html`)
- [ ] GET d'un fichier inexistant → 404
- [ ] GET d'un répertoire sans index → 403 ou listing selon `autoindex`
- [ ] GET avec `autoindex on` → listing HTML du répertoire
- [ ] GET avec `autoindex off` → 403

### ✔️ Méthode POST
- [ ] POST avec body simple (form-data)
- [ ] POST avec `Content-Length` correct
- [ ] POST avec `Transfer-Encoding: chunked`
- [ ] POST sans `Content-Length` ni chunked → 411
- [ ] POST avec body > `client_max_body_size` → 413
- [ ] POST sur une location interdite → 405

### ✔️ Méthode DELETE
- [ ] DELETE d'un fichier existant dans `/uploads`
- [ ] DELETE d'un fichier inexistant → 404
- [ ] DELETE sur location non autorisée → 405
- [ ] DELETE sur un répertoire → comportement défini (404 ou 403)

### ✔️ Headers HTTP
- [ ] Header `Host` correctement parsé
- [ ] Header `Content-Type` correctement détecté (auto-mime)
- [ ] Header `Content-Length` respecté
- [ ] Header `Transfer-Encoding: chunked` géré
- [ ] Header `Connection: keep-alive` maintient la connexion
- [ ] Header `Connection: close` ferme après réponse
- [ ] Header `User-Agent` parsé (optionnel, informatif)
- [ ] Headers multiples sur plusieurs lignes

### ✔️ Status Codes
- [ ] 200 OK (requête réussie)
- [ ] 201 Created (upload réussi)
- [ ] 204 No Content (DELETE réussi)
- [ ] 301/302 Moved Permanently / Found (redirections)
- [ ] 400 Bad Request (requête malformée)
- [ ] 403 Forbidden (accès interdit)
- [ ] 404 Not Found (ressource inexistante)
- [ ] 405 Method Not Allowed (méthode interdite)
- [ ] 411 Length Required (POST sans Content-Length)
- [ ] 413 Payload Too Large (body trop gros)
- [ ] 500 Internal Server Error (erreur serveur)
- [ ] 501 Not Implemented (méthode non supportée)
- [ ] 504 Gateway Timeout (timeout CGI)

### ✔️ Pages d'Erreur Personnalisées
- [ ] 404 affiche `/error_pages/404.html`
- [ ] 500 affiche `/error_pages/500.html`
- [ ] 403 affiche `/error_pages/403.html`
- [ ] 413 affiche `/error_pages/413.html`
- [ ] Fallback sur page d'erreur par défaut si fichier manquant

---

## 🚀 Tests CGI (Common Gateway Interface)

### ✔️ CGI Python
- [ ] GET `/cgi-bin/py/test.py` → exécution Python
- [ ] POST `/cgi-bin/py/post_test.py` avec données
- [ ] Variables d'environnement CGI correctes :
  - [ ] `REQUEST_METHOD`
  - [ ] `QUERY_STRING`
  - [ ] `CONTENT_TYPE`
  - [ ] `CONTENT_LENGTH`
  - [ ] `PATH_INFO`
  - [ ] `SCRIPT_NAME`
  - [ ] `SERVER_NAME`
  - [ ] `SERVER_PORT`
  - [ ] `SERVER_PROTOCOL`
- [ ] Script avec headers custom (`Content-Type`, etc.)
- [ ] Script qui affiche `$_GET` et `$_POST` (Python)
- [ ] Script avec timeout → 504
- [ ] Script infini interrompu après timeout
- [ ] Script qui plante (exit 1) → 500

### ✔️ CGI PHP
- [ ] GET `/cgi-bin/php/test.php` → exécution PHP
- [ ] POST `/cgi-bin/php/post_test.php` avec formulaire
- [ ] Variables `$_GET` et `$_POST` correctes
- [ ] Script PHP avec headers custom
- [ ] Script PHP avec erreur → 500

### ✔️ Cas Limites CGI
- [ ] Script CGI inexistant → 404
- [ ] Extension CGI non configurée → traiter comme fichier statique ou 403
- [ ] CGI sur location non autorisée → 405
- [ ] CGI avec output très long (> buffer size)
- [ ] CGI avec output vide

---

## 📤 Tests Upload de Fichiers

### ✔️ Upload Simple
- [ ] POST `/uploads` avec fichier texte
- [ ] POST `/uploads` avec fichier image
- [ ] POST `/uploads` avec fichier > 1MB
- [ ] Fichier uploadé présent dans `./uploads/`
- [ ] Nom de fichier conservé ou généré (selon implémentation)
- [ ] Upload avec body > `client_max_body_size` → 413

### ✔️ Multipart Form-Data
- [ ] Parsing correct de `Content-Type: multipart/form-data`
- [ ] Extraction du boundary
- [ ] Parsing des champs et fichiers multiples
- [ ] Gestion de plusieurs fichiers en un seul POST
- [ ] Gestion des noms de fichiers avec espaces/caractères spéciaux

### ✔️ Upload + DELETE
- [ ] Upload fichier `test_file.txt`
- [ ] Vérifier présence dans `/uploads`
- [ ] DELETE `/uploads/test_file.txt` → 204
- [ ] Vérifier disparition du fichier

---

## 🔄 Tests de Routage & Locations

### ✔️ Matching de Locations
- [ ] URI `/` match location `/`
- [ ] URI `/uploads/file.txt` match location `/uploads`
- [ ] URI `/cgi-bin/py/test.py` match location `/cgi-bin/py`
- [ ] Longest prefix matching (pas exact match)
- [ ] Location inexistante → 404

### ✔️ Méthodes Autorisées
- [ ] GET autorisé sur `/` → 200
- [ ] POST autorisé sur `/` → 200
- [ ] DELETE non autorisé sur `/` → 405
- [ ] POST autorisé sur `/uploads` → 200/201
- [ ] DELETE autorisé sur `/uploads` → 204

### ✔️ Root & Index
- [ ] `root ./www` + `index index.html` → sert `/www/index.html`
- [ ] `root ./www` sans index → 403 ou autoindex
- [ ] Chemin relatif vs absolu dans root

---

## 🌍 Tests Multi-Serveurs (Virtual Hosts)

### ✔️ Plusieurs Ports
- [ ] Serveur 1 sur port 8080
- [ ] Serveur 2 sur port 8081
- [ ] Requête sur 8080 → config serveur 1
- [ ] Requête sur 8081 → config serveur 2
- [ ] Ports différents, configs différentes

### ✔️ Server Names
- [ ] `server_name localhost` → match `Host: localhost`
- [ ] `server_name example.com` → match `Host: example.com`
- [ ] Header `Host` manquant → serveur par défaut (premier)
- [ ] Header `Host` inconnu → serveur par défaut

---

## 🔥 Tests de Robustesse & Limites

### ✔️ Requêtes Malformées
- [ ] Requête sans méthode → 400
- [ ] Requête sans URI → 400
- [ ] Requête sans HTTP version → 400
- [ ] Headers sans `:` → 400
- [ ] Body sans `Content-Length` ni chunked sur POST → 411
- [ ] Chunked encoding malformé → 400

### ✔️ Concurrence & Charge
- [ ] 10 clients simultanés (connexions keep-alive)
- [ ] 100 requêtes successives sans fermer la connexion
- [ ] Plusieurs uploads simultanés
- [ ] Plusieurs CGI simultanés
- [ ] Serveur ne crash pas sous charge
- [ ] Serveur répond toujours (pas de deadlock)

### ✔️ Keep-Alive & Timeouts
- [ ] Connexion keep-alive reste ouverte entre requêtes
- [ ] Timeout après inactivité (configurable ou ~5s)
- [ ] Connexion fermée proprement après timeout
- [ ] `Connection: close` ferme immédiatement après réponse

### ✔️ Gestion Mémoire
- [ ] Pas de fuites mémoire après 1000 requêtes
- [ ] Valgrind sans erreurs sur session normale
- [ ] Valgrind sans erreurs sur uploads multiples
- [ ] Valgrind sans erreurs sur CGI multiples

### ✔️ Fichiers & Permissions
- [ ] Fichier sans permissions de lecture → 403
- [ ] Répertoire sans permissions d'exécution → 403
- [ ] Upload dans répertoire protégé → 403 ou 500
- [ ] Symbolic links (selon implémentation) → à définir

---

## 🧪 Tests avec Outils Externes

### ✔️ cURL
```bash
# GET simple
curl http://localhost:8080/

# GET avec headers
curl -H "Host: localhost" http://localhost:8080/

# POST avec données
curl -X POST -d "key=value" http://localhost:8080/

# POST avec fichier
curl -X POST -F "file=@test.txt" http://localhost:8080/uploads

# DELETE
curl -X DELETE http://localhost:8080/uploads/test.txt

# Verbose (voir headers)
curl -v http://localhost:8080/
```

### ✔️ Navigateur Web
- [ ] Ouvrir `http://localhost:8080/` dans Chrome/Firefox
- [ ] Navigation entre pages
- [ ] Affichage correct des CSS et images
- [ ] Formulaire d'upload fonctionnel
- [ ] Pages d'erreur personnalisées affichées

### ✔️ Siege / AB (Apache Bench)
```bash
# Test de charge avec siege
siege -c 10 -r 100 http://localhost:8080/

# Test avec ab
ab -n 1000 -c 10 http://localhost:8080/
```
- [ ] Pas de crash sous charge
- [ ] Temps de réponse acceptable
- [ ] Taux de succès élevé (>99%)

### ✔️ Telnet
```bash
telnet localhost 8080
GET / HTTP/1.1
Host: localhost

```
- [ ] Réponse HTTP complète reçue
- [ ] Status line, headers, body corrects

### ✔️ Postman
- [ ] Import collection de tests
- [ ] Tests GET, POST, DELETE
- [ ] Vérification des status codes
- [ ] Vérification des headers de réponse

---

## 🔒 Tests de Sécurité

### ✔️ Path Traversal
- [ ] GET `/../etc/passwd` → 403 ou 404 (pas d'accès hors root)
- [ ] GET `/../../etc/passwd` → bloqué
- [ ] GET avec URL encoding `%2e%2e%2f` → bloqué

### ✔️ Injection & XSS (optionnel)
- [ ] CGI avec input malicieux → échappé ou pas exécuté
- [ ] Upload de fichier avec nom malicieux → nom sanitisé

### ✔️ DoS Prevention
- [ ] Body trop gros → 413 (pas de buffer overflow)
- [ ] Trop de headers → limite ou 400
- [ ] Requête très lente (slowloris) → timeout
- [ ] Connexions multiples d'un même client → limité ou géré

---

## 📊 Cas de Test Réels

### Scénario 1 : Navigation Web Classique
1. Ouvrir `http://localhost:8080/` → affiche `index.html`
2. Cliquer sur lien vers `/about.html` → affiche `about.html`
3. Charger image `/images/logo.png` → affichée
4. Charger CSS `/css/style.css` → appliqué
5. ✅ Toutes les ressources chargées, page complète

### Scénario 2 : Upload & Gestion de Fichiers
1. GET `http://localhost:8080/upload.html` → formulaire d'upload
2. Sélectionner fichier `photo.jpg`
3. POST `/uploads` → réponse 201
4. GET `/uploads` (avec autoindex) → fichier listé
5. GET `/uploads/photo.jpg` → fichier téléchargé
6. DELETE `/uploads/photo.jpg` → 204
7. GET `/uploads/photo.jpg` → 404
8. ✅ Cycle complet upload → list → download → delete

### Scénario 3 : Exécution CGI
1. GET `/cgi-bin/py/test.py` → script Python exécuté
2. GET `/cgi-bin/py/test.py?name=Alice` → query string passée
3. Réponse contient "Hello Alice" → ✅
4. POST `/cgi-bin/py/post_test.py` avec `name=Bob`
5. Réponse contient "Received: Bob" → ✅

### Scénario 4 : Gestion d'Erreurs
1. GET `/nonexistent.html` → 404 avec page custom
2. POST `/` (méthode interdite) → 405
3. POST avec body 20MB (limite 10M) → 413
4. GET CGI qui timeout → 504
5. ✅ Toutes les erreurs gérées proprement

### Scénario 5 : Multi-Serveurs
1. GET `http://localhost:8080/` → serveur 1, `www/index.html`
2. GET `http://localhost:8081/` → serveur 2, `www2/index.html`
3. Contenu différent confirmé → ✅

---

## 🏁 Validation Finale (avant remise)

### ✔️ Checklist Critique
- [ ] **Compilation** : `make` sans warnings
- [ ] **Norminette** : code conforme (si applicable)
- [ ] **Valgrind** : pas de fuites mémoire
- [ ] **Tests manuels** : tous les cas de base passent
- [ ] **Tests navigateur** : site web utilisable
- [ ] **Tests CGI** : Python et PHP fonctionnent
- [ ] **Tests upload** : upload et delete marchent
- [ ] **Tests erreurs** : 404, 403, 500 gérés
- [ ] **Tests charge** : pas de crash sous siege/ab
- [ ] **README** : documentation claire et complète

### ✔️ Tests avec le Sujet
- [ ] Tous les exemples du sujet fonctionnent
- [ ] Toutes les fonctionnalités obligatoires implémentées
- [ ] Respect de la RFC 7230-7235 (HTTP/1.1)
- [ ] Respect des contraintes (C++98, poll(), etc.)

---

## 📝 Notes

### Outils Recommandés
- **cURL** : tests CLI rapides
- **Postman** : tests structurés avec collections
- **Siege/AB** : tests de charge
- **Valgrind** : détection fuites mémoire
- **Navigateur** : tests utilisateur réels

### Automatisation (optionnel)
- Script bash avec tests cURL
- Suite pytest pour tests Python
- Makefile avec target `make test`

### Documentation des Bugs
Pour chaque bug trouvé :
1. Description du comportement attendu
2. Comportement observé
3. Étapes de reproduction
4. Environnement (OS, config, etc.)
5. Priorité (bloquant, majeur, mineur)

---

**Bonne recette ! 🚀**
