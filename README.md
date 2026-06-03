*This project has been created as part of the 42 curriculum by tignatov, jilustre.*

---

# Webserv 

## Description

**Webserv** is a fully functional HTTP/1.1 web server written in C++17, built from scratch as part of the 42 school curriculum. The goal of the project is to deeply understand how the Hypertext Transfer Protocol works by implementing a server capable of handling real browser requests.

The server is event-driven and non-blocking, using a single `poll()` call to multiplex all I/O operations — including listening for new connections, reading client requests, and writing responses. It parses a configuration file inspired by NGINX's syntax, supports static file serving, file uploads, HTTP redirections, directory listing, and CGI execution.

Key features include:

- Non-blocking I/O multiplexing via `poll()` (or equivalent)
- Custom NGINX-inspired configuration file parser (tokenizer + recursive-descent parser)
- Support for `GET`, `POST`, and `DELETE` HTTP methods
- Static website serving with configurable root and index files
- File uploads with configurable storage location
- Directory listing (`autoindex`)
- HTTP redirections (e.g. `301`)
- CGI execution based on file extension (e.g. `.php` via `php-cgi`, Python scripts)
- Default and custom error pages (404, 500, etc.)
- Configurable `client_max_body_size` per server block
- Multi-server / multi-port support from a single config file
- Accurate HTTP response status codes


---

## Instructions

### Requirements

- A C++17-compatible compiler 
- A POSIX-compatible operating system 
- For CGI: `php-cgi`, `python3`, or another interpreter installed on the system

### Compilation

```bash
make
```

This produces the `webserv` executable. Additional Makefile targets:

```bash
make clean    # Remove object files
make fclean   # Remove object files and the binary
make re       # Full rebuild
```

### Running the server

```bash
./webserv [configuration file]
```

If no configuration file is provided, the server looks for one at a default path.

**Example:**

```bash
./webserv testConfig.conf
```

### Configuration file

The configuration file follows an NGINX-inspired syntax. A minimal example:

```nginx
server {
    listen 127.0.0.1:8080;
    server_name webserv.com;

    client_max_body_size 10M;

    error_page 404 /errors/404.html;
    error_page 500 /errors/500.html;

    location / {
        methods GET;
        root /var/www/site;
        index index.html;
        autoindex on;
    }

    location /upload {
        methods POST;
        root /var/www/upload;
        upload_store /var/www/upload;
    }

    location /cgi-bin {
        methods GET POST;
        root /var/www/cgi-bin;
        cgi .php /usr/bin/php-cgi;
    }

    location /old {
        methods GET;
        return 301 "/new";
    }
}
```

Supported directives per `server` block:

| Directive             | Description                                      |
|-----------------------|--------------------------------------------------|
| `listen`              | Interface and port to bind to                    |
| `server_name`         | Optional name for the virtual host               |
| `client_max_body_size`| Maximum allowed size of client request body      |
| `error_page`          | Custom error page for a given HTTP status code   |

Supported directives per `location` block:

| Directive      | Description                                                  |
|----------------|--------------------------------------------------------------|
| `methods`      | Allowed HTTP methods (`GET`, `POST`, `DELETE`)               |
| `root`         | Root directory for file resolution                           |
| `index`        | Default file served for directory requests                   |
| `autoindex`    | Enable directory listing (`on` / `off`)                      |
| `upload_store` | Directory where uploaded files are stored                    |
| `cgi`          | Map a file extension to a CGI interpreter                    |
| `return`       | HTTP redirection (e.g. `301 /new-url`)                       |

### Testing

A Python test suite is included:

```bash
python3 test_webserv.py
```

You can also test with `curl`

---

## Resources

### HTTP Protocol

- [RFC 7230 — HTTP/1.1: Message Syntax and Routing](https://datatracker.ietf.org/doc/html/rfc7230)
- [RFC 7231 — HTTP/1.1: Semantics and Content](https://datatracker.ietf.org/doc/html/rfc7231)
- [MDN Web Docs — HTTP overview](https://developer.mozilla.org/en-US/docs/Web/HTTP/Overview)
- [MDN Web Docs — HTTP response status codes](https://developer.mozilla.org/en-US/docs/Web/HTTP/Status)

### NGINX Reference

- [NGINX Beginner's Guide](https://nginx.org/en/docs/beginners_guide.html)
- [NGINX `server` block documentation](https://nginx.org/en/docs/http/ngx_http_core_module.html)

### CGI

- [RFC 3875 — The Common Gateway Interface (CGI/1.1)](https://datatracker.ietf.org/doc/html/rfc3875)
- [CGI — Wikipedia](https://en.wikipedia.org/wiki/Common_Gateway_Interface)

### I/O Multiplexing

- [`poll(2)` man page](https://man7.org/linux/man-pages/man2/poll.2.html)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

### AI Usage

AI tools (specifically Claude by Anthropic) were used during this project for the following purposes:

- **Design discussions**: brainstorming the architecture of the event loop
- **Debugging assistance**: analyzing crash scenarios, understanding edge cases in HTTP parsing (chunked bodies, header folding, keep-alive)
- **Test case generation**: suggesting edge cases for HTTP request parsing and CGI output handling.

All code was written, reviewed, and understood by the project authors. AI-generated suggestions were always verified against the RFCs, the NGINX reference behaviour, and our own test suite.
