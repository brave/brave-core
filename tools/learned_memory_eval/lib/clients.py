"""Clients for Ollama: /v1/systemone (decision model) and /v1/chat/completions
(local LLM), with retries, latency and a disk cache.

The cache key is the sha256 of the model digest, the endpoint, the exact body
and the repeat index. A new model digest or a new wording gives a new key, so
the cache never gives an old answer. The cache is a JSONL file for each
endpoint in <results root>/cache/.
"""

from __future__ import annotations

import hashlib
import json
import os
import threading
import time
import urllib.error
import urllib.request
from pathlib import Path

DEFAULT_OLLAMA = "http://localhost:11434"
DEFAULT_RESULTS_ROOT = "~/brave/learned_memory_eval_runs"


def results_root(path: str | None = None) -> Path:
    """The results root: the flag, else $LM_EVAL_RUNS, else the default."""
    return Path(
        path or os.environ.get("LM_EVAL_RUNS") or DEFAULT_RESULTS_ROOT
    ).expanduser()


class ModelError(Exception):
    """The model answered with an HTTP error, or it did not answer."""

    def __init__(self, message: str, status: int | None = None, body: str = ""):
        super().__init__(message)
        self.status = status
        self.body = body


def http_json(url: str, body: bytes | None = None, timeout: float = 600):
    request = urllib.request.Request(
        url,
        body,
        {"Content-Type": "application/json"} if body is not None else {},
    )
    with urllib.request.urlopen(request, timeout=timeout) as response:
        return json.load(response)


_digests: dict = {}
_digest_lock = threading.Lock()


def model_digest(ollama: str, model: str) -> str:
    """The digest of an installed model, from /api/tags."""
    with _digest_lock:
        if (ollama, model) not in _digests:
            tags = http_json(f"{ollama}/api/tags", timeout=10)
            for m in tags.get("models", []):
                _digests[(ollama, m["name"])] = m.get("digest", "")
        digest = _digests.get((ollama, model))
    if digest is None:
        raise ModelError(
            f"model {model} is not installed in Ollama at {ollama}"
        )
    return digest


class DiskCache:
    """Append-only JSONL file: one line {"key", "value"} for each answer."""

    def __init__(self, path: Path | None):
        self.path = path
        self.lock = threading.Lock()
        self.data: dict = {}
        if path and path.exists():
            with open(path, encoding="utf-8") as f:
                for line in f:
                    try:
                        item = json.loads(line)
                    except json.JSONDecodeError:
                        continue  # A line that a stopped run did not finish.
                    self.data[item["key"]] = item["value"]

    def get(self, key: str):
        with self.lock:
            return self.data.get(key)

    def put(self, key: str, value) -> None:
        if not self.path:
            return
        with self.lock:
            self.data[key] = value
            self.path.parent.mkdir(parents=True, exist_ok=True)
            with open(self.path, "a", encoding="utf-8") as f:
                f.write(
                    json.dumps({"key": key, "value": value}, ensure_ascii=False)
                    + "\n"
                )


class Client:
    """Sends exact request bodies and returns the parsed answer.

    Each call returns a dict: "answer" (the parsed JSON response or None),
    "status" (HTTP status, 0 when there was no answer), "error" (text or None),
    "latency_ms" (of the first, uncached call), "cached" (bool).
    A 4xx answer is a result, not an exception: Laya answers a request that is
    too long with HTTP 400.
    """

    def __init__(
        self,
        endpoint: str,
        ollama: str = DEFAULT_OLLAMA,
        cache_dir: Path | None = None,
        use_cache: bool = True,
        refresh: bool = False,
        retries: int = 2,
        timeout: float = 600,
    ):
        self.ollama = ollama.rstrip("/")
        self.endpoint = endpoint
        self.url = self.ollama + endpoint
        self.retries = retries
        self.timeout = timeout
        name = endpoint.strip("/").replace("/", "_")
        self.cache = DiskCache(
            cache_dir / f"{name}.jsonl" if (cache_dir and use_cache) else None
        )
        self.use_cache = use_cache
        # Send each request again and write the new answer, for example to
        # measure latency. A later line of the cache file wins.
        self.refresh = refresh
        self.calls = 0
        self.cache_hits = 0
        self.stats_lock = threading.Lock()

    def key(self, model: str, body: str, repeat: int) -> str:
        h = hashlib.sha256()
        for part in (
            model_digest(self.ollama, model),
            self.endpoint,
            body,
            str(repeat),
        ):
            h.update(part.encode())
            h.update(b"\0")
        return h.hexdigest()

    def send(self, model: str, body: str, repeat: int = 0) -> dict:
        key = self.key(model, body, repeat) if self.use_cache else None
        if key and not self.refresh:
            hit = self.cache.get(key)
            if hit is not None:
                with self.stats_lock:
                    self.cache_hits += 1
                return {**hit, "cached": True}
        result = self._post(body)
        with self.stats_lock:
            self.calls += 1
        # Do not cache a connection error or a server error: a rerun tries again.
        if key and (result["status"] == 200 or 400 <= result["status"] < 500):
            self.cache.put(key, result)
        return {**result, "cached": False}

    def _post(self, body: str) -> dict:
        data = body.encode("utf-8")
        error = None
        for attempt in range(self.retries + 1):
            start = time.time()
            try:
                request = urllib.request.Request(
                    self.url, data, {"Content-Type": "application/json"}
                )
                with urllib.request.urlopen(
                    request, timeout=self.timeout
                ) as response:
                    text = response.read().decode("utf-8", "replace")
                    status = response.status
                latency = int((time.time() - start) * 1000)
                try:
                    answer = json.loads(text)
                except json.JSONDecodeError:
                    return {
                        "answer": None,
                        "status": status,
                        "latency_ms": latency,
                        "error": f"not JSON: {text[:200]}",
                    }
                return {
                    "answer": answer,
                    "status": status,
                    "latency_ms": latency,
                    "error": None,
                }
            except urllib.error.HTTPError as e:
                latency = int((time.time() - start) * 1000)
                text = e.read().decode("utf-8", "replace")
                if 400 <= e.code < 500:
                    try:
                        message = json.loads(text).get("error", text)
                    except (json.JSONDecodeError, AttributeError):
                        message = text
                    return {
                        "answer": None,
                        "status": e.code,
                        "latency_ms": latency,
                        "error": str(message)[:500],
                    }
                error = f"HTTP {e.code}: {text[:200]}"
            except (urllib.error.URLError, OSError, TimeoutError) as e:
                error = str(e)
            if attempt < self.retries:
                time.sleep(2 * (attempt + 1))
        return {"answer": None, "status": 0, "latency_ms": None, "error": error}


class SystemOneClient(Client):
    """The decision model. send() takes the body from lib/requests.py."""

    def __init__(self, ollama: str = DEFAULT_OLLAMA, **kwargs):
        super().__init__("/v1/systemone", ollama, **kwargs)

    def ask(self, model: str, body: str, repeat: int = 0) -> dict:
        result = self.send(model, body, repeat)
        answer = result["answer"]
        result["answers"] = (
            answer.get("answers") if isinstance(answer, dict) else None
        )
        if result["answers"] is None and not result["error"]:
            result["error"] = "no answers in the response"
        return result


class ChatClient(Client):
    """The local LLM through the OpenAI-compatible endpoint."""

    def __init__(self, ollama: str = DEFAULT_OLLAMA, **kwargs):
        super().__init__("/v1/chat/completions", ollama, **kwargs)

    def complete(self, model: str, body: str, repeat: int = 0) -> dict:
        result = self.send(model, body, repeat)
        content = None
        try:
            content = result["answer"]["choices"][0]["message"].get("content")
        except (TypeError, KeyError, IndexError):
            if not result["error"]:
                result["error"] = "no completion in the response"
        result["content"] = content
        return result


def make_clients(
    ollama: str = DEFAULT_OLLAMA,
    root: Path | None = None,
    use_cache: bool = True,
    refresh: bool = False,
) -> tuple:
    cache_dir = (root or results_root()) / "cache"
    return (
        SystemOneClient(
            ollama, cache_dir=cache_dir, use_cache=use_cache, refresh=refresh
        ),
        ChatClient(
            ollama, cache_dir=cache_dir, use_cache=use_cache, refresh=refresh
        ),
    )
