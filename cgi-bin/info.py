#!/usr/bin/env python3
import sys
import os

content_len = int(os.environ.get("CONTENT_LENGTH", "0") or "0")
body_raw    = sys.stdin.buffer.read(content_len) if content_len > 0 else b""

method       = os.environ.get("REQUEST_METHOD", "")
query        = os.environ.get("QUERY_STRING", "")
content_type = os.environ.get("CONTENT_TYPE", "")
script       = os.environ.get("SCRIPT_FILENAME", "")
remote       = os.environ.get("REMOTE_ADDR", "")

lines = []
lines.append(f"method        : {method}")
lines.append(f"query_string  : {query}")
lines.append(f"content_type  : {content_type}")
lines.append(f"content_length: {content_len}")
lines.append(f"body_received : {len(body_raw)}")
lines.append(f"script        : {script}")
lines.append(f"remote_addr   : {remote}")

if body_raw:
    lines.append(f"body_preview  : {body_raw[:64]}")

output = "\n".join(lines) + "\n"

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write(f"Content-Length: {len(output)}\r\n")
sys.stdout.write("\r\n")
sys.stdout.write(output)
sys.stdout.flush()
