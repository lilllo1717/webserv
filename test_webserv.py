#!/usr/bin/env python3
"""
Webserv test suite
Usage: python3 test_webserv.py [--host 127.0.0.1] [--port 8080]
"""

import http.client
import os
import sys
import argparse
import json
import time

# ──────────────────────────────────────────────
# Config
# ──────────────────────────────────────────────
DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 8080

IMAGE_PATH   = "/home/tignatov/webserv_git/uploads/cat.jpg"
UPLOAD_URL   = "/cgi-bin/image.php"
DOWNLOAD_URL = "/uploads/cgi_upload.jpg"

PASS = "\033[92m[PASS]\033[0m"
FAIL = "\033[91m[FAIL]\033[0m"
INFO = "\033[94m[INFO]\033[0m"

results = []

# ──────────────────────────────────────────────
# Helpers
# ──────────────────────────────────────────────
def make_connection(host, port):
    return http.client.HTTPConnection(host, port, timeout=10)


def check(name, condition, got, expected=""):
    status = PASS if condition else FAIL
    msg = f"{status} {name}"
    if not condition:
        msg += f"\n       expected: {expected}\n       got:      {got}"
    print(msg)
    results.append((name, condition))
    return condition


def section(title):
    print(f"\n{'─'*55}")
    print(f"  {title}")
    print(f"{'─'*55}")


# ──────────────────────────────────────────────
# Tests
# ──────────────────────────────────────────────

def get(host, port, path, headers=None):
    """Single GET request on a fresh connection."""
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
    """Single POST request on a fresh connection."""
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


def test_static_get(host, port):
    section("Static GET")

    r, _ = get(host, port, "/")
    check("GET / returns 200", r.status == 200, r.status, 200)

    r, _ = get(host, port, "/does_not_exist.html")
    check("GET missing file returns 404", r.status == 404, r.status, 404)


def test_method_not_allowed(host, port):
    section("Method Not Allowed")

    r, _ = post(host, port, "/", body=b"x")
    check("POST to GET-only / returns 405", r.status == 405, r.status, 405)


def test_redirect(host, port):
    section("Redirect")

    r, _ = get(host, port, "/old")
    check("GET /old returns 301",       r.status == 301, r.status, 301)
    check("Location header is correct", "/new" in r.getheader("Location", ""),
          r.getheader("Location", ""),  "/new?from=old&x=1")


def test_cgi_get(host, port):
    section("CGI — GET (no body)")

    r, body = get(host, port, "/cgi-bin/image.php")
    body_str = body.decode(errors="replace")
    check("GET image.php returns 200",          r.status == 200, r.status, 200)
    check("Response contains byte count", "bytes" in body_str, body_str)
    check("0 bytes received on GET",      "received: 0" in body_str, body_str)


def test_cgi_post_image(host, port):
    section("CGI — POST image upload")

    if not os.path.exists(IMAGE_PATH):
        print(f"{INFO} Skipping image upload: {IMAGE_PATH} not found")
        return

    with open(IMAGE_PATH, "rb") as f:
        image_data = f.read()

    expected_bytes = len(image_data)
    r, body = post(host, port, UPLOAD_URL, body=image_data,
                   headers={"Content-Type": "image/jpeg"})
    body_str = body.decode(errors="replace")

    check("POST image.php returns 200",         r.status == 200, r.status, 200)
    check(f"PHP reports {expected_bytes} bytes",
          f"received: {expected_bytes}" in body_str, body_str)


def test_cgi_post_sizes(host, port):
    section("CGI — POST varying body sizes")
    sizes = [0, 1, 255, 1024, 8192, 65535]
    for size in sizes:
        payload = bytes(range(256)) * (size // 256) + bytes(range(size % 256))
        try:
            r, body = post(host, port, UPLOAD_URL, body=payload,
                           headers={"Content-Type": "application/octet-stream"})
            body_str = body.decode(errors="replace")
            check(f"POST {size:>6} bytes → PHP reports correct count",
                  f"received: {size}" in body_str, body_str)
        except Exception as e:
            check(f"POST {size:>6} bytes", False, str(e))


def test_serve_uploaded_image(host, port):
    section("Static GET — serve uploaded image")

    # Upload cat.jpg first so cgi_upload.jpg is fresh
    if os.path.exists(IMAGE_PATH):
        with open(IMAGE_PATH, "rb") as f:
            image_data = f.read()
        post(host, port, UPLOAD_URL, body=image_data,
             headers={"Content-Type": "image/jpeg"})
        time.sleep(0.2)

    r, body = get(host, port, DOWNLOAD_URL)

    check("GET cgi_upload.jpg returns 200",           r.status == 200, r.status, 200)
    check("Content-Type is image/jpeg",               r.getheader("Content-Type", "") == "image/jpeg",
          r.getheader("Content-Type"))
    check("Content-Length > 0",                       int(r.getheader("Content-Length", "0")) > 0,
          r.getheader("Content-Length"))
    check("Body is non-empty",                        len(body) > 0, len(body))
    check("Body starts with JPEG magic bytes (FFD8)", body[:2] == b"\xff\xd8",
          body[:2].hex(), "ffd8")

    if os.path.exists(IMAGE_PATH):
        with open(IMAGE_PATH, "rb") as f:
            original = f.read()
        check("Body matches original cat.jpg", body == original,
              f"{len(body)} vs {len(original)} bytes")


def test_content_length_header(host, port):
    section("Content-Length accuracy")

    r, body = get(host, port, "/")
    cl = int(r.getheader("Content-Length", "-1"))
    check("Content-Length matches actual body size", cl == len(body),
          f"header={cl} body={len(body)}")


def test_keep_alive(host, port):
    section("Keep-Alive — two requests on one connection")
    conn = make_connection(host, port)
    try:
        for i in range(2):
            conn.request("GET", "/", headers={
                "Host":       f"{host}:{port}",
                "Connection": "keep-alive",
            })
            r = conn.getresponse()
            r.read()
            check(f"Keep-alive request {i+1} returns 200", r.status == 200, r.status, 200)
    except Exception as e:
        check("Keep-alive second request", False, str(e))
    finally:
        conn.close()


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


# ──────────────────────────────────────────────
# Entry point
# ──────────────────────────────────────────────
if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    args = parser.parse_args()

    print(f"\n{INFO} Testing http://{args.host}:{args.port}")

    test_static_get(args.host, args.port)
    test_method_not_allowed(args.host, args.port)
    test_redirect(args.host, args.port)
    test_cgi_get(args.host, args.port)
    test_cgi_post_image(args.host, args.port)
    test_cgi_post_sizes(args.host, args.port)
    test_serve_uploaded_image(args.host, args.port)
    test_content_length_header(args.host, args.port)
    test_keep_alive(args.host, args.port)

    summary()
