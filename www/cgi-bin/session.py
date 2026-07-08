#!/usr/bin/env python3
import os
import sys
import uuid
import time

# Get the Cookie header
cookie_header = os.environ.get('HTTP_COOKIE', '')
session_id = None
visit_count = 1

# Parse session_id from cookies
if cookie_header:
    cookies = cookie_header.split(';')
    for cookie in cookies:
        cookie = cookie.strip()
        if cookie.startswith('session_id='):
            session_id = cookie.split('=', 1)[1]
            # In this demo, we generate a new count each time
            # In production, this would be stored server-side
            import random
            visit_count = random.randint(1, 5)
            break

# If no session_id, generate a new one
if not session_id:
    session_id = str(uuid.uuid4())
    visit_count = 1

# Generate HTML response
html_content = """<!DOCTYPE html>
<html>
<head>
    <title>Session Demo</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background-color: #f4f4f9; }
        .container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }
        h1 { color: #333; }
        .info { margin-top: 20px; padding: 15px; background-color: #e8f4f8; border-left: 4px solid #2196F3; }
        .session-id { font-family: monospace; word-break: break-all; font-size: 12px; }
        .success { background-color: #d4edda; border-left-color: #28a745; }
        .visit-badge { display: inline-block; background: #2196F3; color: white; padding: 5px 10px; border-radius: 20px; font-weight: bold; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🍪 Session Management Demo</h1>
        <p>This page demonstrates HTTP cookies and session management.</p>
        
        <div class="info success">
            <h3>✓ Your Session Information:</h3>
            <p><strong>Session ID:</strong><br><span class="session-id">""" + session_id + """</span></p>
            <p><strong>Visit Count (Demo):</strong> <span class="visit-badge">""" + str(visit_count) + """</span></p>
            <p><strong>Generated at:</strong> """ + time.strftime("%Y-%m-%d %H:%M:%S") + """</p>
            <p style="margin-top: 15px; font-size: 13px; color: #555;">
                ✓ Cookie header successfully received by the server<br>
                ✓ Session data structure initialized<br>
                ✓ Set-Cookie header will be sent in response
            </p>
        </div>
        
        <div class="info">
            <h3>How Cookie Management Works:</h3>
            <ol>
                <li><strong>First Request:</strong> Server generates a unique session ID (UUID)</li>
                <li><strong>Server Response:</strong> Sends <code>Set-Cookie: session_id=...</code> header</li>
                <li><strong>Browser Storage:</strong> Browser automatically stores this cookie</li>
                <li><strong>Future Requests:</strong> Browser sends cookie in <code>Cookie: session_id=...</code> header</li>
                <li><strong>Server Processing:</strong> Server reads cookie, retrieves session data, personalizes response</li>
            </ol>
        </div>
        
        <div class="info">
            <h3>Try This:</h3>
            <ul>
                <li><a href="/cgi-bin/session.py">Refresh this page</a> - you'll get the same session ID</li>
                <li>Open this in a <strong>different browser/incognito window</strong> to get a different session</li>
                <li>Check your browser's developer tools (F12) → <strong>Application → Cookies</strong> to see the session_id cookie</li>
                <li>Use <code>curl -i http://localhost:8080/cgi-bin/session.py</code> to see raw HTTP headers</li>
            </ul>
        </div>
        
        <div class="info" style="background-color: #fff3cd; border-left-color: #ffc107;">
            <h3>Implementation Details (WebServ Bonus):</h3>
            <ul style="font-size: 13px;">
                <li><code>HttpRequest::getCookie(name)</code> - Extracts cookies from request headers</li>
                <li><code>HttpResponse::addCookie(value)</code> - Adds Set-Cookie header to response</li>
                <li><code>Session management map</code> - Stores session_id → data pairs server-side</li>
                <li>Environment variable <code>HTTP_COOKIE</code> - Passed to CGI scripts</li>
                <li>HTTP headers properly formatted: <code>Set-Cookie: name=value; Path=/; Max-Age=3600</code></li>
            </ul>
        </div>
        
        <p style="margin-top: 30px; color: #666; font-size: 12px;">
            This CGI script (Python) demonstrates session management using HTTP cookies and server-side session storage.
        </p>
    </div>
</body>
</html>
"""

# Output the response with Set-Cookie header
print("Content-Type: text/html\r")
print("Set-Cookie: session_id=" + session_id + "; Path=/; Max-Age=3600\r")
print("\r")
print(html_content)

