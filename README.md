*This project has been created as part of the 42 curriculum by eschwart, gdosch and lmarck.*

# Webserv - HTTP Server

A lightweight HTTP/1.1 web server implemented in C++98, designed to handle static files, CGI execution, file uploads, and multiple ports (with optional virtual host support).

---

## Description

**Webserv** is a custom HTTP server built from scratch in C++98 as part of the 42 School curriculum. The project implements HTTP/1.1 protocol features (with HTTP/1.0 compatibility as suggested by the subject) and demonstrates deep understanding of network programming, socket I/O, non-blocking I/O, and event-driven architecture.

The server implements core HTTP/1.1 features including:
- Full HTTP request parsing (request line, headers, body)
- Support for GET, POST, DELETE, HEAD, and OPTIONS methods
- Static file serving with automatic MIME type detection
- CGI script execution (PHP, Python) with proper environment setup
- File upload handling (multipart/form-data)
- Multiple server blocks listening on different ports
- Virtual host support (optional feature: server_name matching)
- Location-based routing with method restrictions
- Custom error pages and HTTP redirections
- Non-blocking I/O multiplexing using `poll()`

The configuration system uses an Nginx-like syntax, allowing flexible server and location block definitions with method restrictions, body size limits, and CGI configuration.

---

## Instructions

### Prerequisites

- **Operating System**: Linux (tested on Ubuntu 20.04+)
- **Compiler**: g++ or clang++ with C++98 support
- **Make**: GNU Make
- **Optional**: PHP-CGI and Python for CGI script execution

### Compilation

```bash
# Clone the repository
git clone <repository_url>
cd webserv

# Build the server
make

# Clean build artifacts
make clean

# Full rebuild
make re
```

### Execution

```bash
# Run with default configuration
./webserv

# Run with custom configuration file
./webserv config/custom.conf
```

The server will start listening on the ports specified in the configuration file (default: 8080).

### Configuration

Create a configuration file following this syntax:

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

    location /cgi-bin {
        root ./cgi-bin;
        allowed_methods GET POST;
        cgi_extension .php;
        cgi_path /usr/bin/php-cgi;
    }
}
```

**Key Directives:**
- `listen`: Port number (1-65535)
- `server_name`: Virtual host identifier
- `client_max_body_size`: Maximum request body size (e.g., 10M, 1G)
- `root`: Document root directory
- `index`: Default index files
- `allowed_methods`: Whitelist of HTTP methods (GET, POST, DELETE, HEAD, OPTIONS)
- `autoindex`: Enable directory listing (on/off)
- `cgi_extension`: File extension for CGI scripts
- `cgi_path`: Path to CGI interpreter
- `return`: HTTP redirection URL

### Testing

```bash
# Run automated test suite
make test

# Run evaluation test suite (downloads official 42 tester)
make eval

# Manual testing with curl
curl http://localhost:8080/
curl -X POST -F "file=@test.txt" http://localhost:8080/uploads
curl http://localhost:8080/cgi-bin/php/script.php?name=World
```

### Stopping the Server

Press `Ctrl+C` or use:
```bash

# kill any process called "webserv"
make kill

## Resources

### HTTP Protocol Documentation
- [RFC 7230 - HTTP/1.1: Message Syntax and Routing](https://datatracker.ietf.org/doc/html/rfc7230)
- [RFC 7231 - HTTP/1.1: Semantics and Content](https://datatracker.ietf.org/doc/html/rfc7231)
- [RFC 3875 - The Common Gateway Interface (CGI) Version 1.1](https://datatracker.ietf.org/doc/html/rfc3875)
- [MDN Web Docs - HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)

### Implementation References
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [NGINX Configuration Reference](https://nginx.org/en/docs/)
- [HTTP Made Really Easy - A Practical Guide to HTTP](https://www.jmarshall.com/easy/http/)

### Technical Articles
- [Understanding Non-blocking I/O with poll()](https://man7.org/linux/man-pages/man2/poll.2.html)
- [How to Parse HTTP Requests](https://www.w3.org/Protocols/rfc2616/rfc2616-sec5.html)
- [Multipart/Form-Data Format](https://www.w3.org/TR/html401/interact/forms.html#h-17.13.4)

### AI Usage

**AI tools (GitHub Copilot, Claude, ChatGPT) were used for the following tasks:**

1. **Documentation and Comments**
   - Generated Doxygen-style documentation blocks for class methods
   - Improved inline comments for complex algorithms
   - Formatted README sections and configuration examples

2. **Code Review and Debugging**
   - Identified potential edge cases in HTTP parsing logic
   - Suggested improvements for error handling patterns
   - Helped debug CGI pipe management and timeout handling

3. **Research and Learning**
   - Clarified RFC specifications and HTTP protocol details
   - Explained POSIX system call behavior (poll, fork, exec, pipes)
   - Provided examples of multipart/form-data parsing algorithms

4. **Configuration and Testing**
   - Assisted in creating Makefile rules for automated testing
   - Generated test configuration files for various scenarios
   - Helped design test cases for edge conditions
   - help to design the website used for testing jss/html
   - python script the test CGI implementation

**Core implementation** (HTTP parsing, routing logic, CGI execution, socket management, configuration parsing) was **written by the team** with manual design and implementation. AI was used as a **reference tool** and **documentation assistant**, not as a code generator for the main logic.

---

## Features

### Core HTTP/1.1 Support
- ✅ Multiple HTTP methods: GET, POST, DELETE, HEAD, OPTIONS
- ✅ Full request parsing: request line, headers, body (chunked and content-length)
- ✅ Chunked request un-chunking for CGI (CGI receives plain body)
- ✅ Response building with proper status codes and headers
- ✅ MIME type detection based on file extensions

### Advanced Features
- ✅ **CGI Execution**: Fork/exec PHP and Python scripts with RFC 3875 environment
- ✅ **File Upload**: Multipart/form-data parsing and file storage
- ✅ **Directory Listing**: Automatic HTML directory index generation
- ✅ **HTTP Redirections**: 301 Moved Permanently and 302 Found
- ✅ **Custom Error Pages**: Per-status-code error page configuration
- ✅ **Body Size Limiting**: Configurable max request body size (DoS protection)
- ✅ **Multiple Ports**: Server blocks listening on different ports
- ✅ **Virtual Hosts** (Bonus): Optional server_name matching via Host header
- ✅ **Location Routing**: URL path-based routing with method restrictions

### Configuration System
- ✅ Nginx-like configuration syntax
- ✅ Multiple server blocks on different ports
- ✅ Optional virtual host support (server_name matching)
- ✅ Location blocks with path-based routing
- ✅ Per-location method whitelisting
- ✅ Configurable CGI interpreters per location
- ✅ Upload directory configuration
- ✅ Error page customization

---

## Architecture

### Project Structure

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
│   │   ├── Server.cpp/hpp          # Main server loop (poll, accept, I/O)
│   │   ├── Client.cpp/hpp          # Client connection management
│   │   └── Router.cpp/hpp          # Request routing logic
│   ├── cgi/                        # CGI execution
│   │   └── CGI.cpp/hpp             # Fork/exec with environment setup
│   └── utils/                      # Utilities
│       ├── utils.cpp/hpp           # File I/O, string manipulation
│       ├── MimeTypes.cpp/hpp       # MIME type mappings
│       └── Logger.cpp/hpp          # Logging system
├── config/
│   └── default.conf                # Default configuration
├── www/                            # Static web content
├── cgi-bin/                        # CGI scripts
└── uploads/                        # Upload directory
```

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

### Key Components

- **Server**: Manages listening sockets, accepts connections, multiplexes I/O with `poll()`
- **Client**: Represents a single connection, buffers requests, builds responses
- **HttpRequest**: Parses HTTP requests, handles chunked encoding, validates completeness
- **HttpResponse**: Builds responses, serves static files, generates directory listings
- **Router**: Matches URIs to locations, validates methods, resolves file paths
- **CGI**: Forks child processes, sets environment variables, enforces timeouts
- **Config**: Parses configuration files, validates directives, stores server settings

---

## HTTP Status Codes

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
| 501 | Not Implemented | Feature not supported |
| 504 | Gateway Timeout | CGI script timeout (90 seconds) |

---

## Security Features

- **Body Size Limit**: Prevents memory exhaustion attacks
- **CGI Timeout**: 90-second timeout prevents infinite loops
- **Method Restrictions**: Per-location HTTP method whitelisting
- **Path Validation**: Prevents directory traversal attacks
- **REDIRECT_STATUS**: Required for PHP-CGI security
- **Non-blocking I/O**: Prevents server blocking on slow clients

---

## Limitations (By Design / Subject Scope)

- HTTP/1.1 implementation (no HTTP/2 or HTTP/3)
- No HTTPS/TLS support (plain HTTP only)
- No compression (gzip, deflate, brotli)
- No range requests (partial content / HTTP 206)
- No caching headers (Cache-Control, ETag)
- Single-threaded event loop (non-blocking I/O with poll())
- CGI execution only (no FastCGI, WSGI, ASGI)

---

## Authors

- **eschwart** - [42 Mulhouse]
- **gdosch** - [42 Mulhouse]
- **lmarck** - [42 Mulhouse]

---

## License

This is an educational project created for the 42 School curriculum.

---

**Happy serving! 🚀**
