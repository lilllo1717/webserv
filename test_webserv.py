#!/usr/bin/env python3
"""
Webserv test suite
Usage: python3 test_webserv.py [--host 127.0.0.1] [--port 8080]
"""

import http.client
import os
import argparse
import time
import threading
import socket

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


# ──────────────────────────────────────────────
# 1. Static GET
# ──────────────────────────────────────────────
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


# ──────────────────────────────────────────────
# 2. Content-Type per extension
# ──────────────────────────────────────────────
def test_content_types(host, port):
    section("2 · Content-Type per file extension")

    if os.path.exists(IMAGE_PATH):
        with open(IMAGE_PATH, "rb") as f:
            img = f.read()
        post(host, port, UPLOAD_URL, body=img, headers={"Content-Type": "image/jpeg"})
        time.sleep(0.1)

    r, _ = get(host, port, DOWNLOAD_URL)
    check(".jpg served as image/jpeg",
          "image/jpeg" in r.getheader("Content-Type", ""),
          r.getheader("Content-Type"))

    r, _ = get(host, port, "/")
    check("/ served as text/html",
          "text/html" in r.getheader("Content-Type", ""),
          r.getheader("Content-Type"))


# ──────────────────────────────────────────────
# 3. Content-Length accuracy
# ──────────────────────────────────────────────
def test_content_length(host, port):
    section("3 · Content-Length accuracy")

    for path in ["/", DOWNLOAD_URL]:
        r, body = get(host, port, path)
        if r.status != 200:
            skip(f"Content-Length for {path}", f"status {r.status}")
            continue
        cl = int(r.getheader("Content-Length", "-1"))
        check(f"Content-Length == body size  ({path})",
              cl == len(body), f"header={cl} body={len(body)}")


# ──────────────────────────────────────────────
# 4. Method Not Allowed
# ──────────────────────────────────────────────
def test_method_not_allowed(host, port):
    section("4 · Method Not Allowed")

    r, _ = post(host, port, "/", body=b"x")
    check("POST to GET-only / → 405",       r.status == 405, r.status, 405)

    r, _ = delete(host, port, "/")
    check("DELETE to GET-only / → 405",     r.status == 405, r.status, 405)


# ──────────────────────────────────────────────
# 5. Redirect
# ──────────────────────────────────────────────
def test_redirect(host, port):
    section("5 · Redirect")

    r, _ = get(host, port, "/old")
    check("GET /old → 301",                 r.status == 301, r.status, 301)
    loc = r.getheader("Location", "")
    check("Location contains /new",         "/new" in loc, loc, "/new?from=old&x=1")
    check("Location contains query string", "from=old" in loc, loc)


# ──────────────────────────────────────────────
# 6. Bad requests
# ──────────────────────────────────────────────
def test_bad_requests(host, port):
    section("6 · Bad / malformed requests")

    # Missing Host header → 400
    raw = b"GET / HTTP/1.1\r\n\r\n"
    resp = raw_request(host, port, raw)
    check("Missing Host → 400",             b"400" in resp[:20], resp[:20])

    # Completely garbage line
    raw = b"HELLO WORLD\r\n\r\n"
    resp = raw_request(host, port, raw)
    check("Garbage request line → 4xx",
          resp[:8] == b"HTTP/1.1" and b" 4" in resp[:12], resp[:20])


# ──────────────────────────────────────────────
# 7. Oversized URI
# ──────────────────────────────────────────────
def test_large_uri(host, port):
    section("7 · Oversized URI → 414")

    long_path = "/" + "a" * 9000
    raw = f"GET {long_path} HTTP/1.1\r\nHost: {host}:{port}\r\n\r\n".encode()
    resp = raw_request(host, port, raw)
    check("URI > 8 KB → 414 or 400",
          b"414" in resp[:20] or b"400" in resp[:20], resp[:20])


# ──────────────────────────────────────────────
# 8. Autoindex
# ──────────────────────────────────────────────
def test_autoindex(host, port):
    section("8 · Autoindex directory listing")

    r, body = get(host, port, "/uploads/")
    body_str = body.decode(errors="replace")
    check("GET /uploads/ → 200",            r.status == 200, r.status, 200)
    check("Autoindex contains <a href",     "<a href" in body_str, body_str[:300])
    check("Content-Type is text/html",
          "text/html" in r.getheader("Content-Type", ""),
          r.getheader("Content-Type"))


# ──────────────────────────────────────────────
# 9. CGI PHP — GET
# ──────────────────────────────────────────────
def test_cgi_get(host, port):
    section("9 · CGI PHP — GET (no body)")

    r, body = get(host, port, "/cgi-bin/image.php")
    body_str = body.decode(errors="replace")
    check("GET image.php → 200",            r.status == 200, r.status, 200)
    check("PHP reports 0 bytes received",   "received: 0" in body_str, body_str)


# ──────────────────────────────────────────────
# 10. CGI PHP — POST image + round-trip
# ──────────────────────────────────────────────
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
          f"{len(dl_body)} vs {len(image_data)} bytes")


# ──────────────────────────────────────────────
# 11. CGI PHP — varying body sizes
# ──────────────────────────────────────────────
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


# ──────────────────────────────────────────────
# 12. CGI Python — info.py GET
# ──────────────────────────────────────────────
def test_cgi_python_get(host, port):
    section("12 · CGI Python — info.py GET")

    r, body = get(host, port, CGI_INFO_URL)
    if r.status == 404:
        skip("info.py GET", "script not in cgi-bin/ — copy info.py there first")
        return
    body_str = body.decode(errors="replace")
    check("GET info.py → 200",              r.status == 200, r.status, 200)
    check("REQUEST_METHOD=GET",             "method        : GET" in body_str, body_str)
    check("body_received=0 on GET",         "body_received : 0" in body_str, body_str)
    check("remote_addr present",            "remote_addr" in body_str, body_str)


# ──────────────────────────────────────────────
# 13. CGI Python — info.py POST
# ──────────────────────────────────────────────
def test_cgi_python_post(host, port):
    section("13 · CGI Python — info.py POST with body")

    payload = b"hello from test suite"
    r, body = post(host, port, CGI_INFO_URL, body=payload,
                   headers={"Content-Type": "text/plain"})
    if r.status == 404:
        skip("info.py POST", "script not deployed")
        return
    body_str = body.decode(errors="replace")
    check("POST info.py → 200",             r.status == 200, r.status, 200)
    check("REQUEST_METHOD=POST",            "method        : POST" in body_str, body_str)
    check(f"body_received={len(payload)}",
          f"body_received : {len(payload)}" in body_str, body_str)
    check("body_preview contains payload",  "hello from test suite" in body_str, body_str)


# ──────────────────────────────────────────────
# 14. CGI Python — worldclock.py
# ──────────────────────────────────────────────
def test_cgi_worldclock(host, port):
    section("14 · CGI Python — worldclock.py")

    r, body = get(host, port, CGI_CLOCK_URL)
    if r.status == 404:
        skip("worldclock.py", "script not in cgi-bin/")
        return
    body_str = body.decode(errors="replace")
    check("GET worldclock.py → 200",        r.status == 200, r.status, 200)
    check("Content-Type text/html",
          "text/html" in r.getheader("Content-Type", ""),
          r.getheader("Content-Type"))
    check("Contains Amsterdam",             "Amsterdam" in body_str, body_str[:400])
    check("Contains Tokyo",                 "Tokyo"     in body_str, body_str[:400])
    check("Contains UTC offset",            "UTC+"      in body_str, body_str[:400])


# ──────────────────────────────────────────────
# 15. CGI query string forwarding
# ──────────────────────────────────────────────
def test_cgi_query_string(host, port):
    section("15 · CGI — QUERY_STRING forwarded to script")

    r, body = get(host, port, f"{CGI_INFO_URL}?foo=bar&x=42")
    if r.status == 404:
        skip("Query string test", "info.py not deployed")
        return
    body_str = body.decode(errors="replace")
    check("QUERY_STRING in CGI output",
          "foo=bar" in body_str, body_str)


# ──────────────────────────────────────────────
# 16. Keep-Alive
# ──────────────────────────────────────────────
def test_keep_alive(host, port):
    section("16 · Keep-Alive — 3 requests on one connection")

    conn = make_connection(host, port)
    try:
        for i in range(3):
            conn.request("GET", "/", headers={
                "Host":       f"{host}:{port}",
                "Connection": "keep-alive",
            })
            r = conn.getresponse()
            r.read()
            check(f"Keep-alive request {i+1}/3 → 200", r.status == 200, r.status, 200)
    except Exception as e:
        check("Keep-alive connection", False, str(e))
    finally:
        conn.close()


# ──────────────────────────────────────────────
# 17. Connection: close honoured
# ──────────────────────────────────────────────
def test_connection_close(host, port):
    section("17 · Connection: close honoured")

    r, _ = get(host, port, "/", headers={"Connection": "close"})
    check("Response has Connection: close",
          r.getheader("Connection", "").lower() == "close",
          r.getheader("Connection"))


# ──────────────────────────────────────────────
# 18. Concurrent connections
# ──────────────────────────────────────────────
def test_concurrent(host, port):
    section("18 · Concurrent connections (10 threads)")

    outcomes = []

    def worker():
        try:
            r, _ = get(host, port, "/")
            outcomes.append(r.status)
        except Exception as e:
            outcomes.append(str(e))

    threads = [threading.Thread(target=worker) for _ in range(10)]
    for t in threads:
        t.start()
    for t in threads:
        t.join(timeout=15)

    ok_count = outcomes.count(200)
    check(f"10 concurrent GET / — all 200  ({ok_count}/10 ok)",
          ok_count == 10, outcomes)


# ──────────────────────────────────────────────
# 19. Sequential CGI — server stays alive
# ──────────────────────────────────────────────
def test_sequential_cgi(host, port):
    section("19 · Sequential CGI — server stays stable")

    for i in range(5):
        payload = f"seq-request-{i}".encode()
        try:
            r, body = post(host, port, UPLOAD_URL, body=payload,
                           headers={"Content-Type": "text/plain"})
            check(f"Sequential CGI {i+1}/5 → 200",
                  r.status == 200, r.status, 200)
        except Exception as e:
            check(f"Sequential CGI {i+1}/5", False, str(e))


# ──────────────────────────────────────────────
# 20. Slow client — partial request
# ──────────────────────────────────────────────
def test_slow_client(host, port):
    section("20 · Slow client — partial headers sent in two chunks")

    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.settimeout(5)
        s.connect((host, port))
        s.send(b"GET / HTTP/1.1\r\n")
        time.sleep(0.3)
        s.send(f"Host: {host}:{port}\r\nConnection: close\r\n\r\n".encode())
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
        check("Slow client — server waits and responds 200",
              b"200" in resp[:20], resp[:20])
    except Exception as e:
        check("Slow client", False, str(e))


# ──────────────────────────────────────────────
# Summary
# ──────────────────────────────────────────────
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


# ──────────────────────────────────────────────
# Entry point
# ──────────────────────────────────────────────
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

    summary()
