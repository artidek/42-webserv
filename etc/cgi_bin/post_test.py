#!/usr/bin/env python3

import sys
import os
import urllib.parse
import html

# --- Read POST body ---
content_length = int(os.environ.get("CONTENT_LENGTH", 0))
post_data = sys.stdin.read(content_length)

# --- Parse form data ---
params = urllib.parse.parse_qs(post_data)

name = params.get("name", [""])[0]
message = params.get("message", [""])[0]

# --- Validate input ---
if not name or not message:
    body = """<!DOCTYPE html>
<html>
<head><title>Error</title></head>
<body>
<h1>400 Bad Request</h1>
<p>Name and message are required.</p>
</body>
</html>
"""
    response = (
        "HTTP/1.1 400 Bad Request\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        f"Content-Length: {len(body.encode('utf-8'))}\r\n"
        "\r\n"
        f"{body}"
    )
    sys.stdout.write(response)
    sys.exit(0)

# --- Sanitize filename ---
safe_name = "".join(c for c in name if c.isalnum() or c in ("_", "-"))
filename = safe_name + ".txt"

# --- Save message to file ---
try:
    with open(filename, "w", encoding="utf-8") as f:
        f.write(message)
    save_status = True
except Exception:
    save_status = False

# --- Build HTML body ---
if save_status:
    body = f"""<!DOCTYPE html>
<html>
<head>
    <title>Message Received</title>
</head>
<body>
    <h1>Success</h1>
    <p>{html.escape(name)}, your message has been received.</p>
</body>
</html>
"""
    status_line = "HTTP/1.1 200 OK\r\n"
else:
    body = """<!DOCTYPE html>
<html>
<head><title>Server Error</title></head>
<body>
<h1>500 Internal Server Error</h1>
<p>Could not save message.</p>
</body>
</html>
"""
    status_line = "HTTP/1.1 500 Internal Server Error\r\n"

# --- Output full HTTP response ---
response = (
    status_line +
    "Content-Type: text/html; charset=UTF-8\r\n" +
    f"Content-Length: {len(body.encode('utf-8'))}\r\n" +
    "\r\n" +
    body
)

sys.stdout.write(response)
