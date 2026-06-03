#!/bin/bash
# ─────────────────────────────────────────────
# webserv evaluation — curl test cheat sheet
# ─────────────────────────────────────────────

# ── 1. Multiple ports ────────────────────────
curl -v http://127.0.0.1:8080/
curl -v http://127.0.0.1:8081/

# ── 2. Different hostnames (same port) ───────
curl -v --resolve example.com:8080:127.0.0.1 http://example.com:8080/
curl -v --resolve other.com:8080:127.0.0.1 http://other.com:8080/

# ── 3. Custom 404 error page ─────────────────
curl -v http://127.0.0.1:8080/this-does-not-exist

# ── 4. Client body size limit ────────────────
# small body → 201
curl -v -X POST -H "Content-Type: plain/text" \
  --data "short" \
  http://127.0.0.1:8081/upload

# large body → 413
curl -v -X POST -H "Content-Type: plain/text" \
  --data "BODY IS HERE write something longer than one hundred bytes to trigger the limit on this server!!!!!" \
  http://127.0.0.1:8081/upload

# ── 5. Routes to different directories ───────
curl -v http://127.0.0.1:8080/
curl -v http://127.0.0.1:8080/uploads/
curl -v http://127.0.0.1:8080/cgi-bin/hello.php

# ── 6. Default index file for directory ──────
curl -v http://127.0.0.1:8080/           # serves index.html
curl -v http://127.0.0.1:8080/testAutoindex/  # shows directory listing

# ── 7. Method restrictions ───────────────────
# GET on POST-only route → 405
curl -v -X GET http://127.0.0.1:8080/upload

# DELETE on GET-only route → 405
curl -v -X DELETE http://127.0.0.1:8080/

# POST on GET+DELETE route → 405
curl -v -X POST -d "test" http://127.0.0.1:8080/uploads/

# DELETE with permission → 204
curl -v -X POST -H "Content-Type: text/plain" \
  --data "hello world" \
  http://127.0.0.1:8080/upload
curl -v -X DELETE http://127.0.0.1:8080/uploads/upload.txt

# verify deleted → 404
curl -v http://127.0.0.1:8080/uploads/upload.txt

# ── 8. GET / POST / DELETE + upload round trip
curl -v http://127.0.0.1:8080/uploads/cat.jpg -o /dev/null

curl -v -X POST -H "Content-Type: text/plain" \
  --data "hello evaluator" \
  http://127.0.0.1:8080/upload

curl -v http://127.0.0.1:8080/uploads/upload.txt

curl -v -X DELETE http://127.0.0.1:8080/uploads/upload.txt

curl -v http://127.0.0.1:8080/uploads/upload.txt   # → 404

# ── 9. Unknown method — must not crash ───────
curl -v -X FOOBAR http://127.0.0.1:8080/

# server still alive after unknown method
curl -v http://127.0.0.1:8080/

# ── 10. CGI ───────────────────────────────────
curl -v http://127.0.0.1:8080/cgi-bin/hello.php
curl -v http://127.0.0.1:8080/cgi-bin/worldclock.py
curl -v http://127.0.0.1:8080/cgi-bin/info.py
