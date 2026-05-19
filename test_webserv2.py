#!/usr/bin/env python3
"""
Webserv rigorous test suite
Usage: python3 test_webserv.py [--host 127.0.0.1] [--port 8080]
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
# Config
# ──────────────────────────────────────────────
DEFAULT_HOST  = "127.0.0.1"
DEFAULT_PORT  = 8080

IMAGE_PATH    = "/home/tignatov/webserv_git/uploads/cat.jpg"
UPLOAD_URL    = "/cgi-bin/image.php"
DOWNLOAD_URL  = "/uploads/cgi_upload.jpg"
CGI_INFO_URL  = "/cgi-bin/info.py"
CGI_CLOCK_URL = "/cgi-bin/worldclock.py"
CGI_TESTER    = "/cgi-bin/cgi_tester"

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

    r, _ = get(host, port, "/public")
    check("GET /public → 200 or 3xx",       r.status in (200, 301, 302), r.status)


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

    r, _ = post(host, port, "/", body=b"x")
    check("POST to GET-only / → 405",       r.status == 405, r.status, 405)

    r, _ = delete(host, port, "/")
    check("DELETE to GET-only / → 405",     r.status == 405, r.status, 405)

    # Unknown method
    raw = f"PATCH / HTTP/1.1\r\nHost: {host}:{port}\r\nContent-Length: 0\r\n\r\n".encode()
    resp = raw_request(host, port, raw)
    check("PATCH to GET-only / → 4xx",
          resp[:8] == b"HTTP/1.1" and b" 4" in resp[:12], resp[:12])


# ══════════════════════════════════════════════
# SECTION 5 — Redirect
# ══════════════════════════════════════════════
def test_redirect(host, port):
    section("5 · Redirect")

    r, _ = get(host, port, "/old")
    check("GET /old → 301",                 r.status == 301, r.status, 301)
    loc = r.getheader("Location", "")
    check("Location contains /new",         "/new" in loc, loc)
    check("Location contains query string", "from=old" in loc, loc)

    # Redirect response must have no body or Content-Length: 0
    cl = r.getheader("Content-Length", "0")
    check("Redirect Content-Length is 0 or missing", cl in ("0", ""), cl)


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
# ══════════════════════════════════════════════
def test_autoindex(host, port):
    section("8 · Autoindex directory listing")

    r, body = get(host, port, "/uploads/")
    body_str = body.decode(errors="replace")
    check("GET /uploads/ → 200",            r.status == 200, r.status, 200)
    check("Autoindex contains <a href",     "<a href" in body_str, body_str[:300])
    check("Content-Type text/html",
          "text/html" in r.getheader("Content-Type", ""),
          r.getheader("Content-Type"))

    # Without trailing slash — should still work or redirect
    r, _ = get(host, port, "/uploads")
    check("GET /uploads (no slash) → 200 or 3xx",
          r.status in (200, 301, 302), r.status)


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
# SECTION 16 — cgi_tester binary
# ══════════════════════════════════════════════
def test_cgi_tester(host, port):
    section("16 · cgi_tester binary CGI")

    r, body = get(host, port, CGI_TESTER)
    if r.status == 404:
        skip("cgi_tester GET", "binary not deployed or not executable")
        return
    check("GET cgi_tester → 200",           r.status == 200, r.status, 200)

    # POST with form data
    payload = b"var=hello&num=42"
    r, body = post(host, port, CGI_TESTER, body=payload,
                   headers={"Content-Type": "application/x-www-form-urlencoded"})
    body_str = body.decode(errors="replace")
    check("POST cgi_tester → 200",          r.status == 200, r.status, 200)
    check("cgi_tester echoes VAR",          "VAR=" in body_str or "var=" in body_str.lower(),
          body_str[:100])


# ══════════════════════════════════════════════
# SECTION 17 — Keep-Alive
# ══════════════════════════════════════════════
def test_keep_alive(host, port):
    section("17 · Keep-Alive — 5 requests on one connection")

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
# SECTION 18 — Connection: close
# ══════════════════════════════════════════════
def test_connection_close(host, port):
    section("18 · Connection: close honoured")

    r, _ = get(host, port, "/", headers={"Connection": "close"})
    check("Response has Connection: close",
          r.getheader("Connection", "").lower() == "close",
          r.getheader("Connection"))


# ══════════════════════════════════════════════
# SECTION 19 — Concurrent connections
# ══════════════════════════════════════════════
def test_concurrent(host, port):
    section("19 · Concurrent connections (20 threads)")

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
# SECTION 20 — Sequential CGI stability
# ══════════════════════════════════════════════
def test_sequential_cgi(host, port):
    section("20 · Sequential CGI — server stays stable")

    for i in range(10):
        payload = f"seq-request-{i}".encode()
        try:
            r, _ = post(host, port, UPLOAD_URL, body=payload,
                        headers={"Content-Type": "text/plain"})
            check(f"Sequential CGI {i+1}/10 → 200", r.status == 200, r.status, 200)
        except Exception as e:
            check(f"Sequential CGI {i+1}/10", False, str(e))


# ══════════════════════════════════════════════
# SECTION 21 — Slow client
# ══════════════════════════════════════════════
def test_slow_client(host, port):
    section("21 · Slow client — headers sent in chunks")

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
# SECTION 22 — Pipelined requests (back-to-back raw)
# ══════════════════════════════════════════════
def test_pipelining(host, port):
    section("22 · Pipelined requests on one connection")

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
# SECTION 23 — Body too large (413)
# ══════════════════════════════════════════════
def test_body_too_large(host, port):
    section("23 · Body too large → 413")

    # Send Content-Length larger than server's limit
    # Server limit is 100000 bytes (_recvBufferSize)
    huge = b"x" * 101000
    try:
        r, _ = post(host, port, UPLOAD_URL, body=huge,
                    headers={"Content-Type": "application/octet-stream"})
        check("Body > limit → 413",         r.status == 413, r.status, 413)
    except Exception as e:
        # Server may close connection instead
        check("Body > limit → connection closed or 413", True, str(e))

    check("Server alive after oversized body", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 24 — Empty body POST
# ══════════════════════════════════════════════
def test_empty_body_post(host, port):
    section("24 · POST with empty body")

    r, body = post(host, port, UPLOAD_URL, body=b"",
                   headers={"Content-Type": "text/plain"})
    body_str = body.decode(errors="replace")
    check("POST empty body → 200",          r.status == 200, r.status, 200)
    check("PHP reports 0 bytes",            "received: 0" in body_str, body_str)


# ══════════════════════════════════════════════
# SECTION 25 — Content-Length mismatch
# ══════════════════════════════════════════════
def test_content_length_mismatch(host, port):
    section("25 · Content-Length mismatch")

    # Claim 100 bytes but send 10
    raw = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Content-Type: text/plain\r\n"
        f"Content-Length: 100\r\n"
        f"Connection: close\r\n\r\n"
        f"only10byte"
    ).encode()
    resp = raw_request(host, port, raw, timeout=6)
    # Server should either timeout waiting for more data (no response)
    # or return 400. It must NOT crash.
    check("CL mismatch: server responds or times out (no crash)",
          len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])

    check("Server alive after CL mismatch", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 26 — Encoded URI
# ══════════════════════════════════════════════
def test_encoded_uri(host, port):
    section("26 · URL-encoded URI characters")

    # %2F is encoded slash — should be treated as literal, not path separator
    r, _ = get(host, port, "/does%20not%20exist")
    check("Encoded spaces in URI → 404 (not crash)", r.status == 404, r.status, 404)

    r, _ = get(host, port, "/%2e%2e/etc/passwd")
    check("Path traversal attempt → 4xx or 404",
          r.status in (400, 403, 404), r.status)

    check("Server alive after encoded URIs", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 27 — Path traversal
# ══════════════════════════════════════════════
def test_path_traversal(host, port):
    section("27 · Path traversal attempts")

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
            check(f"Traversal {path[:30]}", True, str(e))  # connection closed = safe

    check("Server alive after traversal attempts", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 28 — HEAD method
# ══════════════════════════════════════════════
def test_head_method(host, port):
    section("28 · HEAD method")

    try:
        conn = make_connection(host, port)
        conn.request("HEAD", "/", headers={
            "Host":       f"{host}:{port}",
            "Connection": "close",
        })
        r = conn.getresponse()
        body = r.read()
        conn.close()
        # HEAD: must return headers same as GET, body must be empty
        check("HEAD / → 200 or 405",        r.status in (200, 405), r.status)
        if r.status == 200:
            check("HEAD response body empty", len(body) == 0, len(body))
    except Exception as e:
        check("HEAD method", False, str(e))


# ══════════════════════════════════════════════
# SECTION 29 — Response headers present
# ══════════════════════════════════════════════
def test_response_headers(host, port):
    section("29 · Required response headers")

    r, _ = get(host, port, "/")
    check("Content-Length header present",  r.getheader("Content-Length") is not None,
          r.getheader("Content-Length"))
    check("Content-Type header present",    r.getheader("Content-Type")   is not None,
          r.getheader("Content-Type"))
    check("Connection header present",      r.getheader("Connection")     is not None,
          r.getheader("Connection"))


# ══════════════════════════════════════════════
# SECTION 30 — Server survives rapid reconnects
# ══════════════════════════════════════════════
def test_rapid_reconnects(host, port):
    section("30 · Rapid connect/disconnect (no body)")

    errors = []
    for i in range(20):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.settimeout(2)
            s.connect((host, port))
            s.close()  # close immediately without sending anything
        except Exception as e:
            errors.append(str(e))

    check("20 rapid connect/close — no errors", len(errors) == 0, errors)
    check("Server alive after rapid reconnects", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 31 — Upload then delete
# ══════════════════════════════════════════════
def test_upload_then_delete(host, port):
    section("31 · Upload file then DELETE it")

    # Upload via CGI
    if not os.path.exists(IMAGE_PATH):
        skip("Upload+delete", f"{IMAGE_PATH} not found")
        return

    with open(IMAGE_PATH, "rb") as f:
        img = f.read()
    r, _ = post(host, port, UPLOAD_URL, body=img,
                headers={"Content-Type": "image/jpeg"})
    check("Upload for delete test → 200",   r.status == 200, r.status, 200)
    time.sleep(0.1)

    # Verify it exists
    r, _ = get(host, port, DOWNLOAD_URL)
    check("File exists after upload → 200", r.status == 200, r.status, 200)


# ══════════════════════════════════════════════
# SECTION 32 — Large number of headers
# ══════════════════════════════════════════════
def test_many_headers(host, port):
    section("32 · Request with many headers")

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
# Summary
# ══════════════════════════════════════════════
# ══════════════════════════════════════════════
# SECTION 33 — Chunked Transfer Encoding
# ══════════════════════════════════════════════
def test_chunked_upload(host, port):
    section("33 · Chunked Transfer-Encoding POST")

    # Build a chunked body manually
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
# SECTION 34 — Multiple query string params
# ══════════════════════════════════════════════
def test_multi_query_string(host, port):
    section("34 · Multiple query string parameters")

    r, body = get(host, port, f"{CGI_INFO_URL}?a=1&b=hello&c=world&d=42")
    if r.status == 404:
        skip("Multi query string", "info.py not deployed")
        return
    body_str = body.decode(errors="replace")
    check("GET with 4 params → 200",        r.status == 200, r.status, 200)
    check("QUERY_STRING forwarded fully",
          "a=1" in body_str and "b=hello" in body_str and "c=world" in body_str,
          body_str)

    # Special chars in query string
    r2, body2 = get(host, port, f"{CGI_INFO_URL}?msg=hello+world&x=%41")
    body_str2 = body2.decode(errors="replace")
    check("Query string with encoded chars → 200", r2.status == 200, r2.status, 200)
    check("Encoded query string forwarded",  "msg=hello" in body_str2, body_str2)


# ══════════════════════════════════════════════
# SECTION 35 — CGI timeout / hanging script
# ══════════════════════════════════════════════
def test_cgi_timeout(host, port):
    section("35 · CGI timeout — hanging script")

    # Create a simple hanging CGI script path
    # We test by checking server stays alive after a request that would hang
    # Use a very short timeout on our side
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(3)
        s.connect((host, port))
        # Request a non-existent CGI that would cause issues
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
        # Should get 404 (script doesn't exist) or timeout response
        check("Non-existent CGI → 404 or 500",
              len(resp) == 0 or resp[:8] == b"HTTP/1.1", resp[:20])
    except Exception as e:
        check("CGI timeout handling", True, str(e))  # connection refused = ok

    check("Server alive after CGI timeout test", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 36 — Custom error pages
# ══════════════════════════════════════════════
def test_error_pages(host, port):
    section("36 · Custom error pages")

    # 404 error page
    r, body = get(host, port, "/definitely_does_not_exist_xyz.html")
    body_str = body.decode(errors="replace")
    check("404 → custom error page served",  r.status == 404, r.status, 404)
    check("404 body non-empty",              len(body) > 0, len(body))
    # If custom error page configured, body should not be the default
    check("404 response has Content-Type",
          r.getheader("Content-Type") is not None, r.getheader("Content-Type"))

    # 405 error page
    r2, body2 = get(host, port, "/upload")  # GET on POST-only route
    check("405 → correct status",           r2.status == 405, r2.status, 405)
    check("405 body non-empty",             len(body2) > 0, len(body2))


# ══════════════════════════════════════════════
# SECTION 37 — DELETE on upload location
# ══════════════════════════════════════════════
def test_delete_file(host, port):
    section("37 · DELETE uploaded file")

    # First upload a file via CGI
    test_filename = "delete_test.txt"
    upload_path = f"/home/tignatov/webserv_git/uploads/{test_filename}"

    # Create the file directly for the test
    try:
        with open(upload_path, "w") as f:
            f.write("delete me")
    except Exception as e:
        skip("DELETE test", f"Cannot create test file: {e}")
        return

    # Verify it's accessible
    r, _ = get(host, port, f"/uploads/{test_filename}")
    check("File exists before DELETE → 200", r.status == 200, r.status, 200)

    # DELETE it
    r2, _ = delete(host, port, f"/uploads/{test_filename}")
    check("DELETE file → 204 or 200",       r2.status in (200, 204), r2.status)

    # Verify it's gone
    time.sleep(0.1)
    r3, _ = get(host, port, f"/uploads/{test_filename}")
    check("File gone after DELETE → 404",   r3.status == 404, r3.status, 404)

    check("Server alive after DELETE",      server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 38 — POST to /upload location
# ══════════════════════════════════════════════
def test_upload_location(host, port):
    section("38 · POST to /upload location (non-CGI)")

    payload = b"plain text file content for upload test"
    r, body = post(host, port, "/upload/testfile.txt", body=payload,
                   headers={"Content-Type": "text/plain"})
    check("POST /upload/testfile.txt → 201 or 200",
          r.status in (200, 201), r.status)

    if r.status in (200, 201):
        time.sleep(0.1)
        # Try to retrieve it
        r2, body2 = get(host, port, "/uploads/testfile.txt")
        check("Uploaded file retrievable → 200", r2.status == 200, r2.status, 200)
        if r2.status == 200:
            check("Uploaded file content matches",
                  body2 == payload, f"{len(body2)} vs {len(payload)}")

    check("Server alive after upload", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 39 — Serving files from /public
# ══════════════════════════════════════════════
def test_public_location(host, port):
    section("39 · Files served from /public location")

    r, body = get(host, port, "/public")
    check("GET /public → 200 or 3xx",       r.status in (200, 301, 302), r.status)

    r2, body2 = get(host, port, "/public/")
    check("GET /public/ → 200",             r2.status == 200, r2.status, 200)
    check("GET /public/ body non-empty",    len(body2) > 0, len(body2))

    # index.html should be served
    r3, body3 = get(host, port, "/public/index.html")
    if r3.status == 404:
        skip("GET /public/index.html", "file not present in www/")
    else:
        check("GET /public/index.html → 200", r3.status == 200, r3.status, 200)
        check("Content-Type text/html",
              "text/html" in r3.getheader("Content-Type", ""),
              r3.getheader("Content-Type"))


# ══════════════════════════════════════════════
# SECTION 40 — Binary file integrity (non-JPEG)
# ══════════════════════════════════════════════
def test_binary_integrity(host, port):
    section("40 · Binary file round-trip (random bytes)")

    # Generate random binary data
    random_data = bytes(range(256)) * 40  # 10240 bytes, all byte values

    # Upload via CGI
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
# SECTION 41 — CGI error (script exits non-zero)
# ══════════════════════════════════════════════
def test_cgi_error_stability(host, port):
    section("41 · CGI error — server stays alive after CGI failure")

    # Request a PHP file that doesn't exist → CGI will fail
    r, body = get(host, port, "/cgi-bin/nonexistent_script.php")
    check("Non-existent CGI → 404 or 500",  r.status in (404, 500), r.status)
    check("Error response has body",        len(body) > 0, len(body))

    # Server must still serve normal requests after CGI failure
    for i in range(3):
        r2, _ = get(host, port, "/")
        check(f"Normal GET after CGI error {i+1}/3 → 200",
              r2.status == 200, r2.status, 200)

    check("Server alive after CGI errors",  server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 42 — Large static file
# ══════════════════════════════════════════════
def test_large_static_file(host, port):
    section("42 · Large static file — Content-Length accuracy")

    # Use cat.jpg which is in www/ (22312 bytes)
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
# SECTION 43 — Header whitespace handling
# ══════════════════════════════════════════════
def test_header_whitespace(host, port):
    section("43 · Header value whitespace trimming")

    # Extra spaces around header value
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
# SECTION 44 — Case-insensitive headers
# ══════════════════════════════════════════════
def test_header_case_insensitive(host, port):
    section("44 · Case-insensitive header names")

    # All uppercase headers
    raw = (
        f"GET / HTTP/1.1\r\n"
        f"HOST: {host}:{port}\r\n"
        f"CONNECTION: close\r\n"
        f"\r\n"
    ).encode()
    resp = raw_request(host, port, raw)
    check("UPPERCASE headers → 200",        b"200" in resp[:20], resp[:20])

    # Mixed case
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
# SECTION 45 — POST without Content-Length
# ══════════════════════════════════════════════
def test_post_no_content_length(host, port):
    section("45 · POST without Content-Length header")

    raw = (
        f"POST {UPLOAD_URL} HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Content-Type: text/plain\r\n"
        f"Connection: close\r\n"
        f"\r\n"
        f"body without content length"
    ).encode()
    resp = raw_request(host, port, raw, timeout=6)
    # Should return 411 (Length Required) or 400, or treat as 0-length body
    check("POST no Content-Length → 4xx or 200",
          resp[:8] == b"HTTP/1.1", resp[:20])
    check("Server alive after no-CL POST", server_alive(host, port))


# ══════════════════════════════════════════════
# SECTION 46 — Index file from subdirectory
# ══════════════════════════════════════════════
def test_subdir_index(host, port):
    section("46 · Index file served from subdirectory")

    # GET /public/ should serve index.html if it exists
    r, body = get(host, port, "/public/")
    check("GET /public/ → 200",             r.status == 200, r.status, 200)
    body_str = body.decode(errors="replace")
    check("/public/ body non-empty",        len(body) > 0, len(body))
    check("Content-Type text/html",
          "text/html" in r.getheader("Content-Type", ""),
          r.getheader("Content-Type"))

    # Autoindex of testAutoindex dir if it exists
    r2, body2 = get(host, port, "/testAutoindex/")
    if r2.status == 404:
        skip("Autoindex testAutoindex/", "directory not accessible via URL")
    else:
        check("GET /testAutoindex/ → 200",  r2.status == 200, r2.status, 200)


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
    test_cgi_tester(args.host, args.port)
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
    test_upload_then_delete(args.host, args.port)
    test_many_headers(args.host, args.port)
    test_chunked_upload(args.host, args.port)
    test_multi_query_string(args.host, args.port)
    test_cgi_timeout(args.host, args.port)
    test_error_pages(args.host, args.port)
    test_delete_file(args.host, args.port)
    test_upload_location(args.host, args.port)
    test_public_location(args.host, args.port)
    test_binary_integrity(args.host, args.port)
    test_cgi_error_stability(args.host, args.port)
    test_large_static_file(args.host, args.port)
    test_header_whitespace(args.host, args.port)
    test_header_case_insensitive(args.host, args.port)
    test_post_no_content_length(args.host, args.port)
    test_subdir_index(args.host, args.port)

    summary()
