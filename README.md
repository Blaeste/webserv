# Webserv - HTTP/1.1 Server

A lightweight, RFC-compliant HTTP/1.1 web server written in C++98, capable of handling static files, CGI execution, file uploads, and more.

---

## 📋 Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [Quick Start](#quick-start)
- [Configuration](#configuration)
- [Architecture](#architecture)
- [HTTP Status Codes](#http-status-codes)
- [CGI Implementation](#cgi-implementation)
- [File Upload](#file-upload)
- [Security](#security)
- [Testing](#testing)
- [Limitations](#limitations)

---

## ✨ Features

### Core HTTP/1.1 Support
- **Multiple HTTP Methods**: GET, POST, DELETE
- **Request Parsing**: Full HTTP/1.1 request parsing with headers and body
- **Response Building**: Automatic status codes, headers, and body generation
- **Chunked Transfer Encoding**: Support for chunked request bodies
- **Keep-Alive**: Persistent connections with timeout management
- **MIME Types**: Automatic content-type detection based on file extensions

### Advanced Features
- **CGI Support**: Execute PHP and Python scripts with full environment setup
- **File Upload**: Handle multipart/form-data file uploads
- **Directory Listing**: Auto-index for browsing directories
- **Redirections**: HTTP 301/302 redirects
- **Custom Error Pages**: Customizable error pages per status code
- **Body Size Limiting**: Configurable maximum request body size (prevents DoS)
- **Virtual Hosts**: Multiple server blocks with different server names
- **Location Routing**: URL path-based routing with method restrictions

### Configuration
- **Nginx-like Syntax**: Familiar configuration file format
- **Multiple Servers**: Run multiple virtual hosts on different ports
- **Location Blocks**: Fine-grained control over different URL paths
- **Method Restrictions**: Allow/deny specific HTTP methods per location
- **Upload Directories**: Configurable upload paths
- **CGI Configuration**: Per-location CGI interpreter setup

---

## 📁 Project Structure

```
webserv/
├── srcs/
│   ├── main.cpp                    # Entry point
│   ├── config/                     # Configuration parsing
│   │   ├── Config.cpp/hpp          # Main config parser
│   │   ├── ServerConfig.cpp/hpp    # Server block configuration
│   │   └── Location.cpp/hpp        # Location block configuration
│   ├── http/                       # HTTP protocol implementation
│   │   ├── HttpRequest.cpp/hpp     # Request parsing
│   │   └── HttpResponse.cpp/hpp    # Response building
│   ├── server/                     # Server core
│   │   ├── Server.cpp/hpp          # Main server loop (poll, accept, read/write)
│   │   ├── Client.cpp/hpp          # Client connection management
│   │   └── Router.cpp/hpp          # Request routing logic
│   ├── cgi/                        # CGI execution
│   │   └── CGI.cpp/hpp             # Fork/exec CGI with environment setup
│   └── utils/                      # Utilities
│       ├── utils.cpp/hpp           # File I/O, string manipulation
│       └── MimeTypes.cpp/hpp       # MIME type mappings
├── config/
│   └── default.conf                # Default configuration file
├── www/                            # Static web content
│   ├── index.html
│   ├── css/
│   └── error_pages/
├── cgi-bin/                        # CGI scripts
│   ├── php/
│   └── py/
└── uploads/                        # Upload directory
```

---

## 🚀 Quick Start

### Build

```bash
make
```

### Run

```bash
./webserv [config_file]
```

Default config file: `config/default.conf`

### Test

```bash
# Simple GET request
curl http://localhost:8080/

# Upload file
curl -X POST -F "file=@test.txt" http://localhost:8080/upload

# CGI script
curl http://localhost:8080/cgi-bin/script.php?name=World
```

---

## ⚙️ Configuration

### Basic Structure

```nginx
server {
    listen 8080;
    server_name localhost;
    client_max_body_size 10M;
    
    error_page 404 /error_pages/404.html;
    
    location / {
        root ./www;
        index index.html;
        allowed_methods GET POST;
        autoindex on;
    }
    
    location /upload {
        root ./www;
        allowed_methods POST DELETE;
        upload_path ./uploads;
    }
    
    location /cgi-bin {
        root ./cgi-bin;
        allowed_methods GET POST;
        cgi_extension .php;
        cgi_path /usr/bin/php-cgi;
    }
    
    location /redirect {
        return https://www.example.com;
    }
}
```

### Directives Reference

#### Server Block

| Directive | Description | Example |
|-----------|-------------|---------|
| `listen` | Port number (1-65535) | `listen 8080;` |
| `server_name` | Virtual host name | `server_name localhost;` |
| `client_max_body_size` | Maximum request body size | `client_max_body_size 10M;` |
| `error_page` | Custom error page | `error_page 404 /404.html;` |

#### Location Block

| Directive | Description | Example |
|-----------|-------------|---------|
| `root` | Root directory | `root ./www;` |
| `index` | Default index files | `index index.html index.htm;` |
| `allowed_methods` | Allowed HTTP methods | `allowed_methods GET POST;` |
| `autoindex` | Enable directory listing | `autoindex on;` |
| `upload_path` | Directory for uploads | `upload_path ./uploads;` |
| `return` | HTTP redirection | `return https://example.com;` |
| `cgi_extension` | CGI file extension | `cgi_extension .php;` |
| `cgi_path` | CGI interpreter path | `cgi_path /usr/bin/php-cgi;` |

---

## 🏗️ Architecture

### Request Flow

```
Client → Server (poll) → accept() → Client
                                      ↓
                                   read request
                                      ↓
                                HttpRequest (parse)
                                      ↓
                                Router (match location)
                                      ↓
                    ┌─────────────────┼─────────────────┐
                    ↓                 ↓                 ↓
              Static File          CGI Script      Directory Listing
                    ↓                 ↓                 ↓
                HttpResponse ← build response ← set headers
                    ↓
                write() → Client
```

### Key Classes

#### **Server**
- Creates listening sockets for each server configuration
- Uses `poll()` for non-blocking I/O multiplexing
- Accepts new clients and manages connections
- Routes requests to appropriate handlers

#### **Client**
- Represents a single client connection
- Manages request buffering and parsing
- Builds and sends HTTP responses
- Tracks connection timeouts

#### **HttpRequest**
- Parses HTTP request line, headers, and body
- Handles chunked transfer encoding
- Parses multipart/form-data for file uploads
- Validates request completeness

#### **HttpResponse**
- Builds HTTP responses with proper status codes
- Serves static files with correct MIME types
- Generates directory listings
- Handles error pages

#### **Router**
- Matches incoming URIs to configured locations
- Validates HTTP methods
- Resolves file paths
- Detects CGI scripts

#### **CGI**
- Forks child process for script execution
- Sets up CGI environment variables (RFC 3875)
- Manages stdin/stdout pipes
- Enforces execution timeouts

---

## 📊 HTTP Status Codes

| Code | Message | Usage |
|------|---------|-------|
| 200 | OK | Successful request |
| 201 | Created | File uploaded successfully |
| 204 | No Content | DELETE successful |
| 301 | Moved Permanently | Permanent redirect |
| 302 | Found | Temporary redirect |
| 400 | Bad Request | Malformed request |
| 403 | Forbidden | Access denied (directory without autoindex) |
| 404 | Not Found | Resource not found |
| 405 | Method Not Allowed | HTTP method not allowed |
| 413 | Payload Too Large | Request body exceeds limit |
| 500 | Internal Server Error | Server-side error |
| 504 | Gateway Timeout | CGI script timeout (5 seconds) |

---

## 🔧 CGI Implementation

### Environment Variables (RFC 3875)

```bash
REQUEST_METHOD      # HTTP method (GET, POST, etc.)
SCRIPT_FILENAME     # Absolute path to the script
SCRIPT_NAME         # URI path to the script
QUERY_STRING        # Query string from URL
SERVER_NAME         # Server hostname
SERVER_PORT         # Server port
SERVER_PROTOCOL     # HTTP version (HTTP/1.1)
GATEWAY_INTERFACE   # CGI version (CGI/1.1)
CONTENT_TYPE        # Request Content-Type header
CONTENT_LENGTH      # Request body length
HTTP_*              # All HTTP headers (e.g., HTTP_USER_AGENT)
REDIRECT_STATUS     # Required by PHP-CGI (security)
```

### Execution Flow

1. Fork child process
2. Setup pipes for stdin/stdout
3. Set environment variables
4. Execute CGI interpreter with script path
5. Write request body to stdin (for POST)
6. Read response from stdout
7. Parse CGI headers (Status, Content-Type)
8. Kill process if timeout exceeded (5 seconds)

### Example CGI Script (PHP)

```php
#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html");
echo "<h1>Hello, " . htmlspecialchars($_GET['name'] ?? 'World') . "!</h1>";
?>
```

---

## 📤 File Upload

### Multipart Form Data Parsing

1. Extract boundary from `Content-Type` header
2. Parse each part with headers and content
3. Extract filename from `Content-Disposition` header
4. Save files to configured upload directory
5. Return `201 Created` on success

### Example Upload Form

```html
<form action="/upload" method="POST" enctype="multipart/form-data">
    <input type="file" name="file">
    <button type="submit">Upload</button>
</form>
```

---

## 🔒 Security

- **Body Size Limit**: Prevents memory exhaustion attacks
- **CGI Timeout**: Prevents infinite loops in scripts (5 seconds)
- **Method Restrictions**: Per-location HTTP method whitelisting
- **Path Validation**: Prevents directory traversal attacks
- **REDIRECT_STATUS**: Required for PHP-CGI to prevent unauthorized execution

---

## ✅ Testing Checklist

- [ ] GET static file
- [ ] GET non-existent file (404)
- [ ] GET directory with autoindex
- [ ] GET directory without autoindex (403)
- [ ] POST file upload
- [ ] POST with body exceeding max size (413)
- [ ] DELETE existing file
- [ ] DELETE non-existent file (404)
- [ ] CGI with GET (query string)
- [ ] CGI with POST (body)
- [ ] CGI timeout (504)
- [ ] HTTP redirection (301)
- [ ] Method not allowed (405)
- [ ] Multiple virtual hosts
- [ ] Chunked transfer encoding
- [ ] Large file download
- [ ] Custom error pages

---

## ⚡ Performance

- **Non-blocking I/O**: Uses `poll()` for efficient connection handling
- **Persistent Connections**: Supports HTTP keep-alive
- **Single-threaded**: Simple, predictable behavior
- **Low Memory Footprint**: Minimal resource usage

---

## 🚫 Limitations (by design)

- HTTP/1.1 only (no HTTP/2)
- No HTTPS/TLS support
- No compression (gzip, deflate)
- No range requests (partial content)
- No caching headers
- Single-threaded (no parallel request processing)
- CGI only (no FastCGI, WSGI, etc.)

---

## 👥 Authors

Built as part of the **42 School** curriculum.

## 📄 License

This is an educational project.

---

**Happy serving! 🚀**