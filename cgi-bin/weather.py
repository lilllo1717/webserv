import sys
import os

# read query string → city=Amsterdam
query = os.environ.get("QUERY_STRING", "")
params = dict(p.split("=") for p in query.split("&") if "=" in p)
city = params.get("city", "Unknown")

html = f"""<!DOCTYPE html>
<html>
<head><title>Weather - {city}</title></head>
<body>
    <h1>Weather for {city}</h1>
    <p>QUERY_STRING was: {query}</p>
    <a href="/cgi-bin/worldclock.py">← Back to world clock</a>
</body>
</html>
"""

body = html.encode("utf-8")
sys.stdout.buffer.write(b"Content-Type: text/html; charset=utf-8\r\n")
sys.stdout.buffer.write(f"Content-Length: {len(body)}\r\n".encode())
sys.stdout.buffer.write(b"\r\n")
sys.stdout.buffer.write(body)
sys.stdout.buffer.flush()