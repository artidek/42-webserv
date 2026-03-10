# 🚀 42-webserv

A lightweight HTTP server implementation built from scratch in **C++98** as part of the 42 School curriculum.

---

## 📋 Overview

**42-webserv** is a fully functional HTTP/1.0 web server that demonstrates core networking concepts, socket programming, and HTTP protocol implementation. This project showcases clean architecture, efficient request handling, and proper C++ practices without relying on external libraries.

### Key Features
- ✅ **HTTP/1.0 Protocol Support** - Full request/response handling
- ✅ **Socket Programming** - Low-level network socket management
- ✅ **Configuration System** - Customizable server settings via `.conf` files (NGINX-style)
- ✅ **Virtual Hosting** - Multiple sites with separate configurations
- ✅ **Static File Serving** - Efficient file delivery
- ✅ **CGI Support** - Dynamic content generation
- ✅ **Error Handling** - Proper HTTP error codes and responses
- ✅ **C++98 Compliant** - Pure C++ without modern features
- ✅ **Modular Architecture** - Clean separation of concerns

---

## 🛠️ Project Structure

```
42-webserv/
├── src/                    # Source code implementation
├── includes/               # Header files (.hpp)
├── main.cpp               # Entry point
├── Makefile               # Build configuration
├── default.conf           # Default server configuration
├── docs/                  # Documentation
├── tests/                 # Test files
├── utils/                 # Utility functions
├── etc/                   # Configuration examples
│   ├── cgi_bin/          # CGI scripts
│   └── error/            # Error pages
└── README.md              # This file
```

---

## 📦 Requirements

- **Language:** C++98
- **Compiler:** c++ or clang++ with C++98 standard support
- **OS:** Linux/Unix-based systems
- **No external libraries** - Pure C++ implementation

---

## 🚀 Getting Started

### Compilation

```bash
make
```

### Running the Server

```bash
./webserv [path/to/config.conf]
```

Or with default configuration:

```bash
./webserv
```

### Build Commands

```bash
make        # Compile the project
make clean  # Remove object files
make fclean # Remove all generated files
make re     # Recompile from scratch
```

---

## ⚙️ Configuration

The configuration file structure is inspired by **NGINX**, providing a flexible and powerful way to define server behavior. All configuration files use a declarative syntax with blocks and semicolon-terminated directives.

> **Note:** All configuration fields must be defined unless stated otherwise. If `cgi_config` or `error_pages` are not provided, the server will use default settings.

### Host Configuration

A server block can contain multiple host definitions, where each host represents a separate website with its own configuration and routing rules.

Inside each host, the `host_configs` section defines the core networking parameters:

- **addr** – IP address the server listens on
- **ports** – list of ports assigned to the host
- **hostname** – unique site name used for virtual host resolution
- **max_request_body** – maximum allowed size of the request body (in bytes)

#### Example:

```nginx
host_configs
{
    addr: 127.0.0.1;
    ports: [8080, 8081];
    max_request_body: 2000000;
    hostname: localhost;
}
```

---

### Routes

Routes define how URLs map to filesystem resources. Each route specifies:

- **new_root** – directory containing the requested files
- **page** – default file served for the route (e.g., `index.html`)
- **methods** – allowed HTTP methods (GET, POST, DELETE, etc.)
- **success_response** – HTTP status code returned on success
- **redirect** – redirect target or rule applied to the request

`success_response` and `redirect` work together to determine the final behavior of the route. If a redirect is defined, the server responds with the corresponding redirect status code and `Location` header.

#### Example:

```nginx
route /about {
    new_root: tests/static_site/;
    page: about.html;
    success_response: 200;
    methods: [GET];
    redirect: none;
}

route /api {
    new_root: api/;
    page: index.php;
    success_response: 200;
    methods: [GET, POST];
    redirect: none;
}

route /old-page {
    new_root: tests/;
    page: none;
    success_response: 301;
    methods: [GET];
    redirect: /new-page;
}
```

---

### CGI Configuration

The `cgi_config` block controls execution of CGI scripts.

**Key options:**

- **cgi_allowed** – enables or disables CGI execution (`on` / `off`)
- **root** – directory containing CGI scripts
- **default_cgi** – default interpreter used if needed (e.g., `python3`, `perl`)
- **cgi_extensions** – file extensions treated as CGI executables

If this block is not defined, the server automatically applies default CGI settings.

#### Example:

```nginx
cgi_config {
    cgi_allowed: on;
    root: etc/cgi_bin;
    default_cgi: none;
    cgi_extensions: [php, py, cgi]
}
```

---

### Locations

`location` blocks apply rules to specific directories and allow control over:

- **directory_listing** – enable or disable directory browsing
- **upload_enabled** – allow file uploads to this location
- **list_ext** – file extensions allowed for directory listing
- **upload_ext** – file extensions allowed for uploading

#### Example:

```nginx
location tests/uploads/
{
    directory_listing: on;
    upload_enabled: on;
    list_ext: [pdf, txt, doc];
    upload_ext: [pdf, txt, doc];
}

location tests/static/
{
    directory_listing: off;
    upload_enabled: off;
}
```

---

### Error Pages

Hosts can define custom error pages mapped to HTTP status codes:

```nginx
error_pages {
    404: etc/error/404.html;
    500: etc/error/500.html;
    403: etc/error/403.html;
    405: etc/error/405.html;
}
```

If the `error_pages` section is not provided, the server falls back to built-in default error responses.

---

### Complete Configuration Example

```nginx
server {
    host_configs {
        addr: 127.0.0.1;
        ports: [8080, 8081];
        max_request_body: 2000000;
        hostname: localhost;
    }

    route / {
        new_root: www/;
        page: index.html;
        success_response: 200;
        methods: [GET];
        redirect: none;
    }

    route /api {
        new_root: api/;
        page: index.php;
        success_response: 200;
        methods: [GET, POST];
        redirect: none;
    }

    route /uploads {
        new_root: uploads/;
        page: none;
        success_response: 200;
        methods: [GET, POST];
        redirect: none;
    }

    cgi_config {
        cgi_allowed: on;
        root: etc/cgi_bin;
        default_cgi: python3;
        cgi_extensions: [php, py, cgi]
    }

    location uploads/
    {
        directory_listing: on;
        upload_enabled: on;
        list_ext: [pdf, txt, jpg, png];
        upload_ext: [pdf, txt, jpg, png];
    }

    error_pages {
        404: etc/error/404.html;
        500: etc/error/500.html;
        403: etc/error/403.html;
    }
}
```

---

## 📡 Supported HTTP Methods

- `GET` - Retrieve resources
- `POST` - Submit data and process forms
- `DELETE` - Remove resources
- `HEAD` - Retrieve metadata without body

---

## 🔧 Technical Implementation

### Low-Level Networking Architecture

This project was built using **low-level Unix networking APIs** to fully understand how web servers operate internally:

- **TCP Sockets** - Raw socket programming for client-server communication
- **Event-Driven Architecture** - Non-blocking I/O with **epoll** for efficient multiplexing of multiple concurrent connections
- **File Descriptor Management** - Careful handling of connection states, socket cleanup, and resource optimization

### Performance Optimizations

To optimize performance and memory usage under load:

- **Pre-sized `std::vector<char>` Buffers** - Fixed-size buffers to control memory allocations during `recv()` operations
- **Reduced Reallocations** - Efficient incremental request parsing without unnecessary buffer resizing
- **Scalable Connection Handling** - Event-driven model supports thousands of concurrent connections with minimal overhead
- **Zero-Copy Where Possible** - Direct file serving to minimize data movement

### Core Components Implemented

- **Custom HTTP Parser** - RFC-compliant request parsing without external libraries, handling headers, body, and edge cases
- **Dynamic Routing Engine** - Location-based routing with configuration matching and method restrictions
- **Virtual Host Resolution** - Multiple sites with separate configurations
- **CGI Integration** - Dynamic response generation alongside static file serving
- **Connection State Management** - Proper handling of keep-alive connections, timeouts, and graceful disconnections
- **Error Recovery** - Comprehensive error handling with proper HTTP status codes and error pages

---

## 🧪 Testing

Run the server:
```bash
./webserv
```

Test in another terminal with curl:
```bash
# Basic GET request
curl http://localhost:8080/

# POST request
curl -X POST -d "data=value" http://localhost:8080/api

# DELETE request
curl -X DELETE http://localhost:8080/resource

# Upload file
curl -X POST -F "file=@filename.txt" http://localhost:8080/uploads

# List directory contents
curl http://localhost:8080/uploads
```

---

## 🏗️ Architecture Highlights

### Design Patterns
- **Non-blocking I/O** - Efficient event-driven model with epoll
- **Configuration-Driven Behavior** - Flexible server customization via config files
- **Clean Separation of Concerns** - Distinct modules for parsing, routing, and serving
- **Resource Management** - RAII principles for safe cleanup
- **Virtual Hosting** - Multiple websites on single server instance

### Key Algorithms
- **HTTP Request Parsing** - State machine approach for robust parsing
- **Event Multiplexing** - Efficient handling of thousands of file descriptors
- **Buffer Management** - Pre-allocated pools to reduce allocations
- **Configuration Parsing** - Custom parser for NGINX-style config syntax

---

## 📝 Implementation Notes

- This is a learning project for the 42 School curriculum
- Focuses on understanding HTTP protocol and network programming fundamentals
- Pure C++98 implementation - no third-party libraries or boost
- Production-grade error handling and edge case management
- Handles concurrent connections efficiently without threading
- Supports virtual hosting through multiple server blocks
- Configuration system inspired by NGINX for familiarity and power

---

## 👤 Author

**artidek** - [GitHub Profile](https://github.com/artidek)

---

## 📚 Resources

- [HTTP/1.1 Specification (RFC 7230)](https://tools.ietf.org/html/rfc7230)
- [HTTP/1.0 Specification (RFC 1945)](https://tools.ietf.org/html/rfc1945)
- [Socket Programming Guide](https://man7.org/linux/man-pages/man7/socket.7.html)
- [epoll Manual](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [CGI Specification](https://tools.ietf.org/html/rfc3875)
- [NGINX Configuration Documentation](https://nginx.org/en/docs/)
- [42 School](https://www.42.fr)

---

## 📄 License

This project is part of the 42 School curriculum.

---
