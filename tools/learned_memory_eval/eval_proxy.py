#!/usr/bin/env python3
"""Records each model request of the browser, and forwards it to Ollama.

Record mode (the only mode today). The proxy listens on 127.0.0.1, forwards
every POST to Ollama, and writes one JSONL line for each call:
  {"t", "set", "path", "kind", "body" (the exact request text), "status",
   "answer" (the response text), "latency_ms"}
"kind" is gate, sentence, relation, relevance, rewrite, relation-llm, merge
or other (lib/requests.py classify()). parity_check.py reads the file.

  ./eval_proxy.py <out>/proxy.jsonl              # port 8898
  ./eval_proxy.py <out>/proxy.jsonl --port 8899 --ollama http://localhost:11434

run_eval.py --proxy starts it for each set and points the decision model URL
and the BYOM endpoint at it.
"""
from __future__ import annotations

import argparse
import datetime
import http.client
import json
import threading
import time
import urllib.parse
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

from lib import requests as rq

DEFAULT_PORT = 8898


class _Handler(BaseHTTPRequestHandler):
    server_version = "LearnedMemoryEvalProxy/1"

    def do_POST(self):  # noqa: N802 (http.server API)
        body = self.rfile.read(int(self.headers.get("Content-Length", 0)))
        start = time.time()
        upstream = urllib.parse.urlparse(self.server.ollama)
        status, answer, headers = 0, b"", {}
        try:
            conn = http.client.HTTPConnection(upstream.hostname, upstream.port or 80,
                                              timeout=self.server.timeout_s)
            conn.request("POST", self.path, body,
                         {"Content-Type": self.headers.get("Content-Type", "application/json")})
            response = conn.getresponse()
            status, answer = response.status, response.read()
            headers = {"Content-Type": response.getheader("Content-Type", "application/json")}
        except (OSError, http.client.HTTPException) as e:
            status, answer = 502, json.dumps({"error": f"proxy: {e}"}).encode()
            headers = {"Content-Type": "application/json"}
        latency = int((time.time() - start) * 1000)
        self.send_response(status)
        for k, v in headers.items():
            self.send_header(k, v)
        self.send_header("Content-Length", str(len(answer)))
        self.end_headers()
        self.wfile.write(answer)
        self.server.record(self.path, body, status, answer, latency)

    def log_message(self, *args):
        pass


class RecordingProxy(ThreadingHTTPServer):
    daemon_threads = True

    def __init__(self, log_path: Path, ollama: str, port: int = DEFAULT_PORT,
                 timeout_s: float = 900):
        super().__init__(("127.0.0.1", port), _Handler)
        self.ollama = ollama.rstrip("/")
        self.timeout_s = timeout_s
        self.log_path = Path(log_path)
        self.log_path.parent.mkdir(parents=True, exist_ok=True)
        self.lock = threading.Lock()
        self.counts: dict = {}
        self.thread = None
        # run_eval.py sets the name of the chat set that runs now.
        self.label = None

    @property
    def url(self) -> str:
        return f"http://127.0.0.1:{self.server_address[1]}"

    def record(self, path: str, body: bytes, status: int, answer: bytes, latency: int):
        text = body.decode("utf-8", "replace")
        try:
            kind = rq.classify(path, json.loads(text))
        except (json.JSONDecodeError, AttributeError):
            kind = "other"
        line = {"t": datetime.datetime.now().isoformat(timespec="milliseconds"),
                "set": self.label, "path": path, "kind": kind, "body": text, "status": status,
                "answer": answer.decode("utf-8", "replace"), "latency_ms": latency}
        with self.lock:
            self.counts[kind] = self.counts.get(kind, 0) + 1
            with open(self.log_path, "a", encoding="utf-8") as f:
                f.write(json.dumps(line, ensure_ascii=False) + "\n")

    def start(self) -> "RecordingProxy":
        self.thread = threading.Thread(target=self.serve_forever, daemon=True)
        self.thread.start()
        return self

    def stop(self) -> None:
        self.shutdown()
        self.server_close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("log", help="JSONL file for the records")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT)
    parser.add_argument("--ollama", default="http://localhost:11434")
    args = parser.parse_args()
    proxy = RecordingProxy(Path(args.log), args.ollama, args.port)
    print(f"Recording to {args.log}. Endpoints: {proxy.url}/v1/systemone, "
          f"{proxy.url}/v1/chat/completions", flush=True)
    try:
        proxy.serve_forever()
    except KeyboardInterrupt:
        pass
    print("Calls: " + ", ".join(f"{k} {v}" for k, v in sorted(proxy.counts.items())))


if __name__ == "__main__":
    main()
