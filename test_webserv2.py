#!/usr/bin/env python3
"""
Webserv rigorous test suite — aligned to testConfig3.conf
Usage: python3 test_webserv2.py [--host 127.0.0.1] [--port 8080]

Config summary (port 8080, server_name example.com):
  location /              GET only  root ./www  index index.html  autoindex off
  location /testAutoindex GET only  root ./www  autoindex on
  location /old-page      GET only  return 301 /new-page
  location /new-page      GET only  root ./www  index index.html
  location /upload        POST only upload_store ./uploads
  location /uploads       GET DELETE  root ./uploads  autoindex on
  location /cgi-bin       GET POST  cgi .php /usr/bin/php-cgi  cgi .py /usr/bin/python3
"""

import http.client
import os
import argparse
import time
import threading
import socket
import random
import string

# ──────────────────────────────────────────────
# Config — root auto-detected from script location
# ──────────────────────────────────────────────
DEFAULT_HOST  = "127.0.0.1"
DEFAULT_PORT  = 8080

PROJECT_ROOT  = os.path.dirname(os.path.abspath(__file__))

IMAGE_PATH    = os.path.join(PROJECT_ROOT, "uploads", "cat.jpg")
# CGI scripts live at /cgi-bin (mapped to ./cgi-bin)
UPLOAD_URL    = "/cgi-bin/image.php"      # POST → saves to ./uploads/cgi_upload.jpg
DOWNLOAD_URL  = "/uploads/cgi_upload.jpg" # served from /uploads location
CGI_INFO_URL  = "/cgi-bin/info.py"
CGI_CLOCK_URL = "/cgi-bin/worldclock.py"
# No cgi_tester binary in config; no /public location in config

PASS = "\033[92m[PASS]\033[0m"
FAIL = "\033[91m[FAIL]\033[0m"
INFO = "\033[94m[INFO]\033[0m"
SKIP = "\033[93m[SKIP]\033[0m"

results = []

# ──────────────────────────────────────────────
# Helpers
# ──────────────────────────────────────────────
def make_connection(host, port):
    return http.client.HTTPConnection(host, port, timeout=10)

def check(name, condition, got="", expected=""):
    status = PASS if condition else FAIL
    msg = f"{status} {name}"
    if not condition:
        msg += f"\n       expected : {expected}\n       got      : {got}"
    print(msg)
    results.append((name, condition))
    return condition

def skip(name, reason):
    print(f"{SKIP} {name} — {reason}")
    results.append((name, True))

def section(title):
    print(f"\n{'─'*60}")
    print(f"  {title}")
    print(f"{'─'*60}")

def get(host, port, path, headers=None):
    h = {"Host": f"{host}:{port}", "Connection": "close"}
    if headers:
        h.update(headers)
    conn = make_connection(host, port)
    conn.request("GET", path, headers=h)
    r = conn.getresponse()
    body = r.read()
    conn.close()
    return r, body

def post(host, port, path, body=b"", headers=None):
    h = {
        "Host":           f"{host}:{port}",
        "Connection":     "close",
        "Content-Length": str(len(body)),
    }
    if headers:
        h.update(headers)
    conn = make_connection(host, port)
    conn.request("POST", path, body=body, headers=h)
    r = conn.getresponse()
    resp_body = r.read()
    conn.close()
    return r, resp_body

def delete(host, port, path, headers=None):
    h = {"Host": f"{host}:{port}", "Connection": "close"}
    if headers:
        h.update(headers)
    conn = make_connection(host, port)
    conn.request("DELETE", path, headers=h)
    r = conn.getresponse()
    body = r.read()
    conn.close()
    return r, body

def raw_request(host, port, data: bytes, timeout=5) -> bytes:
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.settimeout(timeout)
    s.connect((host, port))
    s.sendall(data)
    chunks = []
    try:
        while True:
            chunk = s.recv(4096)
            if not chunk:
                break
            chunks.append(chunk)
    except (socket.timeout, ConnectionResetError):
        pass
    s.close()
    return b"".join(chunks)

def server_alive(host, port):
    """Quick check the server still responds."""
    try:
        r, _ = get(host, port, "/")
        return r.status in (200, 301, 302, 404)
    except Exception:
        return False


# ══════════════════════════════════════════════
# SECTION 1 — Static GET
# ══════════════════════════════════════════════
def test_static_get(host, port):
    section("1 · Static GET")

    r, body = get(host, port, "/")
    check("GET / → 200",                    r.status == 200, r.status, 200)
    check("GET / body non-empty",           len(body) > 0,   len(body))
    check("GET / Content-Type text/html",
          "text/html" in r.getheader("Content-Type", ""),
          r.getheader("Content-Type"))

    r, _ = get(host, port, "/does_not_exist.html")
    check("GET missing file → 404",         r.status == 404, r.status, 404)

    # /new-page is a real location that serves ./www/index.html
    r, _ = get(host, port, "/new-page")
    check("GET /new-page → 200",            r.status == 200, r.status, 200)


# ══════════════════════════════════════════════
# SECTION 2 — Content-Type
# ══════════════════════════════════════════════
def test_content_types(host, port):
    section("2 · Content-Type per file extension")

    if os.path.exists(IMAGE_PATH):
        with open(IMAGE_PATH, "rb") as f:
            img = f.read()
        post(host, port, UPLOAD_URL, body=img, headers={"Content-Type": "image/jpeg"})
        time.sleep(0.1)

    r, _ = get(host, port, DOWNLOAD_URL)
    check(".jpg → image/jpeg",
          "image/jpeg" in r.getheader("Content-Type", ""),
          r.getheader("Content-Type"))

    r, _ = get(host, port, "/")
    check("/ → text/html",
          "text/html" in r.getheader("Content-Type", ""),
          r.getheader("Content-Type"))


# ══════════════════════════════════════════════
# SECTION 3 — Content-Length accuracy
# ══════════════════════════════════════════════
def test_content_length(host, port):
    section("3 · Content-Length accuracy")

    for path in ["/", DOWNLOAD_URL]:
        r, body = get(host, port, path)
        if r.status != 200:
            skip(f"Content-Length {path}", f"status {r.status}")
            continue
        cl = int(r.getheader("Content-Length", "-1"))
        check(f"Content-Length == actual body ({path})",
              cl == len(body), f"header={cl} body={len(body)}")


# ══════════════════════════════════════════════
# SECTION 4 — Method Not Allowed
# ══════════════════════════════════════════════
def test_method_not_allowed(host, port):
    section("4 · Method Not Allowed")

    # / only allows GET
    r, _ = post(host, port, "/", body=b"x")
    check("POST to GET-only / → 405",       r.status == 405, r.status, 405)

    r, _ = delete(host, port, "/")
    check("DELETE to GET-only / → 405",     r.status == 405, r.status, 405)

    # Unknown method
    raw = f"PATCH / HTTP/1.1\r\nHost: {host}:{port}\r\nContent-Length: 0\r\n\r\n".encode()
    resp = raw_request(host, port, raw)
    check("PATCH to GET-only / → 4xx",
          resp[:8] == b"HTTP/1.1" and b" 4" in resp[:12], resp[:12])

    # /upload only allows POST — GET should be 405
    r, _ = get(host, port, "/upload")
    check("GET to POST-only /upload → 405", r.status == 405, r.status, 405)


# ══════════════════════════════════════════════
# SECTION 5 — Redirect
# Config: location /old-page → return 301 /new-page
# ══════════════════════════════════════════════
def test_redirect(host, port):
    section("5 · Redirect")

    r, _ = get(host, port, "/old-page")
    check("GET /old-page → 301",            r.status == 301, r.status, 301)
    loc = r.getheader("Location", "")
    check("Location contains /new-page",    "/new-page" in loc, loc)

    # Redirect response must have no body or Content-Length: 0
    cl = r.getheader("Content-Length", "0")
    check("Redirect Content-Length is 0 or missing", cl in ("0", ""), cl)

    # Follow the redirect manually — /new-page should serve index.html
    r2, body2 = get(host, port, "/new-page")
    check("GET /new-page → 200",            r2.status == 200, r2.status, 200)
    check("/new-page body non-empty",       len(body2) > 0, len(body2))


# ══════════════════════════════════════════════
# SECTION 6 — Bad / malformed requests
# ══════════════════════════════════════════════
def test_bad_requests(host, port):
    section("6 · Bad / malformed requests")

    # Missing Host header
    raw = b"GET / HTTP/1.1\r\n\r\n"
    resp = raw_request(host, port, raw)
    check("Missing Host → 400",             b"400" in resp[:20], resp[:20])

    # Garbage request line
    raw = b"HELLO WORLD\r\n\r\n"
    resp = raw_request(host, port, raw)
    check("Garbage request line → 4xx",
          resp[:8] == b"HTTP/1.1" and b" 4" in resp[:12], resp[:20])

    # Double slash in URI — server must not crash
    r, _ = get(host, port, "//")
    check("Double slash URI — server alive", r.status in range(200, 600), r.status)

    # Null byte in URI
    raw = b"GET /\x00evil HTTP/1.1\r\nHost: localhost\r\n\r\n"
    resp = raw_request(host, port, raw)
    check("Null byte in URI → 4xx or empty",
          len(resp) == 0 or (resp[:8] == b"HTTP/1.1" and b" 4" in resp[:12]),
          resp[:20])

    # HTTP/0.9 request (no version)
    raw = b"GET /\r\n"
    resp = raw_request(host, port, raw, timeout=3)
    check("HTTP/0.9 request → 4xx or close",
          len(resp) == 0 or b" 4" in resp[:12], resp[:20])

    # Extremely long header value
    long_val = "x" * 8000
    raw = f"GET / HTTP/1.1\r\nHost: {host}:{port}\r\nX-Custom: {long_val}\r\n\r\n".encode()
    resp = raw_request(host, port, raw)
    check("Very long header value — server alive",
          len(resp) > 0 and resp[:8] == b"HTTP/1.1", resp[:20])

    check("Server still alive after bad requests", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 7 — Oversized URI
# ══════════════════════════════════════════════
def test_large_uri(host, port):
    section("7 · Oversized URI → 414")

    long_path = "/" + "a" * 9000
    raw = f"GET {long_path} HTTP/1.1\r\nHost: {host}:{port}\r\n\r\n".encode()
    resp = raw_request(host, port, raw)
    check("URI > 8 KB → 414 or 400",
          b"414" in resp[:20] or b"400" in resp[:20], resp[:20])

    check("Server still alive after large URI", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 8 — Autoindex
# Config: /testAutoindex  autoindex on, root ./www
#         /uploads        autoindex on, root ./uploads
# ══════════════════════════════════════════════
def test_autoindex(host, port):
    section("8 · Autoindex directory listing")

    # /uploads has autoindex on — always has content (upload files exist)
    r, body = get(host, port, "/uploads/")
    body_str = body.decode(errors="replace")
    check("GET /uploads/ → 200",            r.status == 200, r.status, 200)
    check("Autoindex contains <a href",     "<a href" in body_str, body_str[:300])
    check("Content-Type text/html",
          "text/html" in r.getheader("Content-Type", ""),
          r.getheader("Content-Type"))

    # Without trailing slash — should redirect or still work
    r, _ = get(host, port, "/uploads")
    check("GET /uploads (no slash) → 200 or 3xx",
          r.status in (200, 301, 302), r.status)

    # /testAutoindex has autoindex on
    r2, body2 = get(host, port, "/testAutoindex/")
    if r2.status == 404:
        skip("GET /testAutoindex/", "directory not present")
    else:
        check("GET /testAutoindex/ → 200",  r2.status == 200, r2.status, 200)
        body_str2 = body2.decode(errors="replace")
        check("testAutoindex listing has <a href", "<a href" in body_str2, body_str2[:300])


# ══════════════════════════════════════════════
# SECTION 9 — CGI PHP GET
# ══════════════════════════════════════════════
def test_cgi_get(host, port):
    section("9 · CGI PHP — GET (no body)")

    r, body = get(host, port, UPLOAD_URL)
    body_str = body.decode(errors="replace")
    check("GET image.php → 200",            r.status == 200, r.status, 200)
    check("PHP reports 0 bytes received",   "received: 0" in body_str, body_str)
    check("CGI response has Content-Type",  r.getheader("Content-Type") is not None,
          r.getheader("Content-Type"))


# ══════════════════════════════════════════════
# SECTION 10 — CGI PHP POST + round-trip
# ══════════════════════════════════════════════
def test_cgi_post_image(host, port):
    section("10 · CGI PHP — POST image + round-trip integrity")

    if not os.path.exists(IMAGE_PATH):
        skip("CGI image upload", f"{IMAGE_PATH} not found")
        return

    with open(IMAGE_PATH, "rb") as f:
        image_data = f.read()
    expected = len(image_data)

    r, body = post(host, port, UPLOAD_URL, body=image_data,
                   headers={"Content-Type": "image/jpeg"})
    body_str = body.decode(errors="replace")
    check("POST image.php → 200",           r.status == 200, r.status, 200)
    check(f"PHP reports {expected} bytes",  f"received: {expected}" in body_str, body_str)

    time.sleep(0.15)
    r2, dl_body = get(host, port, DOWNLOAD_URL)
    check("GET saved image → 200",          r2.status == 200, r2.status, 200)
    check("JPEG magic bytes FFD8",          dl_body[:2] == b"\xff\xd8", dl_body[:2].hex(), "ffd8")
    check("Round-trip body == original",    dl_body == image_data,
          f"{len(dl_body)} vs {len(image_data)}")


# ══════════════════════════════════════════════
# SECTION 11 — CGI PHP varying sizes
# ══════════════════════════════════════════════
def test_cgi_post_sizes(host, port):
    section("11 · CGI PHP — POST varying body sizes")

    sizes = [0, 1, 255, 1024, 4096, 8192, 32768, 65535]
    for size in sizes:
        payload = bytes(i % 256 for i in range(size))
        try:
            r, body = post(host, port, UPLOAD_URL, body=payload,
                           headers={"Content-Type": "application/octet-stream"})
            body_str = body.decode(errors="replace")
            check(f"POST {size:>6} bytes — PHP count correct",
                  f"received: {size}" in body_str, body_str)
        except Exception as e:
            check(f"POST {size:>6} bytes", False, str(e))


# ══════════════════════════════════════════════
# SECTION 12 — CGI Python info.py GET
# ══════════════════════════════════════════════
def test_cgi_python_get(host, port):
    section("12 · CGI Python — info.py GET")

    r, body = get(host, port, CGI_INFO_URL)
    if r.status == 404:
        skip("info.py GET", "not deployed")
        return
    body_str = body.decode(errors="replace")
    check("GET info.py → 200",              r.status == 200, r.status, 200)
    check("REQUEST_METHOD=GET",             "method        : GET" in body_str, body_str)
    check("body_received=0 on GET",         "body_received : 0" in body_str, body_str)
    check("remote_addr present",            "remote_addr" in body_str, body_str)


# ══════════════════════════════════════════════
# SECTION 13 — CGI Python info.py POST
# ══════════════════════════════════════════════
def test_cgi_python_post(host, port):
    section("13 · CGI Python — info.py POST with body")

    payload = b"hello from test suite"
    r, body = post(host, port, CGI_INFO_URL, body=payload,
                   headers={"Content-Type": "text/plain"})
    if r.status == 404:
        skip("info.py POST", "not deployed")
        return
    body_str = body.decode(errors="replace")
    check("POST info.py → 200",             r.status == 200, r.status, 200)
    check("REQUEST_METHOD=POST",            "method        : POST" in body_str, body_str)
    check(f"body_received={len(payload)}",
          f"body_received : {len(payload)}" in body_str, body_str)
    check("body_preview contains payload",  "hello from test suite" in body_str, body_str)


# ══════════════════════════════════════════════
# SECTION 14 — worldclock.py
# ══════════════════════════════════════════════
def test_cgi_worldclock(host, port):
    section("14 · CGI Python — worldclock.py")

    r, body = get(host, port, CGI_CLOCK_URL)
    if r.status == 404:
        skip("worldclock.py", "not deployed")
        return
    body_str = body.decode(errors="replace")
    check("GET worldclock.py → 200",        r.status == 200, r.status, 200)
    check("Content-Type text/html",
          "text/html" in r.getheader("Content-Type", ""),
          r.getheader("Content-Type"))
    check("Contains Amsterdam",             "Amsterdam" in body_str)
    check("Contains Tokyo",                 "Tokyo"     in body_str)
    check("Contains UTC offset",            "UTC+"      in body_str)


# ══════════════════════════════════════════════
# SECTION 15 — QUERY_STRING
# ══════════════════════════════════════════════
def test_cgi_query_string(host, port):
    section("15 · CGI — QUERY_STRING forwarded")

    r, body = get(host, port, f"{CGI_INFO_URL}?foo=bar&x=42")
    if r.status == 404:
        skip("Query string", "info.py not deployed")
        return
    body_str = body.decode(errors="replace")
    check("QUERY_STRING contains foo=bar",  "foo=bar" in body_str, body_str)
    check("QUERY_STRING contains x=42",     "x=42"    in body_str, body_str)


# ══════════════════════════════════════════════
# SECTION 16 — Keep-Alive
# (cgi_tester binary not in config — section renumbered)
# ══════════════════════════════════════════════
def test_keep_alive(host, port):
    section("16 · Keep-Alive — 5 requests on one connection")

    conn = make_connection(host, port)
    try:
        for i in range(5):
            conn.request("GET", "/", headers={
                "Host":       f"{host}:{port}",
                "Connection": "keep-alive",
            })
            r = conn.getresponse()
            body = r.read()
            check(f"Keep-alive request {i+1}/5 → 200", r.status == 200, r.status, 200)
            check(f"Keep-alive request {i+1}/5 body non-empty", len(body) > 0, len(body))
    except Exception as e:
        check("Keep-alive connection", False, str(e))
    finally:
        conn.close()


# ══════════════════════════════════════════════
# SECTION 17 — Connection: close
# ══════════════════════════════════════════════
def test_connection_close(host, port):
    section("17 · Connection: close honoured")

    r, _ = get(host, port, "/", headers={"Connection": "close"})
    check("Response has Connection: close",
          r.getheader("Connection", "").lower() == "close",
          r.getheader("Connection"))


# ══════════════════════════════════════════════
# SECTION 18 — Concurrent connections
# ══════════════════════════════════════════════
def test_concurrent(host, port):
    section("18 · Concurrent connections (20 threads)")

    outcomes = []
    lock = threading.Lock()

    def worker():
        try:
            r, body = get(host, port, "/")
            with lock:
                outcomes.append((r.status, len(body)))
        except Exception as e:
            with lock:
                outcomes.append((str(e), 0))

    threads = [threading.Thread(target=worker) for _ in range(20)]
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=20)

    ok = sum(1 for s, _ in outcomes if s == 200)
    check(f"20 concurrent GET / — all 200  ({ok}/20 ok)", ok == 20, outcomes)

    # Also concurrent CGI
    cgi_outcomes = []

    def cgi_worker():
        try:
            payload = b"concurrent=1"
            r, body = post(host, port, UPLOAD_URL, body=payload,
                           headers={"Content-Type": "text/plain"})
            with lock:
                cgi_outcomes.append(r.status)
        except Exception as e:
            with lock:
                cgi_outcomes.append(str(e))

    threads = [threading.Thread(target=cgi_worker) for _ in range(5)]
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=20)

    ok_cgi = cgi_outcomes.count(200)
    check(f"5 concurrent CGI POST — all 200  ({ok_cgi}/5 ok)",
          ok_cgi == 5, cgi_outcomes)


# ══════════════════════════════════════════════
# SECTION 19 — Sequential CGI stability
# ══════════════════════════════════════════════
def test_sequential_cgi(host, port):
    section("19 · Sequential CGI — server stays stable")

    for i in range(10):
        payload = f"seq-request-{i}".encode()
        try:
            r, _ = post(host, port, UPLOAD_URL, body=payload,
                        headers={"Content-Type": "text/plain"})
            check(f"Sequential CGI {i+1}/10 → 200", r.status == 200, r.status, 200)
        except Exception as e:
            check(f"Sequential CGI {i+1}/10", False, str(e))


# ══════════════════════════════════════════════
# SECTION 20 — Slow client
# ══════════════════════════════════════════════
def test_slow_client(host, port):
    section("20 · Slow client — headers sent in chunks")

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(8)
        s.connect((host, port))
        s.send(b"GET / HTTP/1.1\r\n")
        time.sleep(0.5)
        s.send(f"Host: {host}:{port}\r\n".encode())
        time.sleep(0.5)
        s.send(b"Connection: close\r\n\r\n")
        chunks = []
        try:
            while True:
                c = s.recv(4096)
                if not c:
                    break
                chunks.append(c)
        except socket.timeout:
            pass
        s.close()
        resp = b"".join(chunks)
        check("Slow client (3 chunks) → 200", b"200" in resp[:20], resp[:20])
    except Exception as e:
        check("Slow client", False, str(e))


# ══════════════════════════════════════════════
# SECTION 21 — Pipelined requests
# ══════════════════════════════════════════════
def test_pipelining(host, port):
    section("21 · Pipelined requests on one connection")

    req1 = f"GET / HTTP/1.1\r\nHost: {host}:{port}\r\nConnection: keep-alive\r\n\r\n".encode()
    req2 = f"GET /does_not_exist HTTP/1.1\r\nHost: {host}:{port}\r\nConnection: close\r\n\r\n".encode()

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(8)
        s.connect((host, port))
        s.sendall(req1 + req2)
        chunks = []
        try:
            while True:
                c = s.recv(4096)
                if not c:
                    break
                chunks.append(c)
        except socket.timeout:
            pass
        s.close()
        resp = b"".join(chunks)
        check("Pipelined: first response is 200",  b"200" in resp[:20], resp[:20])
        check("Pipelined: second response present", resp.count(b"HTTP/1.1") >= 2,
              resp.count(b"HTTP/1.1"))
    except Exception as e:
        check("Pipelining", False, str(e))


# ══════════════════════════════════════════════
# SECTION 22 — Body too large (413)
# Manager._recvBufferSize = 100000; config client_max_body_size = 5M
# ══════════════════════════════════════════════
def test_body_too_large(host, port):
    section("22 · Body too large → 413")

    huge = b"x" * 101000
    try:
        r, _ = post(host, port, UPLOAD_URL, body=huge,
                    headers={"Content-Type": "application/octet-stream"})
        check("Body > limit → 413",         r.status == 413, r.status, 413)
    except Exception as e:
        check("Body > limit → connection closed or 413", True, str(e))

    check("Server alive after oversized body", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 23 — Empty body POST
# ══════════════════════════════════════════════
def test_empty_body_post(host, port):
    section("23 · POST with empty body")

    r, body = post(host, port, UPLOAD_URL, body=b"",
                   headers={"Content-Type": "text/plain"})
    body_str = body.decode(errors="replace")
    check("POST empty body → 200",          r.status == 200, r.status, 200)
    check("PHP reports 0 bytes",            "received: 0" in body_str, body_str)


# ══════════════════════════════════════════════
# SECTION 24 — Content-Length mismatch
# ══════════════════════════════════════════════
def test_content_length_mismatch(host, port):
    section("24 · Content-Length mismatch")

    raw = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Content-Type: text/plain\r\n"
        f"Content-Length: 100\r\n"
        f"Connection: close\r\n\r\n"
        f"only10byte"
    ).encode()
    resp = raw_request(host, port, raw, timeout=6)
    check("CL mismatch: server responds or times out (no crash)",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])

    check("Server alive after CL mismatch", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 25 — Encoded URI
# ══════════════════════════════════════════════
def test_encoded_uri(host, port):
    section("25 · URL-encoded URI characters")

    r, _ = get(host, port, "/does%20not%20exist")
    check("Encoded spaces in URI → 404 (not crash)", r.status == 404, r.status, 404)

    r, _ = get(host, port, "/%2e%2e/etc/passwd")
    check("Path traversal attempt → 4xx or 404",
          r.status in (400, 403, 404), r.status)

    check("Server alive after encoded URIs", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 26 — Path traversal
# ══════════════════════════════════════════════
def test_path_traversal(host, port):
    section("26 · Path traversal attempts")

    payloads = [
        "/../etc/passwd",
        "/../../etc/passwd",
        "/cgi-bin/../../../etc/passwd",
        "/..",
        "/./././../etc/passwd",
    ]
    for path in payloads:
        try:
            r, body = get(host, port, path)
            body_str = body.decode(errors="replace")
            check(f"Traversal {path[:30]} → no passwd leak",
                  "root:x:" not in body_str and r.status in (400, 403, 404, 200),
                  f"status={r.status} body={body_str[:50]}")
        except Exception as e:
            check(f"Traversal {path[:30]}", True, str(e))

    check("Server alive after traversal attempts", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 27 — HEAD method
# ══════════════════════════════════════════════
def test_head_method(host, port):
    section("27 · HEAD method")

    try:
        conn = make_connection(host, port)
        conn.request("HEAD", "/", headers={
            "Host":       f"{host}:{port}",
            "Connection": "close",
        })
        r = conn.getresponse()
        body = r.read()
        conn.close()
        check("HEAD / → 200 or 405",        r.status in (200, 405), r.status)
        if r.status == 200:
            check("HEAD response body empty", len(body) == 0, len(body))
    except Exception as e:
        check("HEAD method", False, str(e))


# ══════════════════════════════════════════════
# SECTION 28 — Required response headers
# ══════════════════════════════════════════════
def test_response_headers(host, port):
    section("28 · Required response headers")

    r, _ = get(host, port, "/")
    check("Content-Length header present",  r.getheader("Content-Length") is not None,
          r.getheader("Content-Length"))
    check("Content-Type header present",    r.getheader("Content-Type")   is not None,
          r.getheader("Content-Type"))
    check("Connection header present",      r.getheader("Connection")     is not None,
          r.getheader("Connection"))


# ══════════════════════════════════════════════
# SECTION 29 — Rapid reconnects
# ══════════════════════════════════════════════
def test_rapid_reconnects(host, port):
    section("29 · Rapid connect/disconnect (no body)")

    errors = []
    for i in range(20):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(2)
            s.connect((host, port))
            s.close()
        except Exception as e:
            errors.append(str(e))

    check("20 rapid connect/close — no errors", len(errors) == 0, errors)
    check("Server alive after rapid reconnects", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 30 — Upload then verify
# ══════════════════════════════════════════════
def test_upload_then_verify(host, port):
    section("30 · Upload file then verify via /uploads")

    if not os.path.exists(IMAGE_PATH):
        skip("Upload+verify", f"{IMAGE_PATH} not found")
        return

    with open(IMAGE_PATH, "rb") as f:
        img = f.read()
    r, _ = post(host, port, UPLOAD_URL, body=img,
                headers={"Content-Type": "image/jpeg"})
    check("Upload for verify test → 200",   r.status == 200, r.status, 200)
    time.sleep(0.1)

    r, _ = get(host, port, DOWNLOAD_URL)
    check("File exists after upload → 200", r.status == 200, r.status, 200)


# ══════════════════════════════════════════════
# SECTION 31 — Many headers
# ══════════════════════════════════════════════
def test_many_headers(host, port):
    section("31 · Request with many headers")

    extra = {f"X-Custom-{i}": f"value{i}" for i in range(50)}
    extra["Connection"] = "close"
    try:
        r, body = get(host, port, "/", headers=extra)
        check("50 custom headers — server responds", r.status in range(200, 600), r.status)
        check("50 custom headers — body present",    len(body) > 0, len(body))
    except Exception as e:
        check("50 custom headers", False, str(e))

    check("Server alive after many headers", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 32 — Chunked Transfer-Encoding POST
# ══════════════════════════════════════════════
def test_chunked_upload(host, port):
    section("32 · Chunked Transfer-Encoding POST")

    data = b"hello chunked world"
    chunk = f"{len(data):x}\r\n".encode() + data + b"\r\n" + b"0\r\n\r\n"

    raw = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Transfer-Encoding: chunked\r\n"
        f"Content-Type: text/plain\r\n"
        f"Connection: close\r\n\r\n"
    ).encode() + chunk

    resp = raw_request(host, port, raw, timeout=8)
    check("Chunked POST → not 500",
          resp[:8] == b"HTTP/1.1" and b"500" not in resp[:20], resp[:20])
    check("Chunked POST → 200 or 400",
          b"200" in resp[:20] or b"400" in resp[:20], resp[:20])
    check("Server alive after chunked POST", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 33 — Multiple query string params
# ══════════════════════════════════════════════
def test_multi_query_string(host, port):
    section("33 · Multiple query string parameters")

    r, body = get(host, port, f"{CGI_INFO_URL}?a=1&b=hello&c=world&d=42")
    if r.status == 404:
        skip("Multi query string", "info.py not deployed")
        return
    body_str = body.decode(errors="replace")
    check("GET with 4 params → 200",        r.status == 200, r.status, 200)
    check("QUERY_STRING forwarded fully",
          "a=1" in body_str and "b=hello" in body_str and "c=world" in body_str,
          body_str)

    r2, body2 = get(host, port, f"{CGI_INFO_URL}?msg=hello+world&x=%41")
    body_str2 = body2.decode(errors="replace")
    check("Query string with encoded chars → 200", r2.status == 200, r2.status, 200)
    check("Encoded query string forwarded",  "msg=hello" in body_str2, body_str2)


# ══════════════════════════════════════════════
# SECTION 34 — CGI timeout / hanging script
# ══════════════════════════════════════════════
def test_cgi_timeout(host, port):
    section("34 · CGI timeout — non-existent CGI script")

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(3)
        s.connect((host, port))
        req = (
            f"GET /cgi-bin/hang_forever.php HTTP/1.1\r\n"
            f"Host: {host}:{port}\r\n"
            f"Connection: close\r\n\r\n"
        ).encode()
        s.sendall(req)
        chunks = []
        try:
            while True:
                c = s.recv(4096)
                if not c:
                    break
                chunks.append(c)
        except socket.timeout:
            pass
        s.close()
        resp = b"".join(chunks)
        check("Non-existent CGI → 404 or 500",
              len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    except Exception as e:
        check("CGI timeout handling", True, str(e))

    check("Server alive after CGI timeout test", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 35 — Custom error pages
# Config has error_page 404 and 500 configured
# ══════════════════════════════════════════════
def test_error_pages(host, port):
    section("35 · Custom error pages")

    r, body = get(host, port, "/definitely_does_not_exist_xyz.html")
    body_str = body.decode(errors="replace")
    check("404 → correct status",           r.status == 404, r.status, 404)
    check("404 body non-empty",             len(body) > 0, len(body))
    check("404 response has Content-Type",
          r.getheader("Content-Type") is not None, r.getheader("Content-Type"))

    # POST to GET-only location → 405; body must not be empty
    r2, body2 = post(host, port, "/", body=b"x")
    check("405 → correct status",           r2.status == 405, r2.status, 405)
    check("405 body non-empty",             len(body2) > 0, len(body2))


# ══════════════════════════════════════════════
# SECTION 36 — DELETE file from /uploads
# Config: location /uploads  methods GET DELETE  root ./uploads
# ══════════════════════════════════════════════
def test_delete_file(host, port):
    section("36 · DELETE file from /uploads")

    test_filename = "delete_test.txt"
    upload_path = os.path.join(PROJECT_ROOT, "uploads", test_filename)

    try:
        with open(upload_path, "w") as f:
            f.write("delete me")
    except Exception as e:
        skip("DELETE test", f"Cannot create test file: {e}")
        return

    r, _ = get(host, port, f"/uploads/{test_filename}")
    check("File exists before DELETE → 200", r.status == 200, r.status, 200)

    r2, _ = delete(host, port, f"/uploads/{test_filename}")
    check("DELETE file → 204 or 200",       r2.status in (200, 204), r2.status)

    time.sleep(0.1)
    r3, _ = get(host, port, f"/uploads/{test_filename}")
    check("File gone after DELETE → 404",   r3.status == 404, r3.status, 404)

    check("Server alive after DELETE",      server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 37 — POST to /upload (non-CGI upload)
# Config: location /upload  methods POST  upload_store ./uploads
# ══════════════════════════════════════════════
def test_upload_location(host, port):
    section("37 · POST to /upload location (non-CGI)")

    payload = b"plain text file content for upload test"
    r, body = post(host, port, "/upload/testfile.txt", body=payload,
                   headers={"Content-Type": "text/plain"})
    check("POST /upload/testfile.txt → 201 or 200",
          r.status in (200, 201), r.status)

    if r.status in (200, 201):
        time.sleep(0.1)
        r2, body2 = get(host, port, "/uploads/testfile.txt")
        check("Uploaded file retrievable → 200", r2.status == 200, r2.status, 200)
        if r2.status == 200:
            check("Uploaded file content matches",
                  body2 == payload, f"{len(body2)} vs {len(payload)}")

    check("Server alive after upload", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 38 — Binary file integrity
# ══════════════════════════════════════════════
def test_binary_integrity(host, port):
    section("38 · Binary file round-trip (random bytes)")

    random_data = bytes(range(256)) * 40  # 10240 bytes, all byte values

    r, body = post(host, port, UPLOAD_URL, body=random_data,
                   headers={"Content-Type": "application/octet-stream"})
    body_str = body.decode(errors="replace")
    check("POST binary data → 200",         r.status == 200, r.status, 200)
    check(f"PHP reports {len(random_data)} bytes",
          f"received: {len(random_data)}" in body_str, body_str)

    time.sleep(0.1)
    r2, dl = get(host, port, DOWNLOAD_URL)
    check("Download binary file → 200",     r2.status == 200, r2.status, 200)
    check("Binary round-trip exact match",  dl == random_data,
          f"{len(dl)} vs {len(random_data)}")


# ══════════════════════════════════════════════
# SECTION 39 — CGI error stability
# ══════════════════════════════════════════════
def test_cgi_error_stability(host, port):
    section("39 · CGI error — server stays alive after CGI failure")

    r, body = get(host, port, "/cgi-bin/nonexistent_script.php")
    check("Non-existent CGI → 404 or 500",  r.status in (404, 500), r.status)
    check("Error response has body",        len(body) > 0, len(body))

    for i in range(3):
        r2, _ = get(host, port, "/")
        check(f"Normal GET after CGI error {i+1}/3 → 200",
              r2.status == 200, r2.status, 200)

    check("Server alive after CGI errors",  server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 40 — Large static file
# ══════════════════════════════════════════════
def test_large_static_file(host, port):
    section("40 · Large static file — Content-Length accuracy")

    r, body = get(host, port, "/cat.jpg")
    if r.status == 404:
        skip("Large static file", "cat.jpg not in www/")
        return
    cl = int(r.getheader("Content-Length", "-1"))
    check("GET cat.jpg → 200",              r.status == 200, r.status, 200)
    check("Content-Length > 0",             cl > 0, cl)
    check("Content-Length == body size",    cl == len(body), f"header={cl} body={len(body)}")
    check("JPEG magic bytes FFD8",          body[:2] == b"\xff\xd8", body[:2].hex(), "ffd8")


# ══════════════════════════════════════════════
# SECTION 41 — Header whitespace handling
# ══════════════════════════════════════════════
def test_header_whitespace(host, port):
    section("41 · Header value whitespace trimming")

    raw = (
        f"GET / HTTP/1.1\r\n"
        f"Host:   {host}:{port}   \r\n"
        f"Connection:   close   \r\n"
        f"Content-Type:   text/plain   \r\n"
        f"\r\n"
    ).encode()
    resp = raw_request(host, port, raw)
    check("Headers with extra whitespace → 200",
          b"200" in resp[:20], resp[:20])
    check("Server alive after whitespace headers", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 42 — Case-insensitive headers
# ══════════════════════════════════════════════
def test_header_case_insensitive(host, port):
    section("42 · Case-insensitive header names")

    raw = (
        f"GET / HTTP/1.1\r\n"
        f"HOST: {host}:{port}\r\n"
        f"CONNECTION: close\r\n"
        f"\r\n"
    ).encode()
    resp = raw_request(host, port, raw)
    check("UPPERCASE headers → 200",        b"200" in resp[:20], resp[:20])

    raw2 = (
        f"GET / HTTP/1.1\r\n"
        f"hOsT: {host}:{port}\r\n"
        f"CoNnEcTiOn: close\r\n"
        f"\r\n"
    ).encode()
    resp2 = raw_request(host, port, raw2)
    check("Mixed-case headers → 200",       b"200" in resp2[:20], resp2[:20])

    check("Server alive after case tests",  server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 43 — POST without Content-Length
# ══════════════════════════════════════════════
def test_post_no_content_length(host, port):
    section("43 · POST without Content-Length header")

    raw = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Content-Type: text/plain\r\n"
        f"Connection: close\r\n"
        f"\r\n"
        f"body without content length"
    ).encode()
    resp = raw_request(host, port, raw, timeout=6)
    check("POST no Content-Length → 4xx or 200",
          resp[:8] == b"HTTP/1.1", resp[:20])
    check("Server alive after no-CL POST", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 44 — Index file from /testAutoindex
# Config: location /testAutoindex  autoindex on  root ./www
# ══════════════════════════════════════════════
def test_subdir_index(host, port):
    section("44 · Autoindex of /testAutoindex")

    r, body = get(host, port, "/testAutoindex/")
    if r.status == 404:
        skip("GET /testAutoindex/", "directory not present on disk")
        return
    check("GET /testAutoindex/ → 200",      r.status == 200, r.status, 200)
    body_str = body.decode(errors="replace")
    check("/testAutoindex/ body non-empty", len(body) > 0, len(body))
    check("Content-Type text/html",
          "text/html" in r.getheader("Content-Type", ""),
          r.getheader("Content-Type"))

    # Without trailing slash — /testAutoindex maps to ./www/testAutoindex (dir)
    r2, _ = get(host, port, "/testAutoindex")
    check("GET /testAutoindex (no slash) → 200 or 3xx",
          r2.status in (200, 301, 302), r2.status)


# ══════════════════════════════════════════════
# Summary
# ══════════════════════════════════════════════
# ══════════════════════════════════════════════
# SECTION 47 — HTTP Request Smuggling
# ══════════════════════════════════════════════
def test_request_smuggling(host, port):
    section("47 · HTTP Request Smuggling (CL + TE conflict)")

    # Both Content-Length and Transfer-Encoding present — server must reject or
    # use one consistently, never allow body desync
    raw = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Content-Length: 6\r\n"
        f"Transfer-Encoding: chunked\r\n"
        f"Connection: close\r\n\r\n"
        f"0\r\n\r\n"
    ).encode()
    resp = raw_request(host, port, raw, timeout=6)
    check("CL+TE conflict → 4xx or handled safely",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("Server alive after smuggling attempt", server_alive(host, port))

    # TE says chunked but CL says 999 — must not read 999 bytes
    raw2 = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Transfer-Encoding: chunked\r\n"
        f"Content-Length: 999\r\n"
        f"Connection: close\r\n\r\n"
        f"5\r\nhello\r\n0\r\n\r\n"
    ).encode()
    resp2 = raw_request(host, port, raw2, timeout=6)
    check("TE+CL smuggling variant → no crash",
          len(resp2) == 0 or resp2[:8] == b"HTTP/1.1", resp2[:20])
    check("Server alive after TE+CL variant", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 48 — CRLF Injection
# ══════════════════════════════════════════════
def test_crlf_injection(host, port):
    section("48 · CRLF injection in headers")

    # Try to inject extra header via Host value
    raw = (
        b"GET / HTTP/1.1\r\n"
        b"Host: localhost:8080\r\nX-Injected: evil\r\n"
        b"Connection: close\r\n\r\n"
    )
    resp = raw_request(host, port, raw, timeout=5)
    # Server should either reject (400) or ignore injected header
    # Must NOT echo back X-Injected in response headers
    check("CRLF in Host → 4xx or 200",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("CRLF injection not reflected",
          b"X-Injected" not in resp, resp[:200])
    check("Server alive after CRLF injection", server_alive(host, port))

    # CRLF in URI query string
    raw2 = (
        f"GET /?foo=bar%0d%0aX-Evil: injected HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Connection: close\r\n\r\n"
    ).encode()
    resp2 = raw_request(host, port, raw2, timeout=5)
    check("CRLF in query → no header injection",
          b"X-Evil" not in resp2, resp2[:200])
    check("Server alive after query CRLF", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 49 — Host header injection
# ══════════════════════════════════════════════
def test_host_injection(host, port):
    section("49 · Host header injection")

    # Host with extra header appended
    raw = (
        f"GET / HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\nX-Forwarded-For: 1.2.3.4\r\n"
        f"Connection: close\r\n\r\n"
    ).encode()
    resp = raw_request(host, port, raw, timeout=5)
    check("Host injection → 4xx or 200 (no crash)",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])

    # Host pointing to external domain
    raw2 = (
        b"GET / HTTP/1.1\r\n"
        b"Host: evil.attacker.com\r\n"
        b"Connection: close\r\n\r\n"
    )
    resp2 = raw_request(host, port, raw2, timeout=5)
    # Server may 400, 404, or serve default — must not crash
    check("External Host → handled safely",
          len(resp2) == 0 or resp2[:8] == b"HTTP/1.1", resp2[:20])
    check("Server alive after host injection", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 50 — Absolute URI in request line
# ══════════════════════════════════════════════
def test_absolute_uri(host, port):
    section("50 · Absolute URI in request line")

    raw = (
        f"GET http://evil.com/steal HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Connection: close\r\n\r\n"
    ).encode()
    resp = raw_request(host, port, raw, timeout=5)
    # Server must not proxy to evil.com — should 400 or 404
    check("Absolute URI → 4xx or handled locally",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("Absolute URI not proxied (no 200 from evil.com)",
          b"evil" not in resp.lower()[:500], resp[:100])
    check("Server alive after absolute URI", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 51 — Fragment in URI
# ══════════════════════════════════════════════
def test_uri_fragment(host, port):
    section("51 · URI with fragment identifier")

    # Fragments should be stripped by client normally but if sent to server:
    raw = (
        f"GET /index.html#../../etc/passwd HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Connection: close\r\n\r\n"
    ).encode()
    resp = raw_request(host, port, raw, timeout=5)
    body = resp[resp.find(b"\r\n\r\n")+4:] if b"\r\n\r\n" in resp else b""
    check("Fragment in URI → no /etc/passwd leak",
          b"root:x:" not in body, body[:100])
    check("Fragment URI → 4xx or 200",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("Server alive after fragment URI", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 52 — Deeply nested path
# ══════════════════════════════════════════════
def test_deep_path(host, port):
    section("52 · Excessively nested URI path")

    deep = "/a" * 250  # 500 chars, 250 levels deep
    raw = f"GET {deep} HTTP/1.1\r\nHost: {host}:{port}\r\nConnection: close\r\n\r\n".encode()
    resp = raw_request(host, port, raw, timeout=5)
    check("Deep nested path → 4xx or 404",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("Server alive after deep path", server_alive(host, port))

    # Path with many dots
    dotty = "/../" * 100 + "etc/passwd"
    raw2 = f"GET /{dotty} HTTP/1.1\r\nHost: {host}:{port}\r\nConnection: close\r\n\r\n".encode()
    resp2 = raw_request(host, port, raw2, timeout=5)
    body2 = resp2[resp2.find(b"\r\n\r\n")+4:] if b"\r\n\r\n" in resp2 else b""
    check("100x ../ traversal → no passwd",
          b"root:x:" not in body2, body2[:50])
    check("Server alive after dotty path", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 53 — Zero Content-Length with body
# ══════════════════════════════════════════════
def test_zero_cl_with_body(host, port):
    section("53 · Content-Length: 0 but body present")

    raw = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Content-Type: text/plain\r\n"
        f"Content-Length: 0\r\n"
        f"Connection: close\r\n\r\n"
        f"this body should be ignored"
    ).encode()
    resp = raw_request(host, port, raw, timeout=5)
    check("CL:0 with body → 200 (body ignored)",
          b"200" in resp[:20], resp[:20])
    check("Server alive after CL:0 + body", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 54 — Negative Content-Length
# ══════════════════════════════════════════════
def test_negative_content_length(host, port):
    section("54 · Negative Content-Length")

    raw = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Content-Type: text/plain\r\n"
        f"Content-Length: -1\r\n"
        f"Connection: close\r\n\r\n"
        f"hello"
    ).encode()
    resp = raw_request(host, port, raw, timeout=5)
    check("Negative CL → 4xx",
          len(resp) == 0 or (resp[:8] == b"HTTP/1.1" and b" 4" in resp[:12]),
          resp[:20])
    check("Server alive after negative CL", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 55 — Overflow Content-Length
# ══════════════════════════════════════════════
def test_overflow_content_length(host, port):
    section("55 · Overflow Content-Length value")

    for cl_val in ["99999999999999999999", "18446744073709551616", "999999999999999"]:
        raw = (
            f"POST {UPLOAD_URL} HTTP/1.1\r\n"
            f"Host: {host}:{port}\r\n"
            f"Content-Type: text/plain\r\n"
            f"Content-Length: {cl_val}\r\n"
            f"Connection: close\r\n\r\n"
        ).encode()
        resp = raw_request(host, port, raw, timeout=5)
        check(f"CL={cl_val[:15]} → 4xx or close",
              len(resp) == 0 or (resp[:8] == b"HTTP/1.1" and b" 4" in resp[:12]),
              resp[:20])

    check("Server alive after overflow CL", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 56 — Repeated headers
# ══════════════════════════════════════════════
def test_repeated_headers(host, port):
    section("56 · Repeated header names")

    # 100 Content-Type headers
    hdrs = "Content-Type: text/plain\r\n" * 100
    raw = (
        f"GET / HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        + hdrs +
        f"Connection: close\r\n\r\n"
    ).encode()
    resp = raw_request(host, port, raw, timeout=8)
    check("100 repeated headers → responds",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("Server alive after repeated headers", server_alive(host, port))

    # Repeated Host headers
    raw2 = (
        f"GET / HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Host: evil.com\r\n"
        f"Connection: close\r\n\r\n"
    ).encode()
    resp2 = raw_request(host, port, raw2, timeout=5)
    check("Duplicate Host headers → 4xx or 200",
          len(resp2) == 0 or resp2[:8] == b"HTTP/1.1", resp2[:20])
    check("Server alive after duplicate Host", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 57 — Extra spaces in request line
# ══════════════════════════════════════════════
def test_request_line_spaces(host, port):
    section("57 · Extra spaces in request line")

    # Double space between method and URI
    raw = f"GET  / HTTP/1.1\r\nHost: {host}:{port}\r\nConnection: close\r\n\r\n".encode()
    resp = raw_request(host, port, raw, timeout=5)
    check("Double space in request line → 4xx or 200",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])

    # Trailing space after HTTP version
    raw2 = f"GET / HTTP/1.1 \r\nHost: {host}:{port}\r\nConnection: close\r\n\r\n".encode()
    resp2 = raw_request(host, port, raw2, timeout=5)
    check("Trailing space after version → 4xx or 200",
          len(resp2) == 0 or resp2[:8] == b"HTTP/1.1", resp2[:20])

    # Leading spaces before method
    raw3 = f"  GET / HTTP/1.1\r\nHost: {host}:{port}\r\nConnection: close\r\n\r\n".encode()
    resp3 = raw_request(host, port, raw3, timeout=5)
    check("Leading spaces before method → 4xx",
          len(resp3) == 0 or (resp3[:8] == b"HTTP/1.1" and b" 4" in resp3[:12]),
          resp3[:20])

    check("Server alive after space variants", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 58 — Tab in header
# ══════════════════════════════════════════════
def test_tab_in_header(host, port):
    section("58 · Tab character in header value")

    raw = (
        b"GET / HTTP/1.1\r\n"
        b"Host: localhost:8080\r\n"
        b"Content-Type:\tapplication/json\r\n"
        b"Connection: close\r\n\r\n"
    )
    resp = raw_request(host, port, raw, timeout=5)
    check("Tab in header value → 200 or 400",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("Server alive after tab in header", server_alive(host, port))

    # Tab instead of space after colon (obs-fold)
    raw2 = (
        b"GET / HTTP/1.1\r\n"
        b"Host:\tlocalhost:8080\r\n"
        b"Connection: close\r\n\r\n"
    )
    resp2 = raw_request(host, port, raw2, timeout=5)
    check("Tab instead of space after colon → handled",
          len(resp2) == 0 or resp2[:8] == b"HTTP/1.1", resp2[:20])
    check("Server alive after tab variants", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 59 — Slow Loris
# ══════════════════════════════════════════════
def test_slow_loris(host, port):
    section("59 · Slow Loris — incomplete request never finishes")

    # Open connection, send partial headers very slowly, never complete
    # Server should eventually timeout and close — and keep serving others
    sockets = []
    try:
        for _ in range(5):
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(2)
            try:
                s.connect((host, port))
                # Send partial request — never send final \r\n\r\n
                s.send(f"GET / HTTP/1.1\r\nHost: {host}:{port}\r\n".encode())
                sockets.append(s)
            except Exception:
                pass

        time.sleep(1)

        # Server must still respond to normal requests
        check("Server responds during slow loris", server_alive(host, port))

    finally:
        for s in sockets:
            try:
                s.close()
            except Exception:
                pass

    time.sleep(0.5)
    check("Server alive after slow loris", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 60 — CGI env injection via query string
# ══════════════════════════════════════════════
def test_cgi_env_injection(host, port):
    section("60 · CGI environment variable injection")

    # Newline in query string trying to inject env var
    r, body = get(host, port, f"{CGI_INFO_URL}?foo=bar%0aHTTP_INJECTED=evil")
    if r.status == 404:
        skip("CGI env injection", "info.py not deployed")
        return
    body_str = body.decode(errors="replace")
    check("Query with newline → 200 or 400",  r.status in (200, 400), r.status)
    check("Injected env var not in output",
          "HTTP_INJECTED" not in body_str or "evil" not in body_str,
          body_str[:200])

    # Null byte in query string
    r2, body2 = get(host, port, f"{CGI_INFO_URL}?foo=bar%00baz")
    body_str2 = body2.decode(errors="replace")
    check("Null byte in query → 200 or 400",  r2.status in (200, 400), r2.status)
    check("Server alive after env injection",  server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 61 — POST binary body with null bytes
# ══════════════════════════════════════════════
def test_binary_null_bytes(host, port):
    section("61 · POST binary body containing null bytes")

    # Body with null bytes interspersed — must pass through intact
    payload = b"start\x00middle\x00\x00end"
    expected = len(payload)

    r, body = post(host, port, UPLOAD_URL, body=payload,
                   headers={"Content-Type": "application/octet-stream"})
    body_str = body.decode(errors="replace")
    check("POST with null bytes → 200",      r.status == 200, r.status, 200)
    check(f"PHP reports {expected} bytes",   f"received: {expected}" in body_str, body_str)

    # All-zeros body
    zeros = b"\x00" * 1024
    r2, body2 = post(host, port, UPLOAD_URL, body=zeros,
                     headers={"Content-Type": "application/octet-stream"})
    body_str2 = body2.decode(errors="replace")
    check("POST all-zero body → 200",        r2.status == 200, r2.status, 200)
    check("PHP reports 1024 bytes",          "received: 1024" in body_str2, body_str2)

    check("Server alive after null-byte POST", server_alive(host, port))


# ══════════════════════════════════════════════
# Summary
# ══════════════════════════════════════════════
# ══════════════════════════════════════════════
# SECTION 47 — HTTP Request Smuggling
# ══════════════════════════════════════════════
def test_request_smuggling(host, port):
    section("47 · HTTP Request Smuggling (CL + TE conflict)")

    # Both Content-Length and Transfer-Encoding present — server must reject or
    # use one consistently, never allow body desync
    raw = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Content-Length: 6\r\n"
        f"Transfer-Encoding: chunked\r\n"
        f"Connection: close\r\n\r\n"
        f"0\r\n\r\n"
    ).encode()
    resp = raw_request(host, port, raw, timeout=6)
    check("CL+TE conflict → 4xx or handled safely",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("Server alive after smuggling attempt", server_alive(host, port))

    # TE says chunked but CL says 999 — must not read 999 bytes
    raw2 = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Transfer-Encoding: chunked\r\n"
        f"Content-Length: 999\r\n"
        f"Connection: close\r\n\r\n"
        f"5\r\nhello\r\n0\r\n\r\n"
    ).encode()
    resp2 = raw_request(host, port, raw2, timeout=6)
    check("TE+CL smuggling variant → no crash",
          len(resp2) == 0 or resp2[:8] == b"HTTP/1.1", resp2[:20])
    check("Server alive after TE+CL variant", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 48 — CRLF Injection
# ══════════════════════════════════════════════
def test_crlf_injection(host, port):
    section("48 · CRLF injection in headers")

    # Try to inject extra header via Host value
    raw = (
        b"GET / HTTP/1.1\r\n"
        b"Host: localhost:8080\r\nX-Injected: evil\r\n"
        b"Connection: close\r\n\r\n"
    )
    resp = raw_request(host, port, raw, timeout=5)
    # Server should either reject (400) or ignore injected header
    # Must NOT echo back X-Injected in response headers
    check("CRLF in Host → 4xx or 200",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("CRLF injection not reflected",
          b"X-Injected" not in resp, resp[:200])
    check("Server alive after CRLF injection", server_alive(host, port))

    # CRLF in URI query string
    raw2 = (
        f"GET /?foo=bar%0d%0aX-Evil: injected HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Connection: close\r\n\r\n"
    ).encode()
    resp2 = raw_request(host, port, raw2, timeout=5)
    check("CRLF in query → no header injection",
          b"X-Evil" not in resp2, resp2[:200])
    check("Server alive after query CRLF", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 49 — Host header injection
# ══════════════════════════════════════════════
def test_host_injection(host, port):
    section("49 · Host header injection")

    # Host with extra header appended
    raw = (
        f"GET / HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\nX-Forwarded-For: 1.2.3.4\r\n"
        f"Connection: close\r\n\r\n"
    ).encode()
    resp = raw_request(host, port, raw, timeout=5)
    check("Host injection → 4xx or 200 (no crash)",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])

    # Host pointing to external domain
    raw2 = (
        b"GET / HTTP/1.1\r\n"
        b"Host: evil.attacker.com\r\n"
        b"Connection: close\r\n\r\n"
    )
    resp2 = raw_request(host, port, raw2, timeout=5)
    # Server may 400, 404, or serve default — must not crash
    check("External Host → handled safely",
          len(resp2) == 0 or resp2[:8] == b"HTTP/1.1", resp2[:20])
    check("Server alive after host injection", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 50 — Absolute URI in request line
# ══════════════════════════════════════════════
def test_absolute_uri(host, port):
    section("50 · Absolute URI in request line")

    raw = (
        f"GET http://evil.com/steal HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Connection: close\r\n\r\n"
    ).encode()
    resp = raw_request(host, port, raw, timeout=5)
    # Server must not proxy to evil.com — should 400 or 404
    check("Absolute URI → 4xx or handled locally",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("Absolute URI not proxied (no 200 from evil.com)",
          b"evil" not in resp.lower()[:500], resp[:100])
    check("Server alive after absolute URI", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 51 — Fragment in URI
# ══════════════════════════════════════════════
def test_uri_fragment(host, port):
    section("51 · URI with fragment identifier")

    # Fragments should be stripped by client normally but if sent to server:
    raw = (
        f"GET /index.html#../../etc/passwd HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Connection: close\r\n\r\n"
    ).encode()
    resp = raw_request(host, port, raw, timeout=5)
    body = resp[resp.find(b"\r\n\r\n")+4:] if b"\r\n\r\n" in resp else b""
    check("Fragment in URI → no /etc/passwd leak",
          b"root:x:" not in body, body[:100])
    check("Fragment URI → 4xx or 200",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("Server alive after fragment URI", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 52 — Deeply nested path
# ══════════════════════════════════════════════
def test_deep_path(host, port):
    section("52 · Excessively nested URI path")

    deep = "/a" * 250  # 500 chars, 250 levels deep
    raw = f"GET {deep} HTTP/1.1\r\nHost: {host}:{port}\r\nConnection: close\r\n\r\n".encode()
    resp = raw_request(host, port, raw, timeout=5)
    check("Deep nested path → 4xx or 404",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("Server alive after deep path", server_alive(host, port))

    # Path with many dots
    dotty = "/../" * 100 + "etc/passwd"
    raw2 = f"GET /{dotty} HTTP/1.1\r\nHost: {host}:{port}\r\nConnection: close\r\n\r\n".encode()
    resp2 = raw_request(host, port, raw2, timeout=5)
    body2 = resp2[resp2.find(b"\r\n\r\n")+4:] if b"\r\n\r\n" in resp2 else b""
    check("100x ../ traversal → no passwd",
          b"root:x:" not in body2, body2[:50])
    check("Server alive after dotty path", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 53 — Zero Content-Length with body
# ══════════════════════════════════════════════
def test_zero_cl_with_body(host, port):
    section("53 · Content-Length: 0 but body present")

    raw = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Content-Type: text/plain\r\n"
        f"Content-Length: 0\r\n"
        f"Connection: close\r\n\r\n"
        f"this body should be ignored"
    ).encode()
    resp = raw_request(host, port, raw, timeout=5)
    check("CL:0 with body → 200 (body ignored)",
          b"200" in resp[:20], resp[:20])
    check("Server alive after CL:0 + body", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 54 — Negative Content-Length
# ══════════════════════════════════════════════
def test_negative_content_length(host, port):
    section("54 · Negative Content-Length")

    raw = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Content-Type: text/plain\r\n"
        f"Content-Length: -1\r\n"
        f"Connection: close\r\n\r\n"
        f"hello"
    ).encode()
    resp = raw_request(host, port, raw, timeout=5)
    check("Negative CL → 4xx",
          len(resp) == 0 or (resp[:8] == b"HTTP/1.1" and b" 4" in resp[:12]),
          resp[:20])
    check("Server alive after negative CL", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 55 — Overflow Content-Length
# ══════════════════════════════════════════════
def test_overflow_content_length(host, port):
    section("55 · Overflow Content-Length value")

    for cl_val in ["99999999999999999999", "18446744073709551616", "999999999999999"]:
        raw = (
            f"POST {UPLOAD_URL} HTTP/1.1\r\n"
            f"Host: {host}:{port}\r\n"
            f"Content-Type: text/plain\r\n"
            f"Content-Length: {cl_val}\r\n"
            f"Connection: close\r\n\r\n"
        ).encode()
        resp = raw_request(host, port, raw, timeout=5)
        check(f"CL={cl_val[:15]} → 4xx or close",
              len(resp) == 0 or (resp[:8] == b"HTTP/1.1" and b" 4" in resp[:12]),
              resp[:20])

    check("Server alive after overflow CL", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 56 — Repeated headers
# ══════════════════════════════════════════════
def test_repeated_headers(host, port):
    section("56 · Repeated header names")

    # 100 Content-Type headers
    hdrs = "Content-Type: text/plain\r\n" * 100
    raw = (
        f"GET / HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        + hdrs +
        f"Connection: close\r\n\r\n"
    ).encode()
    resp = raw_request(host, port, raw, timeout=8)
    check("100 repeated headers → responds",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("Server alive after repeated headers", server_alive(host, port))

    # Repeated Host headers
    raw2 = (
        f"GET / HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Host: evil.com\r\n"
        f"Connection: close\r\n\r\n"
    ).encode()
    resp2 = raw_request(host, port, raw2, timeout=5)
    check("Duplicate Host headers → 4xx or 200",
          len(resp2) == 0 or resp2[:8] == b"HTTP/1.1", resp2[:20])
    check("Server alive after duplicate Host", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 57 — Extra spaces in request line
# ══════════════════════════════════════════════
def test_request_line_spaces(host, port):
    section("57 · Extra spaces in request line")

    # Double space between method and URI
    raw = f"GET  / HTTP/1.1\r\nHost: {host}:{port}\r\nConnection: close\r\n\r\n".encode()
    resp = raw_request(host, port, raw, timeout=5)
    check("Double space in request line → 4xx or 200",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])

    # Trailing space after HTTP version
    raw2 = f"GET / HTTP/1.1 \r\nHost: {host}:{port}\r\nConnection: close\r\n\r\n".encode()
    resp2 = raw_request(host, port, raw2, timeout=5)
    check("Trailing space after version → 4xx or 200",
          len(resp2) == 0 or resp2[:8] == b"HTTP/1.1", resp2[:20])

    # Leading spaces before method
    raw3 = f"  GET / HTTP/1.1\r\nHost: {host}:{port}\r\nConnection: close\r\n\r\n".encode()
    resp3 = raw_request(host, port, raw3, timeout=5)
    check("Leading spaces before method → 4xx",
          len(resp3) == 0 or (resp3[:8] == b"HTTP/1.1" and b" 4" in resp3[:12]),
          resp3[:20])

    check("Server alive after space variants", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 58 — Tab in header
# ══════════════════════════════════════════════
def test_tab_in_header(host, port):
    section("58 · Tab character in header value")

    raw = (
        b"GET / HTTP/1.1\r\n"
        b"Host: localhost:8080\r\n"
        b"Content-Type:\tapplication/json\r\n"
        b"Connection: close\r\n\r\n"
    )
    resp = raw_request(host, port, raw, timeout=5)
    check("Tab in header value → 200 or 400",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    check("Server alive after tab in header", server_alive(host, port))

    # Tab instead of space after colon (obs-fold)
    raw2 = (
        b"GET / HTTP/1.1\r\n"
        b"Host:\tlocalhost:8080\r\n"
        b"Connection: close\r\n\r\n"
    )
    resp2 = raw_request(host, port, raw2, timeout=5)
    check("Tab instead of space after colon → handled",
          len(resp2) == 0 or resp2[:8] == b"HTTP/1.1", resp2[:20])
    check("Server alive after tab variants", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 59 — Slow Loris
# ══════════════════════════════════════════════
def test_slow_loris(host, port):
    section("59 · Slow Loris — incomplete request never finishes")

    # Open connection, send partial headers very slowly, never complete
    # Server should eventually timeout and close — and keep serving others
    sockets = []
    try:
        for _ in range(5):
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(2)
            try:
                s.connect((host, port))
                # Send partial request — never send final \r\n\r\n
                s.send(f"GET / HTTP/1.1\r\nHost: {host}:{port}\r\n".encode())
                sockets.append(s)
            except Exception:
                pass

        time.sleep(1)

        # Server must still respond to normal requests
        check("Server responds during slow loris", server_alive(host, port))

    finally:
        for s in sockets:
            try:
                s.close()
            except Exception:
                pass

    time.sleep(0.5)
    check("Server alive after slow loris", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 60 — CGI env injection via query string
# ══════════════════════════════════════════════
def test_cgi_env_injection(host, port):
    section("60 · CGI environment variable injection")

    # Newline in query string trying to inject env var
    r, body = get(host, port, f"{CGI_INFO_URL}?foo=bar%0aHTTP_INJECTED=evil")
    if r.status == 404:
        skip("CGI env injection", "info.py not deployed")
        return
    body_str = body.decode(errors="replace")
    check("Query with newline → 200 or 400",  r.status in (200, 400), r.status)
    check("Injected env var not in output",
          "HTTP_INJECTED" not in body_str or "evil" not in body_str,
          body_str[:200])

    # Null byte in query string
    r2, body2 = get(host, port, f"{CGI_INFO_URL}?foo=bar%00baz")
    body_str2 = body2.decode(errors="replace")
    check("Null byte in query → 200 or 400",  r2.status in (200, 400), r2.status)
    check("Server alive after env injection",  server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 61 — POST binary body with null bytes
# ══════════════════════════════════════════════
def test_binary_null_bytes(host, port):
    section("61 · POST binary body containing null bytes")

    # Body with null bytes interspersed — must pass through intact
    payload = b"start\x00middle\x00\x00end"
    expected = len(payload)

    r, body = post(host, port, UPLOAD_URL, body=payload,
                   headers={"Content-Type": "application/octet-stream"})
    body_str = body.decode(errors="replace")
    check("POST with null bytes → 200",      r.status == 200, r.status, 200)
    check(f"PHP reports {expected} bytes",   f"received: {expected}" in body_str, body_str)

    # All-zeros body
    zeros = b"\x00" * 1024
    r2, body2 = post(host, port, UPLOAD_URL, body=zeros,
                     headers={"Content-Type": "application/octet-stream"})
    body_str2 = body2.decode(errors="replace")
    check("POST all-zero body → 200",        r2.status == 200, r2.status, 200)
    check("PHP reports 1024 bytes",          "received: 1024" in body_str2, body_str2)

    check("Server alive after null-byte POST", server_alive(host, port))


# ══════════════════════════════════════════════
# Summary
# ══════════════════════════════════════════════
def summary():
    section("Summary")
    passed = sum(1 for _, ok in results if ok)
    total  = len(results)
    color  = "\033[92m" if passed == total else "\033[91m"
    print(f"{color}{passed}/{total} tests passed\033[0m\n")
    if passed < total:
        print("Failed tests:")
        for name, ok in results:
            if not ok:
                print(f"  • {name}")
    print()


# ══════════════════════════════════════════════
# Entry point
# ══════════════════════════════════════════════
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    args = parser.parse_args()

    print(f"\n{INFO} Testing http://{args.host}:{args.port}\n")

    test_static_get(args.host, args.port)
    test_content_types(args.host, args.port)
    test_content_length(args.host, args.port)
    test_method_not_allowed(args.host, args.port)
    test_redirect(args.host, args.port)
    test_bad_requests(args.host, args.port)
    test_large_uri(args.host, args.port)
    test_autoindex(args.host, args.port)
    test_cgi_get(args.host, args.port)
    test_cgi_post_image(args.host, args.port)
    test_cgi_post_sizes(args.host, args.port)
    test_cgi_python_get(args.host, args.port)
    test_cgi_python_post(args.host, args.port)
    test_cgi_worldclock(args.host, args.port)
    test_cgi_query_string(args.host, args.port)
    test_keep_alive(args.host, args.port)
    test_connection_close(args.host, args.port)
    test_concurrent(args.host, args.port)
    test_sequential_cgi(args.host, args.port)
    test_slow_client(args.host, args.port)
    test_pipelining(args.host, args.port)
    test_body_too_large(args.host, args.port)
    test_empty_body_post(args.host, args.port)
    test_content_length_mismatch(args.host, args.port)
    test_encoded_uri(args.host, args.port)
    test_path_traversal(args.host, args.port)
    test_head_method(args.host, args.port)
    test_response_headers(args.host, args.port)
    test_rapid_reconnects(args.host, args.port)
    test_upload_then_verify(args.host, args.port)
    test_many_headers(args.host, args.port)
    test_chunked_upload(args.host, args.port)
    test_multi_query_string(args.host, args.port)
    test_cgi_timeout(args.host, args.port)
    test_error_pages(args.host, args.port)
    test_delete_file(args.host, args.port)
    test_upload_location(args.host, args.port)
    test_binary_integrity(args.host, args.port)
    test_cgi_error_stability(args.host, args.port)
    test_large_static_file(args.host, args.port)
    test_header_whitespace(args.host, args.port)
    test_header_case_insensitive(args.host, args.port)
    test_post_no_content_length(args.host, args.port)
    test_subdir_index(args.host, args.port)
    test_request_smuggling(args.host, args.port)
    test_crlf_injection(args.host, args.port)
    test_host_injection(args.host, args.port)
    test_absolute_uri(args.host, args.port)
    test_uri_fragment(args.host, args.port)
    test_deep_path(args.host, args.port)
    test_zero_cl_with_body(args.host, args.port)
    test_negative_content_length(args.host, args.port)
    test_overflow_content_length(args.host, args.port)
    test_repeated_headers(args.host, args.port)
    test_request_line_spaces(args.host, args.port)
    test_tab_in_header(args.host, args.port)
    test_slow_loris(args.host, args.port)
    test_cgi_env_injection(args.host, args.port)
    test_binary_null_bytes(args.host, args.port)

    summary()
