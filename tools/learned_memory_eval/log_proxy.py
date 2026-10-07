#!/usr/bin/env python3
"""Logs each request to a chat model endpoint, then forwards it to Ollama.

Use it to see what Leo sends to a BYOM model, for example the memory block with
the learned memories. Point the endpoint of the custom model at
http://127.0.0.1:8899/v1/chat/completions, then run

  ./log_proxy.py /tmp/requests.jsonl

Each line of the log has the time, the path and the request body.
"""
import http.client
import json
import sys
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

LOG = sys.argv[1] if len(sys.argv) > 1 else "/tmp/requests.jsonl"
class H(BaseHTTPRequestHandler):
    def do_POST(self):
        body = self.rfile.read(int(self.headers.get('Content-Length', 0)))
        try:
            req = json.loads(body)
        except Exception:
            req = {"raw": body.decode(errors='replace')}
        with open(LOG, 'a') as f:
            f.write(json.dumps({"t": time.strftime('%H:%M:%S'), "path": self.path, "request": req}) + "\n")
        up = http.client.HTTPConnection('localhost', 11434, timeout=600)
        up.request('POST', self.path, body, {'Content-Type': 'application/json'})
        r = up.getresponse()
        self.send_response(r.status)
        self.send_header('Content-Type', r.getheader('Content-Type', 'application/json'))
        self.end_headers()
        while True:
            chunk = r.read1(8192) if hasattr(r, 'read1') else r.read(8192)
            if not chunk:
                break
            self.wfile.write(chunk); self.wfile.flush()
    def log_message(self, *a): pass
ThreadingHTTPServer(('127.0.0.1', 8899), H).serve_forever()
